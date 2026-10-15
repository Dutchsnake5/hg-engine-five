"""Makes the white part of the fight menu's move buttons one tile row (8 px) taller, for MOVE_EFFECTIVENESS_DISPLAY
(include/config.h), so the move name and the effectiveness line under it both fit. Run by narcs.mk on the extracted a007
(battle HUD) files.

usage: build_effectiveness_buttons.py <a007 directory> <nitrogfx>

How the buttons are drawn (bottom screen BG; a007 member 37 = the fight menu's tilemap, 353 = the same with hg-engine's
Mega button): each button is 8 tile rows. Tile row 6 holds the white/tan boundary (white pixel rows 0-2, the tan strip's
top highlight from row 3), rows 7-8 are plain tan, row 9 is the bottom frame. Moving the boundary row down one tile row
(the plain white row above takes its place) needs no new tiles: only the tilemap changes. The tan strip keeps 14 px,
enough for the type tag and PP numbers, which move down to match (bytereplacement); the move names move up
(src/battle/move_effectiveness_display.c). Pressing a button shows the same tiles + 192 / + 384, so it still works.

Running it again on an already-changed tilemap changes nothing.
"""
import os
import struct
import subprocess
import sys

BOUNDARY_ROWS = (6, 14)            # tile rows of the white/tan boundary: top buttons, bottom buttons
BOUNDARY_TILES = range(113, 125)   # the boundary row's tiles of the four buttons (3 columns each)


def lz10_decompress(data):
    assert data[0] == 0x10, 'not LZ10 compressed'
    size = data[1] | (data[2] << 8) | (data[3] << 16)
    out = bytearray()
    i = 4
    while len(out) < size:
        flags = data[i]
        i += 1
        for bit in range(8):
            if len(out) >= size:
                break
            if flags & (0x80 >> bit):
                length = (data[i] >> 4) + 3
                disp = ((data[i] & 0xF) << 8 | data[i + 1]) + 1
                i += 2
                for _ in range(length):
                    out.append(out[-disp])
            else:
                out.append(data[i])
                i += 1
    return bytearray(out)


def section(data, magic):
    """offset of an NTR file section (its magic)"""
    pos = struct.unpack_from('<H', data, 0xC)[0]
    for _ in range(struct.unpack_from('<H', data, 0xE)[0]):
        if data[pos:pos + 4] == magic:
            return pos
        pos += struct.unpack_from('<I', data, pos + 4)[0]
    raise ValueError('section %s not found' % magic)


def fix_tilemap(path, nitrogfx):
    data = lz10_decompress(open(path, 'rb').read())
    base = section(data, b'NRCS') + 0x14       # u16 entries: tile (10 bits), h/v flip, palette (4 bits)
    entry = lambda tx, ty: struct.unpack_from('<H', data, base + 2 * (ty * 32 + tx))[0]
    put = lambda tx, ty, v: struct.pack_into('<H', data, base + 2 * (ty * 32 + tx), v)

    if not any((entry(tx, BOUNDARY_ROWS[0]) & 0x3FF) in BOUNDARY_TILES for tx in range(32)):
        return False                           # already done
    for row in BOUNDARY_ROWS:
        for tx in range(32):
            boundary = entry(tx, row)
            if (boundary & 0x3FF) in BOUNDARY_TILES:
                put(tx, row + 1, boundary)          # the boundary moves down a row (over the first plain tan row)
                put(tx, row, entry(tx, row - 1))    # and the plain white row above takes its place
    tmp = path + '.tmp'
    with open(tmp, 'wb') as f:
        f.write(bytes(data))
    subprocess.check_call([nitrogfx, tmp, tmp + '.lz'])   # back to LZ10, as the game loads it
    os.replace(tmp + '.lz', path)
    os.remove(tmp)
    return True


def main():
    a007, nitrogfx = sys.argv[1], sys.argv[2]
    for member in ('7_037', '7_353'):
        fix_tilemap(os.path.join(a007, member), nitrogfx)


if __name__ == '__main__':
    main()
