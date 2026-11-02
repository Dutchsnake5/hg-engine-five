"""Finds the roof colours of every building texture (SEASONAL_MAPS winter roofs):
    season_roofs.py <rom.nds> <output json> [preview png]   (also writes roof_masks.json next to the output json)

Roofs share each building's texture atlas with its walls, windows and door, so whole palettes can't be recoloured.
Instead every exterior building model (archive a/0/4/0) is read: triangles facing up (normal y > 0.3) in the upper
part of the model are roof. Their texture coordinates are rasterised onto the texture, and a palette colour counts as
roof when at least 60% of the texels using it lie on a roof. Output: {texture set: {palette: [palette indices]}} for
the building texture sets (a/0/7/0), read by scripts/build_seasons.py. roof_masks.json keeps the roof texels themselves
({texture: {w, h, pals, bits}}, bits = base64 of the zlib-compressed row-major bitmap; texels a wall or low face also
shows are left out) and, under '_pairs', every palette with the textures the models show with it, so winter can also snow the
roof texels whose colours are shared with walls, signs or logos (scripts/season_recolor.py snow_roof_texels). Needs numpy and the Script Inspector's
maprender.py (TexSet, Model)."""
import base64
import json
import os
import zlib
import struct
import sys
from collections import defaultdict

import ndspy.narc
import ndspy.rom
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.environ.get('SCRIPT_INSPECTOR', '/mnt/c/Users/Drew/Desktop/HG ROM Hack Project/ScriptInspector'))
import maprender as M  # noqa: E402

_OV = os.path.join(HERE, '..', 'data', 'seasons', 'roof_overrides.json')
OVERRIDES = {k: v for k, v in json.load(open(_OV)).items() if not k.startswith('_')} if os.path.exists(_OV) else {}
UP, HIGH, SHARE = 0.15, 0.45, 0.6   # steep roofs (Mahogany's) face up only a little
# textures that face up but aren't roofs: shadows, window glass, the whirlpool, flower planters (they wilt instead)
SKIP = ('kage', 'mado', 'uzushio', 'wind', 'glass', '_sp')


def models_of(data):
    if data[:4] != b'BMD0':
        return []
    out = []
    for i in range(struct.unpack_from('<H', data, 0x0E)[0]):
        o = struct.unpack_from('<I', data, 0x10 + 4 * i)[0]
        if data[o:o + 4] == b'MDL0':
            out.append(M.Model(data, o))
    return out


def rasterise(mask, uv, repeat=(1, 1), flip=(0, 0)):
    """Mark the texels a triangle's texture coordinates cover, wrapping, clamping or mirroring them the way the
    material does (maprender's Canvas._wrap)."""
    h, w = mask.shape
    lo, hi = np.floor(uv.min(0)).astype(int), np.ceil(uv.max(0)).astype(int)
    if (hi - lo).min() <= 0:
        return
    wrap = M.Canvas._wrap
    if (hi - lo).prod() > 4 * w * h:
        # a face that repeats its texture many times (long roofs tile a thin strip): mark its bounding box
        xs = wrap(np.arange(lo[0], min(hi[0], lo[0] + 2 * w)), w, repeat[0], flip[0])
        ys = wrap(np.arange(lo[1], min(hi[1], lo[1] + 2 * h)), h, repeat[1], flip[1])
        mask[np.ix_(np.unique(ys), np.unique(xs))] = True
        return
    # sample points across the triangle (pixel-centre tests miss slivers: Mahogany's roofs use a strip under a texel
    # wide, stretched along the roof)
    n = int(min(512, max(4, 4 * (hi - lo).max())))
    i, j = np.mgrid[0:n + 1, 0:n + 1]
    keep = i + j <= n
    a, b = i[keep] / n, j[keep] / n
    p = np.outer(a, uv[0]) + np.outer(b, uv[1]) + np.outer(1 - a - b, uv[2])
    xs, ys = np.floor(p[:, 0]).astype(int), np.floor(p[:, 1]).astype(int)
    mask[wrap(ys, h, repeat[1], flip[1]), wrap(xs, w, repeat[0], flip[0])] = True


def closed_holes(mask):
    """Texels with roof on at least 70% of their neighbours inside the texture: single missed texels (specks of the old
    colour where a window's side faces touch the roof texture, gaps along a texture's edge) fill in; real wall regions
    are bigger and stay."""
    h, w = mask.shape
    pad = np.pad(mask, 1).astype(int)
    inside = np.pad(np.ones_like(mask), 1).astype(int)
    roof = sum(pad[1 + dy:h + 1 + dy, 1 + dx:w + 1 + dx] for dy in (-1, 0, 1) for dx in (-1, 0, 1) if dy or dx)
    valid = sum(inside[1 + dy:h + 1 + dy, 1 + dx:w + 1 + dx] for dy in (-1, 0, 1) for dx in (-1, 0, 1) if dy or dx)
    return roof >= 0.7 * valid


def grown(mask, wall, img, steps=2):
    """Roof texels' neighbours (up to `steps` away) of the same colours as the roof and not on a wall: the edge
    columns and ends of a roof strip that thin faces miss (Elm's lab)."""
    rgb = img[..., :3].astype(np.int32)
    key = (rgb[..., 0] << 16) | (rgb[..., 1] << 8) | rgb[..., 2]
    vals, counts = np.unique(key[mask & (img[..., 3] > 0)], return_counts=True)
    roofcol = np.isin(key, vals[counts >= 3])
    out = mask.copy()
    for _ in range(steps):
        pad = np.pad(out, 1)
        near = pad[:-2, 1:-1] | pad[2:, 1:-1] | pad[1:-1, :-2] | pad[1:-1, 2:]
        out |= near & roofcol & ~wall
    return out & ~mask


HOLE_MAX = 64   # gaps of roof colours inside a roof up to this many texels are roof (a little wall trim shares them)


def small_gaps(mask, img):
    """Groups of roof-coloured texels left out of the roof (a small piece of wall trim shows them too) that are small
    and mostly surrounded by roof: patches of the old colour on a snowy roof look worse than a bit of snow on the trim."""
    rgb = img[..., :3].astype(np.int32)
    key = (rgb[..., 0] << 16) | (rgb[..., 1] << 8) | rgb[..., 2]
    vals, counts = np.unique(key[mask & (img[..., 3] > 0)], return_counts=True)
    cand = np.isin(key, vals[counts >= 3]) & ~mask & (img[..., 3] > 0)
    h, w = mask.shape
    seen = np.zeros_like(mask)
    out = np.zeros_like(mask)
    for y0 in range(h):
        for x0 in range(w):
            if not cand[y0, x0] or seen[y0, x0]:
                continue
            comp, stack, edge_roof, edge_other = [], [(y0, x0)], 0, 0
            seen[y0, x0] = True
            while stack:
                y, x = stack.pop()
                comp.append((y, x))
                for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    ny, nx = y + dy, x + dx
                    if not (0 <= ny < h and 0 <= nx < w):
                        continue
                    if mask[ny, nx]:
                        edge_roof += 1
                    elif cand[ny, nx]:
                        if not seen[ny, nx]:
                            seen[ny, nx] = True
                            stack.append((ny, nx))
                    else:
                        edge_other += 1
            if len(comp) <= HOLE_MAX and edge_roof >= edge_other:   # mostly surrounded by roof
                for y, x in comp:
                    out[y, x] = True
    return out


def main():
    rom = ndspy.rom.NintendoDSRom.fromFile(sys.argv[1])
    texsets = ndspy.narc.NARC(rom.getFileByName('a/0/7/0')).files
    models = ndspy.narc.NARC(rom.getFileByName('a/0/4/0')).files
    # where each (texture, palette) lives, and its size
    owner = defaultdict(list)
    for si, d in enumerate(texsets):
        for ts in M.load_texsets(d):
            for tn, (param, _) in ts.textures.items():
                owner[tn].append((si, ts, param))
    roof, wall, pairs = {}, {}, defaultdict(set)
    for d in models:
        for mdl in models_of(d):
            tris = mdl.triangles()
            if not tris:
                continue
            ys = np.array([t[0][:, 1] for t in tris])
            ymin, ymax = ys.min(), ys.max()
            for pos, uv, _col, mid in tris:
                if mid < 0 or mid >= len(mdl.materials):
                    continue
                mat = mdl.materials[mid]
                if not mat['tex'] or mat['tex'] not in owner:
                    continue
                if mat['pal']:
                    pairs[mat['pal']].add(mat['tex'])
                if any(k in mat['tex'] for k in SKIP):
                    continue
                n = np.cross(pos[1] - pos[0], pos[2] - pos[0])
                ln = np.linalg.norm(n)
                if ln == 0:
                    continue
                facing_up = n[1] / ln >= UP
                up = facing_up and pos[:, 1].mean() >= ymin + HIGH * (ymax - ymin)
                awn = OVERRIDES.get(mat['tex'], {}).get('awnings')
                if facing_up and not up and awn == 'roof':
                    up = True
                elif facing_up and not up and n[1] / ln < 0.9 and awn != 'wall':
                    continue   # low sloped faces (awnings, porch roofs) are neither: they may share the roof's texels
                               # (flat low ones, ledges and rims, count as wall)
                param = owner[mat['tex']][0][2]
                w, h = 8 << ((param >> 20) & 7), 8 << ((param >> 23) & 7)
                key = (mat['tex'], mat['pal'])
                masks = roof if up else wall
                if key not in masks:
                    masks[key] = np.zeros((h, w), bool)
                rasterise(masks[key], np.asarray(uv, float), mat['repeat'], mat['flip'])
    # texels a wall also shows aren't roof: recolouring them would paint those too (the gym's orange ribs
    # run from the top to the ground on one strip of texture; a Pokemon Center's grey rim has its side on the same texels)
    for key, mask in roof.items():
        wm = wall[key] if key in wall and wall[key].shape == mask.shape else np.zeros_like(mask)
        mask &= ~wm
        mask |= closed_holes(mask)
        ov = OVERRIDES.get(key[0], {})
        img = next((ts.image(key[0], key[1]) for si, ts, param in owner[key[0]] if key[1] in ts.palettes), None)
        if img is not None and img.shape[:2] == mask.shape:
            mask |= grown(mask, wm, img, ov.get('grow', 2))
            if ov.get('gaps', True):
                mask |= small_gaps(mask, img)
    out = defaultdict(dict)
    previews = []
    for (tn, pn), mask in roof.items():
        if not pn or not mask.any():
            continue
        for si, ts, param in owner[tn]:
            if pn not in ts.palettes:
                continue
            img = ts.image(tn, pn)
            if img is None or img.shape[:2] != mask.shape:
                continue
            pal = ts.palettes[pn]
            ncol = {2: 4, 3: 16, 4: 256}.get((param >> 26) & 7)
            if not ncol:
                continue
            cols = [ts.color(pal, k) for k in range(ncol)]
            rgb = img[..., :3].reshape(-1, 3)
            m = mask.reshape(-1) & (img[..., 3].reshape(-1) > 0)
            idx = []
            for k, c in enumerate(cols):
                same = np.all(rgb == c, axis=1)
                total = same.sum()
                if total and (same & m).sum() / total >= SHARE:
                    idx.append(k)
            if idx:
                out[str(si)][pn] = sorted(set(out[str(si)].get(pn, []) + idx))
                if len(sys.argv) > 3 and si == 0:
                    previews.append((tn, img, mask))
    json.dump(out, open(sys.argv[2], 'w'), indent=1, sort_keys=True)
    masks = {}
    for (tn, pn), mask in roof.items():
        if not mask.any():
            continue
        e = masks.setdefault(tn, {'w': mask.shape[1], 'h': mask.shape[0], 'pals': [], 'm': np.zeros_like(mask)})
        if e['m'].shape == mask.shape:
            e['m'] |= mask
            if pn and pn not in e['pals']:
                e['pals'].append(pn)
    for e in masks.values():
        e['bits'] = base64.b64encode(zlib.compress(np.packbits(e.pop('m')).tobytes(), 9)).decode()
    masks['_pairs'] = {p: sorted(t) for p, t in sorted(pairs.items())}   # every palette and the textures shown with it
    json.dump(masks, open(os.path.join(os.path.dirname(os.path.abspath(sys.argv[2])), 'roof_masks.json'), 'w'),
              sort_keys=True, separators=(',', ':'))
    print('roof colours found in %d building texture sets, %d palettes' % (len(out), sum(len(v) for v in out.values())))
    if len(sys.argv) > 3:
        from PIL import Image
        S = 96
        sheet = Image.new('RGBA', (S * 2 * 6, max(1, S * ((len(previews) + 5) // 6))), (40, 40, 40, 255))
        for i, (tn, img, mask) in enumerate(previews):
            a = Image.fromarray(img, 'RGBA')
            b = img.copy()
            b[mask] = (b[mask] * 0.3 + np.array([255, 0, 0, 255]) * 0.7).astype(np.uint8)
            b = Image.fromarray(b, 'RGBA')
            x, y = (i % 6) * 2 * S, (i // 6) * S
            sheet.alpha_composite(a.resize((S - 4, S - 4), Image.NEAREST), (x, y))
            sheet.alpha_composite(b.resize((S - 4, S - 4), Image.NEAREST), (x + S, y))
        sheet.save(sys.argv[3])


main()
