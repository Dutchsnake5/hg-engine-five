"""Hand-painted winter building textures (SEASONAL_MAPS), on top of the automatic snow:
    season_roofpaint.py export <rom.nds> <building set> <texture> [palette] [out dir]
    season_roofpaint.py check  <rom.nds> [painting dir]
    season_roofpaint.py install <from dir>            (copies finished paintings into data/seasons/roofs/)

A painting is a PNG of one texture (with one palette) at its real size, named <set>_<texture>_<palette>.png
(e.g. 000_wk_labo_a_wk_labo_a_pl.png). `export` writes the automatic winter version to start from, plus
..._summer.png next to it for reference. The build (scripts/build_seasons.py) and the Season Preview apply every
painting in data/seasons/roofs/ to the winter building texture set after the automatic snow:
- colours already in the texture's palette are used as they are;
- a new colour takes a palette entry no texture shown with that palette uses (after the painting), so other
  buildings sharing the palette never change; too many new colours is an error (`check` lists them);
- fully transparent pixels become colour 0 when the texture has a see-through colour 0.
Default painting folder for Drew: C:\\Users\\Drew\\Desktop\\HG ROM Hack Project\\Seasons\\roofs.

Map textures (ground, trees, flowers, water edges) can be painted the same way, per season: data/seasons/maptex/<season>/
<map texture set>_<texture>_<palette>.png (season = spring, autumn or winter), applied to that season's seasonal map
texture member after the automatic recolour, the flower sheet and the autumn trees. Which textures share a palette
comes from data/seasons/map_pairs.json (`season_roofpaint.py mappairs <DSPRE unpacked/maps dir>` rewrites it).
    season_roofpaint.py mapcheck <rom.nds> [maptex dir]"""
import os
import re
import shutil
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import season_recolor as R  # noqa: E402

REPO_DIR = os.path.join(HERE, '..', 'data', 'seasons', 'roofs')
DESKTOP_DIR = '/mnt/c/Users/Drew/Desktop/HG ROM Hack Project/Seasons/roofs'
MAP_REPO_DIR = os.path.join(HERE, '..', 'data', 'seasons', 'maptex')
MAP_DESKTOP_DIR = '/mnt/c/Users/Drew/Desktop/HG ROM Hack Project/Seasons/maptex'
MAP_PAIRS = os.path.join(HERE, '..', 'data', 'seasons', 'map_pairs.json')
MAP_SEASONS = ('spring', 'autumn', 'winter')


def file_name(bset, tex, pal):
    return '%03d_%s_%s.png' % (bset, tex, pal)


def parse_name(name):
    """(set, texture, palette) from a painting's file name, or None (the summer references don't count)."""
    if name.endswith('_summer.png'):
        return None
    m = re.match(r'^(\d{3})_(.+)\.png$', name)
    if not m:
        return None
    return int(m.group(1)), m.group(2)


def _palettes(data):
    out = {}
    for off, n, name in R.palettes(data):
        out[name] = (off, n)
    return out


def _bgr(rgb):
    r, g, b = rgb
    return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)


def texture_rgba(data, tex, pal):
    """(w, h, [(r, g, b, a)]) of a texture with a palette, decoded from the texel indices (palette formats only)."""
    info = {n: (p, o) for n, p, o in R.textures(data)}
    param, off = info[tex]
    fmt = (param >> 26) & 7
    w, h = 8 << ((param >> 20) & 7), 8 << ((param >> 23) & 7)
    if fmt not in R._INDEXED:
        raise ValueError('%s: texture format %d has no palette indices' % (tex, fmt))
    poff, n = _palettes(data)[pal]
    cols = [struct.unpack_from('<H', data, poff + 2 * k)[0] for k in range(n)]
    bpp, imask = R._INDEXED[fmt]
    get, _ = R._texel_io(bytearray(data), off, bpp, imask)
    tr0 = (param >> 29) & 1
    px = []
    for i in range(w * h):
        k = get(i)
        if tr0 and k == 0:
            px.append((0, 0, 0, 0))
        else:
            c = cols[k] if k < n else 0
            px.append(((c & 31) * 255 // 31, ((c >> 5) & 31) * 255 // 31, ((c >> 10) & 31) * 255 // 31, 255))
    return w, h, px


def save_png(path, w, h, px):
    from PIL import Image
    im = Image.new('RGBA', (w, h))
    im.putdata(px)
    im.save(path)


def apply_painting(data, path, pairs):
    """data (bytearray, a building texture set) with one painting applied; returns a list of problems (empty = ok).
    pairs = {palette: [textures shown with it]} (roof_masks.json '_pairs')."""
    from PIL import Image
    m = re.match(r'^\d{3}_(.+)\.png$', os.path.basename(path))
    pals = _palettes(data)
    info = {n: (p, o) for n, p, o in R.textures(data)}
    # texture and palette: the longest texture name that fits, the rest is the palette
    stem = m.group(1)
    tex = pal = None
    for t in sorted(info, key=len, reverse=True):
        if stem.startswith(t + '_') and stem[len(t) + 1:] in pals:
            tex, pal = t, stem[len(t) + 1:]
            break
    if tex is None:
        return ['%s: no texture + palette in this set matches the file name' % os.path.basename(path)]
    param, off = info[tex]
    fmt = (param >> 26) & 7
    w, h = 8 << ((param >> 20) & 7), 8 << ((param >> 23) & 7)
    if fmt not in R._INDEXED:
        return ['%s: texture format %d can\'t be painted' % (tex, fmt)]
    im = Image.open(path).convert('RGBA')
    if im.size != (w, h):
        return ['%s: the painting is %dx%d, the texture is %dx%d' % (os.path.basename(path), im.width, im.height, w, h)]
    poff, n = pals[pal]
    bpp, imask = R._INDEXED[fmt]
    reach = min(n, imask + 1)
    cols = [struct.unpack_from('<H', data, poff + 2 * k)[0] for k in range(n)]
    tr0 = (param >> 29) & 1
    want = []
    for r, g, b, a in getattr(im, "get_flattened_data", im.getdata)():
        want.append(None if a < 128 else _bgr((r, g, b)))
    if any(x is None for x in want) and not tr0:
        return ['%s: has see-through pixels but the texture has no see-through colour' % tex]
    # entries the other textures shown with this palette use
    used = set()
    for t in pairs.get(pal, []):
        if t == tex or t not in info:
            continue
        p2, o2 = info[t]
        f2 = (p2 >> 26) & 7
        if f2 not in R._INDEXED:
            continue
        b2, m2 = R._INDEXED[f2]
        reach = min(reach, m2 + 1)
        get, _ = R._texel_io(data, o2, b2, m2)
        used |= {get(i) for i in range((8 << ((p2 >> 20) & 7)) * (8 << ((p2 >> 23) & 7)))}
    index = {}
    for k in range(reach):
        if not (tr0 and k == 0):
            index.setdefault(cols[k] & 0x7FFF, k)
    new = sorted({c for c in want if c is not None and c not in index})
    keepers = set(used) | {index[c] for c in want if c is not None and c in index}
    free = [k for k in range(1 if tr0 else 0, reach) if k not in keepers]
    if len(new) > len(free):
        return ['%s: %d new colours but only %d free palette entries (%s); reuse colours already in the palette'
                % (tex, len(new), len(free), ', '.join('%02x%02x%02x' % ((c & 31) * 8, ((c >> 5) & 31) * 8, ((c >> 10) & 31) * 8) for c in new[:8]))]
    for c, k in zip(new, free):
        struct.pack_into('<H', data, poff + 2 * k, c)
        index[c] = k
    _, put = R._texel_io(data, off, bpp, imask)
    for i, c in enumerate(want):
        put(i, 0 if c is None else index[c])
    return []


def apply_all(data, bset, pairs, folder=None):
    """A building texture set's winter member with every painting for it applied: (bytes, problems)."""
    folder = folder or REPO_DIR
    out = bytearray(data)
    problems = []
    if os.path.isdir(folder):
        for f in sorted(os.listdir(folder)):
            p = parse_name(f)
            if p and p[0] == bset:
                problems += apply_painting(out, os.path.join(folder, f), pairs)
    return bytes(out), problems


def model_pairs(bmd):
    """{(texture, palette)} that the materials of a map model (BMD0 bytes) draw with."""
    import season_trees as T
    d = bytearray(bmd)
    if d[:4] != b'BMD0':
        return set()
    mdl0 = struct.unpack_from('<I', d, 0x10)[0]
    m = mdl0 + struct.unpack_from('<I', d, T._dict(d, mdl0 + 8)[0][1])[0]
    base = m + struct.unpack_from('<I', d, m + 8)[0]
    ofs_tex, ofs_pal = struct.unpack_from('<HH', d, base)
    by_mat = {}
    for which, ofs in ((0, ofs_tex), (1, ofs_pal)):
        for name, e, _ in T._dict(d, base + ofs):
            lst, num = struct.unpack_from('<HB', d, e)
            for mat in d[base + lst:base + lst + num]:
                by_mat.setdefault(mat, [None, None])[which] = name.decode('latin1')
    return {(t, p) for t, p in by_mat.values() if t and p}


def load_map_pairs(path=None):
    """{palette: [textures drawn with it on any map]} (data/seasons/map_pairs.json)."""
    import json
    path = path or MAP_PAIRS
    return json.load(open(path)) if os.path.exists(path) else {}


def apply_map_all(data, mset, season, pairs, folder=None):
    """A seasonal map texture member (season 'spring' / 'autumn' / 'winter') with every painting for map texture set
    mset applied: (bytes, problems)."""
    return apply_all(data, mset, pairs, os.path.join(folder or MAP_REPO_DIR, season))


def main():
    cmd = sys.argv[1]
    if cmd == 'mappairs':
        import json
        sys.path.insert(0, os.environ.get('SCRIPT_INSPECTOR', '/mnt/c/Users/Drew/Desktop/HG ROM Hack Project/ScriptInspector'))
        import maprender as M
        pairs = {}
        folder = sys.argv[2]
        for f in sorted(os.listdir(folder)):
            if f.isdigit():
                _, _, mdl = M.split_map_file(open(os.path.join(folder, f), 'rb').read())
                for t, p in model_pairs(mdl):
                    pairs.setdefault(p, set()).add(t)
        json.dump({p: sorted(ts) for p, ts in sorted(pairs.items())}, open(MAP_PAIRS, 'w'), indent=0)
        print('%d palettes -> %s' % (len(pairs), MAP_PAIRS))
        return
    if cmd == 'install':
        src = sys.argv[2]
        os.makedirs(REPO_DIR, exist_ok=True)
        for f in sorted(os.listdir(src)):
            if parse_name(f):
                shutil.copy(os.path.join(src, f), os.path.join(REPO_DIR, f))
                print('installed', f)
        return
    import json
    import ndspy.narc
    import ndspy.rom
    rom = ndspy.rom.NintendoDSRom.fromFile(sys.argv[2])
    bnarc = ndspy.narc.NARC(rom.getFileByName('a/0/7/0')).files
    masks = R.load_masks(os.path.join(HERE, '..', 'data', 'seasons', 'roof_masks.json'))
    if cmd == 'export':
        bset, tex = int(sys.argv[3]), sys.argv[4]
        pal = sys.argv[5] if len(sys.argv) > 5 else None
        out = sys.argv[6] if len(sys.argv) > 6 else DESKTOP_DIR
        base = bnarc[bset]
        pal = pal or next((p for p, ts in masks['_pairs'].items() if tex in ts and p in _palettes(base)), None)
        winter = R.recolor_btx(base, R.WINTER, 'building', None, masks)
        winter, _ = apply_all(winter, bset, masks['_pairs'])
        os.makedirs(out, exist_ok=True)
        path = os.path.join(out, file_name(bset, tex, pal))
        save_png(path, *texture_rgba(winter, tex, pal))
        save_png(path[:-4] + '_summer.png', *texture_rgba(base, tex, pal))
        print(path)
    elif cmd == 'mapcheck':
        import re as _re
        import season_trees as T
        folder = sys.argv[3] if len(sys.argv) > 3 else MAP_REPO_DIR
        narc = ndspy.narc.NARC(rom.getFileByName('a/0/4/4')).files
        src = open(os.path.join(HERE, '..', 'src', 'seasons.c'), encoding='utf-8').read()
        body = _re.search(r'sSeasonalMapTexSets\[\] = \{(.*?)\};', src, _re.S).group(1)
        seasonal = [int(x) for x in _re.findall(r'\d+', _re.sub(r'//[^\n]*', '', body))]
        pairs = load_map_pairs()
        bad = 0
        for season in MAP_SEASONS:
            sdir = os.path.join(folder, season)
            if not os.path.isdir(sdir):
                continue
            code = {'spring': R.SPRING, 'autumn': R.AUTUMN, 'winter': R.WINTER}[season]
            for s in sorted({parse_name(f)[0] for f in os.listdir(sdir) if parse_name(f)}):
                if s not in seasonal:
                    print('%s: set %d is not seasonal (sSeasonalMapTexSets)' % (season, s))
                    bad += 1
                    continue
                d = R.recolor_btx(narc[s], code, 'map')
                if code == R.AUTUMN:
                    d = T.autumn_trees(narc[s], d)
                _, problems = apply_map_all(d, s, season, pairs, folder)
                for p in problems:
                    print('%s set %d: %s' % (season, s, p))
                    bad += 1
        print('%d problem(s)' % bad)
        sys.exit(1 if bad else 0)
    elif cmd == 'check':
        folder = sys.argv[3] if len(sys.argv) > 3 else REPO_DIR
        sets = sorted({parse_name(f)[0] for f in os.listdir(folder) if parse_name(f)})
        bad = 0
        for s in sets:
            winter = R.recolor_btx(bnarc[s], R.WINTER, 'building', None, masks)
            _, problems = apply_all(winter, s, masks['_pairs'], folder)
            for p in problems:
                print('set %d: %s' % (s, p))
                bad += 1
        print('%d problem(s)' % bad)
        sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
