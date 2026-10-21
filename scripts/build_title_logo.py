#!/usr/bin/env python3
"""Puts the hack's logo on the title screen and turns HeartGold's orange sunburst behind it jade green
(CUSTOM_TITLE_LOGO).

    build_title_logo.py <rom.nds> <logo png> <output narc>

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
narc = ndspy.narc.NARC(ndspy.rom.NintendoDSRom.fromFile(rom_path).getFileByName('a/0/4/6'))

im = Image.open(png)
if im.mode != 'P' or im.size != (256, 120):
    sys.exit('%s: needs a 256 x 120 indexed (palette) PNG, got %s %s' % (png, im.mode, im.size))
pix = im.tobytes()
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

open(out_path, 'wb').write(narc.save())
