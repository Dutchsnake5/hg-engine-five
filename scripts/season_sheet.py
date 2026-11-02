"""Seasonal flower textures as one editable sheet (SEASONAL_MAPS):
    season_sheet.py export <rom.nds> <sheet.png> <layout.json>      write the sheet from the automatic recolour
    season_sheet.py recolor <rom.nds> <sheet.png> <layout.json>     hand-edited non-summer cells (those that differ from
                                                                    the automatic version) get their season's recolour
                                                                    (the sheet is backed up as …_before_recolor.png)
    season_sheet.py install <sheet.png> <layout.json>               copy the sheet into data/seasons/ for the build
scripts/build_seasons.py then replaces every non-summer cell that differs from the automatic version with the sheet's
pixels (apply_sheet below).

Rows = textures (map texture set 2, building texture set 0), columns = spring, summer (original), autumn, winter, each at
its real pixel size; the game's see-through colour is transparent. A 4x zoomed copy with labels is written next to the
sheet (…_preview.png). layout.json records every cell (texture, palette, set, season, x, y, size, format) so an edited
sheet can be read back (each cell must keep at most the texture's colour count: 16, or 4 for 2-colour-bit textures)."""
import json
import os
import sys

import ndspy.narc
import ndspy.rom
from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.environ.get('SCRIPT_INSPECTOR', '/mnt/c/Users/Drew/Desktop/HG ROM Hack Project/ScriptInspector'))
import maprender as M  # noqa: E402
import season_recolor as R  # noqa: E402

SEASONS = [('spring', R.SPRING), ('summer', R.SUMMER), ('autumn', R.AUTUMN), ('winter', R.WINTER)]
ROWS = [('map', 2, 'yo_sp1', 'yo_sp1_pl', 'flower bed'), ('map', 2, 'flower01', 'flower01', 'grass flowers 1'),
        ('map', 2, 'flower02', 'flower02', 'grass flowers 2'), ('map', 2, 'flower03', 'flower03', 'grass flowers 3'),
        ('building', 0, 'yo_sp1', 'yo_sp1_pl', 'window box flowers'), ('building', 0, 'yo_sp2', 'yo_sp2_pl', 'window box')]
GAP = 8


def auto_cells(arch, roofs):
    """The automatic (scripts/season_recolor.py) version of every cell, with its image, plus column width/row heights."""
    cells, colw, rowh = [], 0, []
    for kind, s, tex, pal, label in ROWS:
        base = arch[kind][s]
        h = 0
        for name, season in SEASONS:
            d = base if season == R.SUMMER else R.recolor_btx(base, season, kind, roofs.get(str(s), {}) if kind == 'building' else None)
            ts = M.load_texsets(d)[0]
            param = ts.textures[tex][0]
            img = ts.image(tex, pal)
            cells.append(dict(kind=kind, set=s, texture=tex, palette=pal, label=label, season=name,
                              w=img.shape[1], h=img.shape[0], format=(param >> 26) & 7, transparent0=(param >> 29) & 1, img=img))
            colw, h = max(colw, img.shape[1]), max(h, img.shape[0])
        rowh.append(h)
    return cells, colw, rowh


def load(rom_path):
    rom = ndspy.rom.NintendoDSRom.fromFile(rom_path)
    arch = {'map': ndspy.narc.NARC(rom.getFileByName('a/0/4/4')).files,
            'building': ndspy.narc.NARC(rom.getFileByName('a/0/7/0')).files}
    roofs = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'data', 'seasons', 'roof_colors.json')))
    return arch, roofs


def edited(sheet, cell, auto_img):
    """The sheet's pixels for a cell if they differ from the automatic version (transparent pixels compared as such)."""
    import numpy as np
    px = sheet[cell['y']:cell['y'] + cell['h'], cell['x']:cell['x'] + cell['w']]
    a, b = px.copy(), auto_img.copy()
    a[a[..., 3] == 0] = 0
    b[b[..., 3] == 0] = 0
    return None if np.array_equal(a, b) else px


def main():
    _, rom_path, sheet_path, layout_path = sys.argv[1:5]
    arch, roofs = load(rom_path)
    cells, colw, rowh = auto_cells(arch, roofs)
    W = GAP + 4 * (colw + GAP)
    H = GAP + sum(h + GAP for h in rowh)
    sheet = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    y = GAP
    for r in range(len(ROWS)):
        for c in range(4):
            cell = cells[r * 4 + c]
            x = GAP + c * (colw + GAP)
            sheet.paste(Image.fromarray(cell.pop('img'), 'RGBA'), (x, y))
            cell.update(x=x, y=y)
        y += rowh[r] + GAP
    sheet.save(sheet_path)
    json.dump({'sheet': os.path.basename(sheet_path), 'seasons': [n for n, _ in SEASONS], 'cells': cells}, open(layout_path, 'w'), indent=1)
    # labelled, zoomed preview (not for editing)
    Z, L = 4, 150
    prev = Image.new('RGBA', (L + W * Z, 24 + H * Z), (48, 48, 48, 255))
    prev.alpha_composite(sheet.resize((W * Z, H * Z), Image.NEAREST), (L, 24))
    dr = ImageDraw.Draw(prev)
    for c, (name, _) in enumerate(SEASONS):
        dr.text((L + (GAP + c * (colw + GAP)) * Z, 6), name, fill=(255, 255, 255, 255))
    y = GAP
    for r, (kind, s, tex, pal, label) in enumerate(ROWS):
        dr.text((6, 24 + y * Z + 4), '%s\n%s (%s set %d)' % (label, tex, kind, s), fill=(255, 255, 160, 255))
        y += rowh[r] + GAP
    prev.save(os.path.splitext(sheet_path)[0] + '_preview.png')
    print('wrote', sheet_path, 'and', layout_path, '(%d cells)' % len(cells))


def recolor_cmd():
    import numpy as np
    import shutil
    _, rom_path, sheet_path, layout_path = sys.argv[1:5]
    arch, roofs = load(rom_path)
    cells, _, _ = auto_cells(arch, roofs)
    layout = json.load(open(layout_path))['cells']
    sheet = np.array(Image.open(sheet_path).convert('RGBA'))
    shutil.copy(sheet_path, os.path.splitext(sheet_path)[0] + '_before_recolor.png')
    season_of = dict(SEASONS)
    for cell, lay in zip(cells, layout):
        if cell['season'] == 'summer':
            continue
        px = edited(sheet, lay, cell['img'])
        if px is None:
            continue
        season = season_of[cell['season']]
        out = px.copy()
        for y in range(px.shape[0]):
            for x in range(px.shape[1]):
                r, g, b, a = (int(v) for v in px[y, x])
                if not a:
                    continue
                c = R.recolor((r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10), season)
                out[y, x, :3] = [(c & 31) * 255 // 31, ((c >> 5) & 31) * 255 // 31, ((c >> 10) & 31) * 255 // 31]
        sheet[lay['y']:lay['y'] + lay['h'], lay['x']:lay['x'] + lay['w']] = out
        print('recoloured', cell['kind'], cell['texture'], cell['season'])
    Image.fromarray(sheet, 'RGBA').save(sheet_path)


def install_cmd():
    import shutil
    dest = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'data', 'seasons')
    shutil.copy(sys.argv[2], os.path.join(dest, 'flower_sheet.png'))
    shutil.copy(sys.argv[3], os.path.join(dest, 'flower_sheet.json'))
    print('installed into data/seasons/')


def apply_sheet(arch_out, sheet_path, layout_path, arch, roofs, member_of):
    """Called by build_seasons.py: writes edited cells into the seasonal members. member_of(kind, set, season name) =
    the member index in arch_out[kind] (a list of file bytes) holding that set's version for that season, or None."""
    import numpy as np
    import struct
    cells, _, _ = auto_cells(arch, roofs)
    layout = json.load(open(layout_path))['cells']
    sheet = np.array(Image.open(sheet_path).convert('RGBA'))
    for cell, lay in zip(cells, layout):
        if cell['season'] == 'summer':
            continue
        px = edited(sheet, lay, cell['img'])
        m = member_of(cell['kind'], cell['set'], cell['season'])
        if px is None or m is None:
            continue
        data = bytearray(arch_out[cell['kind']][m])
        ts = M.load_texsets(bytes(data))[0]
        param = ts.textures[cell['texture']][0]
        fmt, ncol = (param >> 26) & 7, {2: 4, 3: 16}.get((param >> 26) & 7)
        if not ncol:
            sys.exit('season_sheet.py: %s has texture format %d, which the sheet can\'t write' % (cell['texture'], fmt))
        transparent0 = (param >> 29) & 1
        opaque = px[..., 3] > 0
        cols = sorted({tuple(int(v) >> 3 for v in p[:3]) for p in px[opaque]})
        first = 1 if transparent0 else 0
        if len(cols) > ncol - first:
            sys.exit('season_sheet.py: %s %s uses %d colours, at most %d fit' % (cell['texture'], cell['season'], len(cols), ncol - first))
        if not transparent0 and not opaque.all():
            sys.exit('season_sheet.py: %s %s has transparent pixels, but the texture has no see-through colour' % (cell['texture'], cell['season']))
        index = {c: first + i for i, c in enumerate(cols)}
        if cols:
            pal = {name: (off, n) for off, n, name in R.palettes(bytes(data))}
            off, n = pal[cell['palette']]
            if n < first + len(cols):
                sys.exit('season_sheet.py: palette %s has only %d colours' % (cell['palette'], n))
            for c, k in index.items():
                struct.pack_into('<H', data, off + 2 * k, c[0] | (c[1] << 5) | (c[2] << 10))
        bits = {4: 2, 16: 4}[ncol]
        w, h = cell['w'], cell['h']
        texels = ts.tex_data + ((param & 0xFFFF) << 3)
        out = bytearray(w * h * bits // 8)
        for y in range(h):
            for x in range(w):
                k = index[tuple(int(v) >> 3 for v in px[y, x, :3])] if opaque[y, x] else 0
                i = y * w + x
                out[i * bits // 8] |= k << ((i * bits) % 8)
        data[texels:texels + len(out)] = out
        arch_out[cell['kind']][m] = bytes(data)
        print('sheet:', cell['kind'], cell['texture'], cell['season'], '(%d colours)' % len(cols))


if __name__ == '__main__':
    {'export': main, 'recolor': recolor_cmd, 'install': install_cmd}[sys.argv[1]]()
