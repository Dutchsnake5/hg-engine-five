"""Reads and recolours Nitro particle files (.spa, a/0/2/9 members) for move animations.

Layout from pret pokeheartgold lib/spl (SPLFileHeader, SPLResourceHeader, ...): a 0x20 header, then one block per
emitter (an 88-byte header plus optional scale/colour/alpha/texture animations, child resource and behaviours, as its
flags say), then the textures. Colours are GXRgb (BGR555): the emitter's colour (+0x22), the colour animation's start
and end, the child resource's colour.

usage: spa_tool.py list <file.spa>
       spa_tool.py recolor <in.spa> <out.spa> <r,g,b> [min brightness 0-1]
                     (every colour -> the given colour, keeping each one's brightness but not below the minimum)
"""
import struct
import sys

HEADER = 88
SIZES = [('scale', 8, 12), ('color', 9, 12), ('alpha', 10, 8), ('tex', 11, 12), ('child', 16, 20)]
BEHAVIOURS = [(24, 8), (25, 8), (26, 16), (27, 4), (28, 8), (29, 16)]


def colour_slots(data):
    """[(offset, what)] for every GXRgb in the emitters."""
    magic, version, res_count, tex_count, _, res_size, tex_size, tex_offset = struct.unpack_from('<4s4sHHIIII', data, 0)
    assert magic == b' APS', 'not a particle file'
    slots = []
    o = 0x20
    for r in range(res_count):
        flags = struct.unpack_from('<I', data, o)[0]
        slots.append((o + 0x22, 'emitter %d colour' % r))
        p = o + HEADER
        for name, bit, size in SIZES:
            if flags >> bit & 1:
                if name == 'color':
                    slots += [(p, 'emitter %d colour anim start' % r), (p + 2, 'emitter %d colour anim end' % r)]
                if name == 'child':
                    slots.append((p + 10, 'emitter %d child colour' % r))
                p += size
        for bit, size in BEHAVIOURS:
            if flags >> bit & 1:
                p += size
        o = p
    assert o == 0x20 + res_size, 'emitter blocks end at 0x%X, header says 0x%X' % (o, 0x20 + res_size)
    return slots


def rgb(c):
    return (c & 31, c >> 5 & 31, c >> 10 & 31)


def recolor(data, target, min_light=0.0):
    data = bytearray(data)
    tr, tg, tb = target
    for o, _ in colour_slots(data):
        r, g, b = rgb(struct.unpack_from('<H', data, o)[0])
        light = max(max(r, g, b) / 31, min_light)   # keep each colour's brightness (not below min_light), take the target's hue
        top = max(target)
        nr, ng, nb = (min(31, round(x / top * 31 * light)) for x in target)
        struct.pack_into('<H', data, o, nr | ng << 5 | nb << 10)
    return bytes(data)


if __name__ == '__main__':
    if sys.argv[1] == 'list':
        d = open(sys.argv[2], 'rb').read()
        for o, what in colour_slots(d):
            c = struct.unpack_from('<H', d, o)[0]
            print('0x%04X %-32s %s' % (o, what, tuple(x * 8 for x in rgb(c))))
    elif sys.argv[1] == 'recolor':
        d = open(sys.argv[2], 'rb').read()
        floor = float(sys.argv[5]) if len(sys.argv) > 5 else 0.0
        open(sys.argv[3], 'wb').write(recolor(d, tuple(int(x) for x in sys.argv[4].split(',')), floor))
