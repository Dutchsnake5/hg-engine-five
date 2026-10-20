"""Turns the FRZ status icon into FRB for FROSTBITE (include/config.h). Run by narcs.mk on the extracted a/0/3/9 files.

usage: build_frostbite_icons.py <a039 directory>

a/0/3/9 member 64 is the status icon strip the party menu and the summary screen use: 7 icons of 3 tiles each (4bpp,
uncompressed NCGR): PKRS, PAR, FRZ, SLP, PSN, BRN, FNT. The FRZ icon's Z (pixel columns 13-16 of the icon) is replaced by
the B of BRN (its columns 3-6), drawn in the FRZ icon's colours. The battle HP box has its own copy of the icons in
overlay 12 (bytereplacement). Running it again changes nothing.
"""
import os
import struct
import sys

ICON_TILES = 3
FRZ_ICON, BRN_ICON = 2, 5          # icon positions in the strip
Z_COLUMNS = range(13, 17)
B_COLUMNS = range(3, 7)


def main(directory):
    path = os.path.join(directory, '9_064')
    data = bytearray(open(path, 'rb').read())
    assert data[:4] == b'RGCN' and data[0x10:0x14] == b'RAHC', 'a/0/3/9 member 64 is not an uncompressed NCGR'
    assert struct.unpack_from('<I', data, 0x1C)[0] == 3, 'not 4bpp'
    tiles = 0x30

    def pixel(icon, x, y):
        tile = icon * ICON_TILES + x // 8
        i = y * 8 + x % 8
        b = data[tiles + tile * 32 + i // 2]
        return (b >> 4) if i & 1 else b & 0xF

    def set_pixel(icon, x, y, v):
        tile = icon * ICON_TILES + x // 8
        i = y * 8 + x % 8
        o = tiles + tile * 32 + i // 2
        data[o] = (data[o] & 0x0F) | (v << 4) if i & 1 else (data[o] & 0xF0) | v

    frz_bg = pixel(FRZ_ICON, 1, 3)      # inside colour, left of the letters
    brn_bg = pixel(BRN_ICON, 1, 3)
    for y in range(1, 7):
        for k, x in enumerate(Z_COLUMNS):
            v = pixel(BRN_ICON, B_COLUMNS[k], y)
            set_pixel(FRZ_ICON, x, y, frz_bg if v == brn_bg else v)
    open(path, 'wb').write(data)


if __name__ == '__main__':
    main(sys.argv[1])
