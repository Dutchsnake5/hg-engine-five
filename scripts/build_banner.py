#!/usr/bin/env python3
"""Writes the ROM's banner (the icon and title on the DS menu and in flashcart/emulator lists) for the ROM packer.

    build_banner.py <rom.nds> <base_dsrom/banner folder> [--icon] [--title]

--icon (FOREST_UI): HeartGold's gold Poke Ball icon, recoloured forest green with build_forest_ui.py's forest().
--title (CUSTOM_TITLE_LOGO): the title in every language becomes TITLE below.

ds-rom packs base_dsrom/banner: banner.yaml (titles), bitmap.png (32 x 32, each pixel in its palette colour) and
palette.png (16 x 1, colour 0 see-through), colours stored as the DS's 5-bit values x 8. That folder is only
re-extracted when rom.nds changes, so the icon is always made from rom.nds's own banner (bitmap at 0x20, 4bpp
tiles; palette at 0x220), never from the folder, and is never recoloured twice.
"""
import os
import struct
import sys

import ndspy.rom
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from build_forest_ui import forest  # noqa: E402

TITLE = ['Pokémon', 'Timeless Jade Version', "Five_4321"]
LANGUAGES = ['japanese', 'english', 'french', 'german', 'italian', 'spanish']

rom_path, folder = sys.argv[1:3]
flags = sys.argv[3:]
banner = ndspy.rom.NintendoDSRom.fromFile(rom_path).iconBanner

if '--icon' in flags:
    pal = [forest(struct.unpack_from('<H', banner, 0x220 + i * 2)[0]) for i in range(16)]
    rgb = [((c & 31) * 8, (c >> 5 & 31) * 8, (c >> 10 & 31) * 8) for c in pal]
    palette = Image.new('RGBA', (16, 1))
    for i, c in enumerate(rgb):
        palette.putpixel((i, 0), c + ((0,) if i == 0 else (255,)))
    bitmap = Image.new('RGBA', (32, 32))
    for t in range(16):
        for row in range(8):
            for col in range(8):
                byte = banner[0x20 + t * 32 + row * 4 + col // 2]
                v = byte >> 4 if col & 1 else byte & 15
                x, y = (t % 4) * 8 + col, (t // 4) * 8 + row
                bitmap.putpixel((x, y), (0, 0, 0, 0) if v == 0 else rgb[v] + (255,))
    palette.save(os.path.join(folder, 'palette.png'))
    bitmap.save(os.path.join(folder, 'bitmap.png'))

if '--title' in flags:
    path = os.path.join(folder, 'banner.yaml')
    text = open(path, encoding='utf-8').read()
    head, rest = text.split('title:\n', 1)
    tail = rest[rest.index('images:'):]
    body = ''.join('  %s: |-\n%s' % (lang, ''.join('    %s\n' % line for line in TITLE)) for lang in LANGUAGES)
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        f.write(head + 'title:\n' + body + tail)
