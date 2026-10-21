#!/usr/bin/env python3
"""Changes the opening movie's sprites (CUSTOM_INTRO): archive a/2/6/2 (pret demo/opening/gs_opening.narc).

    build_intro.py <rom.nds> <repo root> <input narc> <output narc>

- Scene 1, the legendary flying out of the sun toward the camera: Ho-Oh (members 23 palette, 24 tiles, 26 cells;
  the animation 25 is kept, so the flight path, timing and scaling stay the game's own) becomes Celebi, from the
  3 front-facing frames in data/graphics/intro/celebi_flying.png (32 x 32 each, from a Mystery Dungeon sheet), drawn
  CELEBI_SCALE times bigger. Ho-Oh's 6 wing-flap cells (1-6) become frames A B C A B C; cell 0 (the sun) is kept.
- Scene 4, the starters: Chikorita (85 palette, 86 tiles) becomes Rowlet and Totodile (89, 90) Oshawott, from
  hg-engine's front sprites (data/graphics/sprites/<mon>/male/front.png, first frame) at full size: the intro's
  starters are the battle front sprites' middle 48 x 48, pixels and palette (cells kept). Cyndaquil stays.
Runs at the end of hg-engine's own a/2/6/2 rule (OPENDEMO in narcs.mk, which edits other members): the input narc is
hg-engine's result, and the members changed here are always taken from rom.nds first, so rebuilding never stacks
changes. Members that were LZ-compressed are compressed again.
"""
import os
import struct
import sys

import ndspy.lz10
import ndspy.narc
import ndspy.rom
import numpy as np
from PIL import Image

CELEBI_SCALE = 3            # the frames are 17 x 25 Mystery Dungeon pixels; 3 = 51 x 75 in a 64 x 96 cell
STARTERS = {'rowlet': (85, 86), 'oshawott': (89, 90)}   # new mon -> (palette member, tile member) it replaces

rom_path, repo, in_path, out_path = sys.argv[1:5]
narc = ndspy.narc.NARC(open(in_path, 'rb').read())
original = ndspy.narc.NARC(ndspy.rom.NintendoDSRom.fromFile(rom_path).getFileByName('a/2/6/2'))
for m in (23, 24, 26, 85, 86, 89, 90):
    narc.files[m] = original.files[m]


def unpack(i):
    d = bytes(narc.files[i])
    if d[:1] == b'\x10':
        return ndspy.lz10.decompress(d), True
    return d, False


def repack(i, data, lz):
    narc.files[i] = ndspy.lz10.compress(data) if lz else data


def block(data, magic):
    nblk = struct.unpack_from('<H', data, 14)[0]
    p = 16
    for _ in range(nblk):
        if data[p:p + 4] == magic:
            return p
        p += struct.unpack_from('<I', data, p + 4)[0]
    sys.exit('no %s block' % magic)


def quantize(im, n):
    """RGBA -> (index array with 0 = see-through, list of up to n (r, g, b) 0-31 colours for indices 1..n)"""
    a = np.array(im)
    opaque = a[:, :, 3] >= 128
    cols = sorted({tuple(int(v) >> 3 for v in p[:3]) for p in a[opaque]})
    if len(cols) > n:
        q = Image.fromarray(a[:, :, :3]).quantize(n, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
        pal = q.getpalette()[:n * 3]
        idx = np.array(q) + 1
        cols = [tuple(v >> 3 for v in pal[k * 3:k * 3 + 3]) for k in range(n)]
    else:
        lut = {c: k + 1 for k, c in enumerate(cols)}
        idx = np.zeros(a.shape[:2], int)
        for y in range(a.shape[0]):
            for x in range(a.shape[1]):
                if opaque[y, x]:
                    idx[y, x] = lut[tuple(int(v) >> 3 for v in a[y, x, :3])]
    idx[~opaque] = 0
    return idx, cols


def tiles4(idx, x0, y0, w, h):
    """4bpp tiles of the w x h area at (x0, y0) of an index array, in 1D order (row by row of 8 x 8 tiles)"""
    out = bytearray()
    for ty in range(h // 8):
        for tx in range(w // 8):
            for yy in range(8):
                for xx in range(0, 8, 2):
                    y, x = y0 + ty * 8 + yy, x0 + tx * 8 + xx
                    lo = idx[y, x] if 0 <= y < idx.shape[0] and 0 <= x < idx.shape[1] else 0
                    hi = idx[y, x + 1] if 0 <= y < idx.shape[0] and 0 <= x + 1 < idx.shape[1] else 0
                    out.append(int(lo) | int(hi) << 4)
    return bytes(out)


def set_palette(member, slot, cols):
    d = bytearray(narc.files[member])
    p = block(d, b'TTLP')
    off = struct.unpack_from('<I', d, p + 20)[0]
    base = p + 8 + off + slot * 32
    for k in range(15):
        r, g, b = cols[k] if k < len(cols) else (0, 0, 0)
        struct.pack_into('<H', d, base + 2 * (k + 1), r | g << 5 | b << 10)
    narc.files[member] = bytes(d)


def set_tiles(data, tiles):
    """replace an NCGR's tile data (sizes updated)"""
    d = bytearray(data)
    p = block(d, b'RAHC')
    size, off = struct.unpack_from('<II', d, p + 0x18)
    start = p + 8 + off
    d[start:start + size] = tiles
    struct.pack_into('<I', d, p + 0x18, len(tiles))
    struct.pack_into('<I', d, p + 4, struct.unpack_from('<I', d, p + 4)[0] - size + len(tiles))
    struct.pack_into('<I', d, 8, len(d))
    return bytes(d)


# ---- scene 1: Celebi --------------------------------------------------------------------------------------------
sheet = Image.open(os.path.join(repo, 'data/graphics/intro/celebi_flying.png')).convert('RGBA')
frames = [sheet.crop((i * 32, 0, i * 32 + 32, 32)) for i in range(3)]
boxes = [f.getbbox() for f in frames]
x0 = min(b[0] for b in boxes); y0 = min(b[1] for b in boxes); x1 = max(b[2] for b in boxes); y1 = max(b[3] for b in boxes)
frames = [f.crop((x0, y0, x1, y1)).resize(((x1 - x0) * CELEBI_SCALE, (y1 - y0) * CELEBI_SCALE), Image.NEAREST) for f in frames]
W, H = 64, 96
if frames[0].width > W or frames[0].height > H:
    sys.exit('Celebi frames are %dx%d at scale %d; the cell holds %dx%d' % (frames[0].width, frames[0].height, CELEBI_SCALE, W, H))
canvas = []
allpix = Image.new('RGBA', (W * 3, H), (0, 0, 0, 0))
for i, f in enumerate(frames):
    c = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    c.alpha_composite(f, ((W - f.width) // 2, (H - f.height) // 2))
    canvas.append(c)
    allpix.alpha_composite(c, (i * W, 0))
idx, cols = quantize(allpix, 15)
set_palette(23, 1, cols)                      # Ho-Oh used OBJ palette 1 (the sun palette 0)

chars, lz24 = unpack(24)
p = block(chars, b'RAHC')
old_tiles = chars[p + 8 + struct.unpack_from('<I', chars, p + 0x1C)[0]:]
sun = old_tiles[:16 * 32]                      # the sun cell uses tiles 0-15
new = bytearray(sun)
frame_tile = []
for i in range(3):                             # each frame: a 64x64 piece then a 64x32 piece
    frame_tile.append(len(new) // 32)
    new += tiles4(idx, i * W, 0, 64, 64)
    new += tiles4(idx, i * W, 64, 64, 32)
repack(24, set_tiles(chars, bytes(new)), lz24)

cells, lz26 = unpack(26)
p = block(cells, b'KBEC')
ncell, battr, ofs = struct.unpack_from('<HHI', cells, p + 8)
assert battr & 1 and ncell == 7
ent0 = p + 8 + ofs
oam0 = ent0 + ncell * 16
sun_entry = cells[ent0:ent0 + 16]
n_sun, _, sun_ofs = struct.unpack_from('<HHI', sun_entry)
sun_oams = cells[oam0 + sun_ofs: oam0 + sun_ofs + n_sun * 6]
entries = bytearray(sun_entry)
oams = bytearray(sun_oams)
order = [0, 1, 2, 0, 1, 2]                     # Ho-Oh's 6 flap cells -> Celebi frames A B C A B C
for c in range(6):
    t = frame_tile[order[c]]
    entries += struct.pack('<HHIhhhh', 2, 0x0800 | 15, len(oams), 31, 47, -32, -48)
    # 64x64 at (-32, -48), then 64x32 at (-32, 16); OBJ palette 1
    oams += struct.pack('<HHH', (-48 & 0xFF), (-32 & 0x1FF) | 3 << 14, t | 1 << 12)
    oams += struct.pack('<HHH', (16 & 0xFF) | 1 << 14, (-32 & 0x1FF) | 3 << 14, (t + 64) | 1 << 12)
kbec = bytearray(cells[p:p + 8 + ofs]) + entries + oams
while len(kbec) % 4:
    kbec.append(0)
struct.pack_into('<I', kbec, 4, len(kbec))
rest = cells[p + struct.unpack_from('<I', cells, p + 4)[0]:]     # LABL / UEXT blocks, unchanged
newcells = bytearray(cells[:p]) + kbec + rest
struct.pack_into('<I', newcells, 8, len(newcells))
repack(26, bytes(newcells), lz26)

# ---- scene 4: Rowlet and Oshawott ---------------------------------------------------------------------------------
# The intro's starters ARE the battle front sprites at full size: Chikorita's 48 x 48 intro sprite is exactly the
# middle of its 80 x 80 front sprite (pixels and palette). So the new ones are taken the same way, pixel for pixel,
# with their own palettes (no resizing or new colours).
LAYOUT = [(0, 0, 32, 32), (32, 0, 16, 32), (0, 32, 32, 16), (32, 32, 16, 16)]   # the 48x48 cell's 4 pieces, tile order
for mon, (pal_m, chr_m) in STARTERS.items():
    src = Image.open(os.path.join(repo, 'data/graphics/sprites/%s/male/front.png' % mon))
    if src.mode != 'P':
        sys.exit('%s front.png: expected an indexed (16-colour) picture' % mon)
    pal = src.getpalette()[:48]
    idx = np.array(src)[:80, :80].astype(int)       # first frame; index 0 = see-through
    ys, xs = np.where(idx != 0)
    x0, y0 = 16, 16                                 # the middle 48 x 48, as the originals
    if xs.min() < x0 or ys.min() < y0 or xs.max() >= x0 + 48 or ys.max() >= y0 + 48:
        cx, cy = (xs.min() + xs.max() + 1) // 2, (ys.min() + ys.max() + 1) // 2
        x0, y0 = cx - 24, cy - 24                   # doesn't fit the middle: centre it instead
        if xs.max() - xs.min() >= 48 or ys.max() - ys.min() >= 48:
            sys.exit('%s: the sprite is bigger than the intro\'s 48 x 48' % mon)
    win = np.zeros((48, 48), int)
    for y in range(48):
        for x in range(48):
            if 0 <= y0 + y < 80 and 0 <= x0 + x < 80:
                win[y, x] = idx[y0 + y, x0 + x]
    cols = [tuple(v >> 3 for v in pal[k * 3:k * 3 + 3]) for k in range(1, 16)]
    t = b''.join(tiles4(win, x, y, w, h) for x, y, w, h in LAYOUT)
    chars, lz = unpack(chr_m)
    repack(chr_m, set_tiles(chars, t), lz)
    set_palette(pal_m, 0, cols)

open(out_path, 'wb').write(narc.save())
