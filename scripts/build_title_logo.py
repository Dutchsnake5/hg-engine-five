#!/usr/bin/env python3
"""Puts the hack's logo on the title screen and turns HeartGold's orange sunburst behind it jade green
(CUSTOM_TITLE_LOGO).

    build_title_logo.py <rom.nds> <logo png> <output narc> [<model .nsbmd> <animation .nsbca>]

With a model and animation (CUSTOM_TITLE_SCENE): the cloud sea is recoloured to a forest canopy (palettes
hou_cloud01_pl and hou_kumo03_pl in member 25), the bottom screen's Ho-Oh is hidden (its 15 shapes' draw
commands in member 25 become NOPs; the sky, clouds and their scrolling stay) and the given model + joint
animation replace the sparkle trail (members 38 and 39), which the title screen draws at the same place with
the same camera. The model and animation come from the Desktop's Title Scene tools (build_celebi.py).

The HeartGold title screen draws its logo from archive a/0/4/6 (pret: demo/title/titledemo.narc): member 3 = the
256 x 120 picture as 8bpp tiles (NCGR), member 4 = its 256-colour palette (NCLR, loaded as a sub-screen extended
palette), member 0 = the tilemap shared with SoulSilver (tiles 0-479 in order on rows 5-19). Colours 128-255 of that
palette also colour the sunburst picture behind the logo (member 34, on the normal sub-screen palette), so the logo
only has colours 1-127. The PNG must be a 256 x 120 indexed picture, colour 0 = see-through, colours 1-127 only (the
Desktop's Tools\\title_logo\\convert_logo.py makes it from the full-size artwork).

The archive is always read from the original ROM (the build overwrites base/root/a/0/4/6 with this script's output,
and the colour change below must not be applied twice). Only pixel and colour data are replaced, so the files'
headers stay the game's own.
"""
import colorsys
import struct
import sys

import ndspy.narc
import ndspy.rom
from PIL import Image

rom_path, png, out_path = sys.argv[1:4]
scene = sys.argv[4:6]
narc = ndspy.narc.NARC(ndspy.rom.NintendoDSRom.fromFile(rom_path).getFileByName('a/0/4/6'))

im = Image.open(png)
if im.mode != 'P' or im.size != (256, 120):
    sys.exit('%s: needs a 256 x 120 indexed (palette) PNG, got %s %s' % (png, im.mode, im.size))
pix = im.tobytes()
t = im.info.get('transparency')
if isinstance(t, int) and t != 0:
    # an image editor saved the see-through colour somewhere other than slot 0: swap it into slot 0
    pal = im.getpalette() or []
    pal += [0] * (768 - len(pal))
    pal[0:3], pal[t * 3:t * 3 + 3] = pal[t * 3:t * 3 + 3], pal[0:3]
    pix = bytes(0 if v == t else t if v == 0 else v for v in pix)
    im = Image.frombytes('P', im.size, pix)
    im.putpalette(pal)
if max(pix) > 127:
    sys.exit('%s: uses colour %d; the logo may only use colours 1-127 (128-255 colour the sunburst)' % (png, max(pix)))
if any(pix[y * 256 + x] for y in range(8) for x in range(8)):
    sys.exit('%s: the top-left 8 x 8 corner must be see-through (the tilemap uses tile 0 as the empty tile)' % png)

# NCGR: 'RGCN' header, then 'RAHC' at 0x10; 8bpp, 15 x 32 tiles, data size 0x7800 at 0x30
ncgr = bytearray(narc.files[3])
assert ncgr[:4] == b'RGCN' and ncgr[0x10:0x14] == b'RAHC'
assert struct.unpack_from('<HHI', ncgr, 0x18) == (15, 32, 4) and struct.unpack_from('<I', ncgr, 0x28)[0] == 0x7800
data = bytearray()
for ty in range(15):
    for tx in range(32):
        for y in range(8):
            o = (ty * 8 + y) * 256 + tx * 8
            data += pix[o:o + 8]
ncgr[0x30:0x30 + 0x7800] = data
narc.files[3] = bytes(ncgr)

# NCLR: 'RLCN' header, 'TTLP' at 0x10, 256 BGR555 colours at 0x28
nclr = bytearray(narc.files[4])
assert nclr[:4] == b'RLCN' and nclr[0x10:0x14] == b'TTLP'
pal = im.getpalette() or []
pal += [0] * (768 - len(pal))
for i in range(1, 128):            # colour 0 is never shown: keep the game's
    r, g, b = (pal[i * 3 + k] >> 3 for k in range(3))
    struct.pack_into('<H', nclr, 0x28 + i * 2, r | (g << 5) | (b << 10))


def jade(c):
    """the sunburst's warm colours (dark red at the edges ... yellow near the glow) become teal ... jade green, a little
    less saturated; its blues and the white glow stay as they are"""
    r, g, b = ((c >> s & 31) / 31 for s in (0, 5, 10))
    h, s, v = colorsys.rgb_to_hsv(r, g, b)
    d = h * 360
    if 200 < d < 290:
        return c
    if d >= 290:
        d -= 360
    d = max(-15.0, min(60.0, d))
    h = (165 + (135 - 165) * (d + 15) / 75) / 360
    r, g, b = colorsys.hsv_to_rgb(h, s * 0.75, v)
    return round(r * 31) | round(g * 31) << 5 | round(b * 31) << 10


for i in range(128, 256):
    o = 0x28 + i * 2
    struct.pack_into('<H', nclr, o, jade(struct.unpack_from('<H', nclr, o)[0]))
narc.files[4] = bytes(nclr)

# NSCR: rows 3-4 repeat a few tiles of the picture's top-right corner above the logo (blank in HeartGold's logo,
# not in ours); point them at the empty tile 0
nscr = bytearray(narc.files[0])
assert nscr[:4] == b'RCSN'
for row in (3, 4):
    for col in range(32):
        struct.pack_into('<H', nscr, 0x24 + (row * 32 + col) * 2, 0)
narc.files[0] = bytes(nscr)



def sbc_oplen(data, p):
    """byte length of the NNS G3D render command at p"""
    cmd, flag = data[p] & 0x1F, data[p] >> 5
    if cmd in (0x00, 0x01, 0x0B):
        return 1
    if cmd == 0x02:
        return 3
    if cmd in (0x03, 0x04, 0x05):
        return 2
    if cmd == 0x06:
        return 4 + (1 if flag in (1, 2) else 2 if flag == 3 else 0)
    if cmd in (0x07, 0x08):
        return 2 + (1 if flag in (1, 2) else 2 if flag == 3 else 0)
    if cmd == 0x09:
        return 3 + 3 * data[p + 2]
    if cmd == 0x0A:
        return 9
    if cmd in (0x0C, 0x0D):
        return 3
    sys.exit('unknown render command 0x%02x' % data[p])


def hide_shapes(data, shape_ids):
    """NOPs out the SHP commands of the given shapes in the first model's render commands"""
    data = bytearray(data)
    assert data[:4] == b'BMD0'
    mdl = struct.unpack_from('<I', data, 16)[0]
    dic = mdl + 8
    m = mdl + struct.unpack_from('<I', data, dic + struct.unpack_from('<H', data, dic + 6)[0] + 4)[0]
    p, end = (m + o for o in struct.unpack_from('<II', data, m + 4))
    hidden = 0
    while p < end and data[p] & 0x1F != 0x01:
        n = sbc_oplen(data, p)
        if data[p] & 0x1F == 0x05 and data[p + 1] in shape_ids:
            data[p] = data[p + 1] = 0
            hidden += 1
        p += n
    return bytes(data), hidden


def tex0_palettes(data):
    """{palette name: (offset in the file, number of colours)} of an .nsbmd's TEX0 block"""
    nblk = struct.unpack_from('<H', data, 14)[0]
    for off in struct.unpack_from('<%dI' % nblk, data, 16):
        if data[off:off + 4] == b'TEX0':
            break
    else:
        sys.exit('no TEX0 block')
    t = off
    size_pltt, _, ofs_dict = struct.unpack_from('<HHH', data, t + 0x30)
    ofs_data = struct.unpack_from('<I', data, t + 0x38)[0]
    d = t + ofs_dict
    n = data[d + 1]
    ent = d + struct.unpack_from('<H', data, d + 6)[0]
    unit, ofs_name = struct.unpack_from('<HH', data, ent)
    items = []
    for i in range(n):
        po = struct.unpack_from('<H', data, ent + 4 + i * unit)[0] << 3
        name = data[ent + ofs_name + 16 * i: ent + ofs_name + 16 * (i + 1)].split(b'\0')[0].decode()
        items.append((po, name))
    end = size_pltt << 3
    out = {}
    for k, (po, name) in enumerate(sorted(items)):
        nxt = sorted(items)[k + 1][0] if k + 1 < len(items) else end
        out[name] = (t + ofs_data + po, (nxt - po) // 2)
    return out


FOREST = [(0.0, (1, 3, 1)), (0.4, (2, 7, 3)), (0.7, (4, 11, 4)), (1.0, (9, 17, 6))]   # deep shade .. sunlit leaves


def forest(c, lo, hi):
    """a cloud colour -> forest canopy: its brightness, stretched over the palette's own range lo..hi so the soft
    cloud shading becomes contrasty leaf clumps, picks a green from FOREST"""
    r, g, b = (c >> s_ & 31 for s_ in (0, 5, 10))
    lum = (0.3 * r + 0.59 * g + 0.11 * b) / 31
    lum = min(1.0, max(0.0, (lum - lo) / ((hi - lo) or 1)))
    for (a, ca), (b_, cb) in zip(FOREST, FOREST[1:]):
        if lum <= b_:
            f = (lum - a) / (b_ - a)
            rr, gg, bb = (round(ca[i] + (cb[i] - ca[i]) * f) for i in range(3))
            return rr | gg << 5 | bb << 10
    rr, gg, bb = FOREST[-1][1]
    return rr | gg << 5 | bb << 10


def luminance(c):
    return (0.3 * (c & 31) + 0.59 * (c >> 5 & 31) + 0.11 * (c >> 10 & 31)) / 31


if scene:
    HOOH_SHAPES = set(range(5, 20))     # polygon5-19 = Ho-Oh; 0-4 = clouds, starry band, sky dome
    narc.files[25], n = hide_shapes(narc.files[25], HOOH_SHAPES)
    if n != 15:
        sys.exit('member 25: expected Ho-Oh\'s 15 draw commands, found %d' % n)
    # the cloud sea (hou_cloud01) and the wisps drifting over it (hou_kumo03) become a forest canopy seen from above;
    # the orange streaks in the sky (hou_cloud02) and the sky stay
    m25 = bytearray(narc.files[25])
    pals = tex0_palettes(m25)
    for name in ('hou_cloud01_pl', 'hou_kumo03_pl'):
        if name not in pals:
            sys.exit('member 25: no palette %s' % name)
        po, count = pals[name]
        cols = [struct.unpack_from('<H', m25, po + 2 * i)[0] for i in range(count)]
        lums = [luminance(c) for c in cols]
        for i, c in enumerate(cols):
            struct.pack_into('<H', m25, po + 2 * i, forest(c, min(lums), max(lums)))
    narc.files[25] = bytes(m25)
    model, anim = (open(f, 'rb').read() for f in scene)
    if model[:4] != b'BMD0' or anim[:4] != b'BCA0':
        sys.exit('%s / %s: not an .nsbmd / .nsbca' % tuple(scene))
    narc.files[38], narc.files[39] = model, anim

open(out_path, 'wb').write(narc.save())
