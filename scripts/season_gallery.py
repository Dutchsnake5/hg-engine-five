"""Top-down previews of outdoor areas in every season (SEASONAL_MAPS), without the emulator:
    season_gallery.py <out dir> [texture set ...]

Renders each map header's chunks with the Script Inspector's renderer (maprender.py) from the DSPRE project, swapping
in the seasonal map and building textures exactly as scripts/build_seasons.py makes them (incl. the hand-edited
flower sheet cells) for the areas src/seasons.c makes seasonal; other areas stay as they are, as in the game. The
game's winter lighting boost isn't modelled, so winter looks a little greyer than in game. The Season Preview tool
(HG ROM Hack Project\\Season Preview) uses the same functions."""
import io
import json
import os
import re
import sys
import time

import ndspy.code
import ndspy.rom
import numpy as np
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
sys.path.insert(0, HERE)
sys.path.insert(0, os.environ.get('SCRIPT_INSPECTOR', '/mnt/c/Users/Drew/Desktop/HG ROM Hack Project/ScriptInspector'))
import maprender as M  # noqa: E402
import season_recolor as R  # noqa: E402

PROJECT = os.environ.get('DSPRE_PROJECT', '/mnt/c/Users/Drew/Desktop/HG ROM Hack Project/DSPRE/rom_DSPRE_contents')
SEASONS = [('spring', R.SPRING), ('summer', R.SUMMER), ('autumn', R.AUTUMN), ('winter', R.WINTER)]
SEASON_OF = dict(SEASONS)
ROOFS = json.load(open(os.path.join(REPO, 'data', 'seasons', 'roof_colors.json')))
MASKS = R.load_masks(os.path.join(REPO, 'data', 'seasons', 'roof_masks.json'))
SHEET = {}   # (kind folder, set, season) -> member with the hand-edited flower sheet applied


def seasonal_sets():
    """The map texture sets src/seasons.c makes seasonal (sSeasonalMapTexSets)."""
    src = open(os.path.join(REPO, 'src', 'seasons.c'), encoding='utf-8').read()
    body = re.search(r'sSeasonalMapTexSets\[\] = \{(.*?)\};', src, re.S).group(1)
    return [int(x) for x in re.findall(r'\d+', re.sub(r'//[^\n]*', '', body))]


def seasonal_texset(kind, n, data, season):
    """A map or building texture set as the game loads it in a seasonal area."""
    if season == R.SUMMER:
        return data
    if kind == 'mapTextures':
        d = R.recolor_btx(data, season, 'map')
    elif season == R.SPRING:
        return data
    else:
        d = R.recolor_btx(data, season, 'building', ROOFS.get(str(n), {}), MASKS)
        if season == R.WINTER and d[:4] == b'BTX0':
            import season_roofpaint as RP
            d, _ = RP.apply_all(d, n, MASKS['_pairs'])   # hand-painted roofs (data/seasons/roofs/)
    return SHEET.get((kind, n, season), d)


class SeasonRenderer(M.Renderer):
    def __init__(self, project, season, seasonal=None):
        super().__init__(project)
        self.season = season
        self.seasonal = set(seasonal_sets() if seasonal is None else seasonal)
        self.active = True

    def texset(self, kind, n):
        key = (kind, n, self.active)
        if key not in self._texsets:
            try:
                d = self.file(kind, '%04d' % n)
            except OSError:
                self._texsets[key] = []
                return []
            self._texsets[key] = M.load_texsets(seasonal_texset(kind, n, d, self.season) if self.active else d)
        return self._texsets[key]

    def render_map(self, map_id, area_id):
        """maprender's, except that buildings take textures from the area's building set before their own embedded
        copies, as the game does (the seasonal roofs live in the set), and that only seasonal areas change."""
        ad = self.file('areaData', '%04d' % area_id)
        bld_set, map_set = M.u16(ad, 0), M.u16(ad, 2)
        self.active = map_set in self.seasonal and len(ad) > 6 and ad[6] != 0
        _, blds, mdl = M.split_map_file(self.file('maps', '%04d' % map_id))
        canvas = M.Canvas()
        jobs = []
        model, own = M.load_model(mdl)
        if model:
            jobs.append((model.triangles(), model.materials, self.texset('mapTextures', map_set) + own))
        for e in blds:
            mid, mat = M.building_placement(e)
            bm, btex = self.building(False, mid)
            if bm is None:
                continue
            tris = [((np.c_[p, np.ones(3)] @ mat.T)[:, :3], uv, col, m) for p, uv, col, m in bm.triangles()]
            jobs.append((tris, bm.materials, self.texset('buildingTextures', bld_set) + btex))
        for translucent in (False, True):
            for tris, mats, ts in jobs:
                canvas.draw(tris, mats, ts, translucent)
        return canvas


def load_sheet():
    """Seasonal members with the hand-edited flower sheet applied, as build_seasons.py writes them."""
    SHEET.clear()
    sheet = os.path.join(REPO, 'data', 'seasons', 'flower_sheet.png')
    if not os.path.exists(sheet):
        return
    import season_sheet as SH
    js = json.load(open(sheet[:-4] + '.json'))
    wanted = {(c['kind'], c['set']) for c in js['cells']}
    orig = {'map': {}, 'building': {}}
    out = {'map': {}, 'building': {}}
    idx = {}
    for kind, s in wanted:
        folder = 'mapTextures' if kind == 'map' else 'buildingTextures'
        d = open(os.path.join(PROJECT, 'unpacked', folder, '%04d' % s), 'rb').read()
        orig[kind][s] = d
        for name, season in SEASONS:
            if season == R.SUMMER or (kind == 'building' and season == R.SPRING):
                continue
            idx[(kind, s, name)] = len(idx) + 100000
            out[kind][idx[(kind, s, name)]] = R.recolor_btx(d, season, kind, ROOFS.get(str(s), {}) if kind == 'building' else None,
                                                           MASKS if kind == 'building' else None)
    SH.apply_sheet(out, sheet, sheet[:-4] + '.json', orig, ROOFS, lambda kind, s, season: idx.get((kind, s, season)))
    for (kind, s, name), i in idx.items():
        SHEET[('mapTextures' if kind == 'map' else 'buildingTextures', s, SEASON_OF[name])] = out[kind][i]


def load_arm9():
    rom = ndspy.rom.NintendoDSRom.fromFile(os.path.join(REPO, 'rom.nds'))
    return ndspy.code.MainCodeFile(rom.arm9, 0x02000000, rom.arm9PostData).save(compress=False)


def outdoor_headers(arm9):
    """[(header, internal name, area, map texture set)] of every outdoor map header that has map chunks."""
    names = open(os.path.join(PROJECT, 'files', 'fielddata', 'maptable', 'mapname.bin'), 'rb').read()
    out = []
    for h in range(540):
        area = arm9[0xF6BE0 + 24 * h + 1]
        ad = open(os.path.join(PROJECT, 'unpacked', 'areaData', '%04d' % area), 'rb').read()
        if ad[6] and M.u16(ad, 2) != 0:   # set 0: the placeholder headers (EVERYWHERE, NOTHING, UG)
            out.append((h, names[16 * h:16 * h + 16].split(b'\0')[0].decode('latin1'), area, M.u16(ad, 2)))
    return out


def area_image(renderer, arm9, header):
    """The whole area of a map header, stitched from its chunks (None if it has none)."""
    area, mat, m, cells = M.header_chunks(PROJECT, arm9, header)
    if not cells:
        return None
    c0, r0 = min(c for c, r, _ in cells), min(r for c, r, _ in cells)
    w = max(c for c, r, _ in cells) - c0 + 1
    h = max(r for c, r, _ in cells) - r0 + 1
    img = Image.new('RGBA', (512 * w, 512 * h), (0, 0, 0, 255))
    for c, r, mp in cells:
        png = renderer.render_map(mp, area).png()
        img.alpha_composite(Image.open(io.BytesIO(png)).convert('RGBA'), ((c - c0) * 512, (r - r0) * 512))
    return img


def texture_sheet(data_by_season, title):
    """One picture of a texture set: a column per season, every texture (with its palette by name) in the same place."""
    sets = {s: M.load_texsets(d)[0] for s, d in data_by_season.items()}
    first = next(iter(sets.values()))
    items = []
    for tex in sorted(first.textures):
        pals = [tex + '_pl', tex, tex.split('.')[0] + '_pl']
        pal = next((p for p in pals if p in first.palettes), next(iter(first.palettes), None))
        img = first.image(tex, pal)
        if img is not None and pal is not None:
            items.append((tex, pal, img.shape[1], img.shape[0]))
    colw, x, y, shelf, pos = 256, 0, 0, 0, []
    for tex, pal, w, h in items:
        w2, h2 = min(w, colw), h * min(w, colw) // w
        if x + w2 > colw:
            x, y, shelf = 0, y + shelf + 2, 0
        pos.append((x, y, w2, h2))
        x, shelf = x + w2 + 2, max(shelf, h2)
    gap, top = 12, 16
    out = Image.new('RGBA', (len(sets) * (colw + gap), y + shelf + top), (32, 32, 32, 255))
    d = ImageDraw.Draw(out)
    names = {v: k for k, v in SEASONS}
    for i, (s, ts) in enumerate(sets.items()):
        ox = i * (colw + gap)
        d.text((ox + 2, 2), title + ' - ' + names[s], fill=(255, 255, 255))
        for (tex, pal, w, h), (x, y, w2, h2) in zip(items, pos):
            img = ts.image(tex, pal)
            if img is not None:
                out.alpha_composite(Image.fromarray(img, 'RGBA').resize((w2, h2), Image.NEAREST), (ox + x, top + y))
    return out


def texture_set_sheet(kind, n):
    """kind 'map' (all four seasons) or 'building' (summer, autumn, winter); None for an empty member."""
    folder = 'mapTextures' if kind == 'map' else 'buildingTextures'
    base = open(os.path.join(PROJECT, 'unpacked', folder, '%04d' % n), 'rb').read()
    if base[:4] != b'BTX0':
        return None
    seasons = [s for _, s in SEASONS] if kind == 'map' else [R.SUMMER, R.AUTUMN, R.WINTER]
    return texture_sheet({s: seasonal_texset(folder, n, base, s) for s in seasons}, '%s set %d' % (kind, n))


def main():
    out_dir = sys.argv[1]
    only = {int(x) for x in sys.argv[2:]}
    os.makedirs(out_dir, exist_ok=True)
    load_sheet()
    arm9 = load_arm9()
    renderers = {s: SeasonRenderer(PROJECT, s) for _, s in SEASONS}
    index = []
    for h, name, area, tset in sorted(outdoor_headers(arm9), key=lambda e: (e[3], e[0])):
        if only and tset not in only:
            continue
        t = time.time()
        for sname, s in SEASONS:
            img = area_image(renderers[s], arm9, h)
            if img is None:
                break
            img.save(os.path.join(out_dir, '%03d_%s_%s.png' % (tset, name, sname)))
        else:
            index.append(dict(set=tset, header=h, name=name))
            print(tset, name, '%.1fs' % (time.time() - t), flush=True)
    json.dump(index, open(os.path.join(out_dir, 'index.json'), 'w'), indent=1)


if __name__ == '__main__':
    main()
