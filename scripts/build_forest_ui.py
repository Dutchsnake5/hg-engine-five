#!/usr/bin/env python3
"""Turns HeartGold's gold interface colours forest green (FOREST_UI).

    build_forest_ui.py intro <rom.nds> <output narc>
    build_forest_ui.py textbox <rom.nds> <output narc> [<input narc>]

intro: Oak's speech (pret oaks_speech.c) colours its backgrounds from archive a/1/2/0 (pret demo/intro/intro.narc):
member 1 = top screen (rows 0-2 loaded), member 30 = bottom screen (rows 0-4); members 2 and 31 are SoulSilver's
blue versions. Their gold ramp (stripes, Poke Ball pattern) becomes green; the buttons' blues and cyans, greys and
white stay.

textbox: the message box frames are archive a/0/3/8, frame n = tiles member n + 2, palette member n + 26
(LoadUserFrameGfx2). Frame 0 ("Type 1" in the Options, the default, also the one Oak's speech uses) has a gold
strip, colours 11-13 of member 26. The other members come from the input narc when given (hg-engine's own
textbox narc, with IMPLEMENT_TRANSPARENT_TEXTBOXES), else from the ROM; member 26 always comes from the ROM.

Everything is read from the original ROM, never from base/root (which holds the previous build's copy), so the
change is never applied twice.
"""
import colorsys
import struct
import sys

import ndspy.narc
import ndspy.rom


PIVOT = 0.62     # brightness the ramp's middle ends up at (lower = darker overall)
CONTRAST = 1.9   # how far the ramp's light and dark ends are pushed apart (1 = as in HeartGold)


def forest(c):
    """gold / orange / tan (hue 15-70 degrees) -> forest green, with more contrast and less saturation; others unchanged"""
    r, g, b = ((c >> s & 31) / 31 for s in (0, 5, 10))
    h, s, v = colorsys.rgb_to_hsv(r, g, b)
    d = h * 360
    if not (15 <= d <= 70) or s < 0.15:
        return c
    # keep the ramp's small hue differences (lighter = yellower) as a spread around 122 degrees
    h = (122 + (d - 40) * 0.4) / 360
    # the gold ramp only spans brightness ~0.65-1; spread it (CONTRAST times around 0.85 -> PIVOT) so the light
    # stripes and the dark pattern/outlines stand further apart
    v = max(0.0, min(1.0, PIVOT + (v - 0.85) * CONTRAST))
    r, g, b = colorsys.hsv_to_rgb(h, s * 0.8, v)
    return (c & 0x8000) | round(r * 31) | round(g * 31) << 5 | round(b * 31) << 10


def recolour(nclr, first=0, count=None):
    nclr = bytearray(nclr)
    i = nclr.find(b'TTLP')
    assert nclr[:4] == b'RLCN' and i >= 0
    psize, off = struct.unpack_from('<II', nclr, i + 16)
    start = i + 8 + off
    n = psize // 2 if count is None else count
    for k in range(first, first + n):
        o = start + k * 2
        struct.pack_into('<H', nclr, o, forest(struct.unpack_from('<H', nclr, o)[0]))
    return bytes(nclr)


mode = sys.argv[1]
rom = ndspy.rom.NintendoDSRom.fromFile(sys.argv[2])
if mode == 'intro':
    narc = ndspy.narc.NARC(rom.getFileByName('a/1/2/0'))
    for m in (1, 30):
        narc.files[m] = recolour(narc.files[m])
    out = sys.argv[3]
elif mode == 'textbox':
    orig = ndspy.narc.NARC(rom.getFileByName('a/0/3/8'))
    narc = ndspy.narc.NARC(open(sys.argv[4], 'rb').read()) if len(sys.argv) > 4 else orig
    narc.files[26] = recolour(orig.files[26], 11, 3)
    out = sys.argv[3]
else:
    sys.exit('mode must be intro or textbox')
with open(out, 'wb') as f:
    f.write(narc.save())
