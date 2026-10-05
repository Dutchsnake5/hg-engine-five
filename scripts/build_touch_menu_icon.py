"""
Convert a 40x32 touch screen start menu icon PNG into the game's compressed sprite graphics (NCGR), using another
icon's graphics file from the same archive as the template for the file header.

usage: build_touch_menu_icon.py ICON.png TEMPLATE_NCGR PALETTE_NCLR OUTPUT

The PNG can be indexed or RGB(A), with any palette order, as long as every visible pixel uses one of the icon
palette's colors (a/0/1/4 file 14, its first 16 colors). Transparent pixels (the PNG's transparent index, or alpha 0)
become color 0. The game swaps in the palette's second row while the icon is selected, which turns some of these
colors red: that is how the selection border is drawn.
The game draws the icon as a 32x32 block plus an 8x32 strip on its right, so the tiles are stored in that order.
"""
import struct
import sys

from PIL import Image

WIDTH, HEIGHT = 40, 32


def lz10_decompress(src):
    size = src[1] | src[2] << 8 | src[3] << 16
    out = bytearray()
    i = 4
    while len(out) < size:
        flags = src[i]
        i += 1
        for bit in range(8):
            if len(out) >= size:
                break
            if flags & (0x80 >> bit):
                x = src[i] << 8 | src[i + 1]
                i += 2
                for _ in range((x >> 12) + 3):
                    out.append(out[-((x & 0xFFF) + 1)])
            else:
                out.append(src[i])
                i += 1
    return bytes(out)


def lz10_compress(data):
    # uncompressed blocks are valid LZ10 data
    out = bytearray([0x10, len(data) & 0xFF, (len(data) >> 8) & 0xFF, (len(data) >> 16) & 0xFF])
    for i in range(0, len(data), 8):
        out.append(0)
        out += data[i:i + 8]
    while len(out) % 4:
        out.append(0)
    return bytes(out)


def palette_colors(path):
    """the first 16 colors of an NCLR palette, as 8-bit RGB"""
    data = open(path, 'rb').read()
    pltt = data.find(b'TTLP')
    offset = struct.unpack_from('<I', data, pltt + 0x14)[0]
    colors = struct.unpack_from('<16H', data, pltt + 8 + offset)
    return [((c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3) for c in colors]


def icon_indices(path, palette):
    im = Image.open(path)
    if im.size != (WIDTH, HEIGHT):
        sys.exit(f'{path} must be {WIDTH}x{HEIGHT}, not {im.size[0]}x{im.size[1]}')
    rgba = im.convert('RGBA')
    # colors 1-15 of the palette; color 0 is transparent. the hardware keeps 5 bits per channel
    lookup = {}
    for i, c in enumerate(palette[1:], 1):
        lookup.setdefault(tuple(v >> 3 for v in c), i)
    rows, missing = [], set()
    for y in range(HEIGHT):
        row = []
        for x in range(WIDTH):
            r, g, b, a = rgba.getpixel((x, y))
            if a < 128:
                row.append(0)
                continue
            index = lookup.get((r >> 3, g >> 3, b >> 3))
            if index is None:
                missing.add((r, g, b))
                index = 0
            row.append(index)
        rows.append(row)
    if missing:
        allowed = ', '.join(f'({r}, {g}, {b})' for r, g, b in sorted(set(palette[1:])))
        sys.exit(f'{path} uses colors that are not in the icon palette: {sorted(missing)}\nallowed colors: {allowed}')
    return rows


def tile(rows, ox, oy):
    out = bytearray()
    for y in range(8):
        for x in range(0, 8, 2):
            out.append(rows[oy + y][ox + x] | rows[oy + y][ox + x + 1] << 4)
    return bytes(out)


def main():
    png, template_path, palette_path, output = sys.argv[1:5]
    template = bytearray(lz10_decompress(open(template_path, 'rb').read()))
    char_offset = struct.unpack_from('<H', template, 0xC)[0] + 0x20

    rows = icon_indices(png, palette_colors(palette_path))

    tiles = [tile(rows, (i % 4) * 8, (i // 4) * 8) for i in range(16)] + [tile(rows, 32, i * 8) for i in range(4)]
    data = b''.join(tiles)
    template[char_offset:char_offset + len(data)] = data
    open(output, 'wb').write(lz10_compress(bytes(template)))


if __name__ == '__main__':
    main()
