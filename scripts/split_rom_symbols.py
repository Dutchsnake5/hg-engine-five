#!/usr/bin/env python3
"""
Split a linker symbol file (rom.ld, build/rom_gen.ld, build/rom_gen_battle.ld) so the linker can call game functions
with a plain bl/blx instead of LONG_CALL's address literal.

usage: split_rom_symbols.py SYMBOLS.ld OUT_ARM.ld OUT_THUMB.s [KEEP.o ...]

The linker treats a symbol assigned in a linker script as ARM code, so a bl to a Thumb game function would switch to
ARM mode and crash. Every symbol whose value is odd (the Thumb bit) goes to OUT_THUMB.s as a .thumb_set, which marks
it as a Thumb function. Everything else (ARM functions and data) stays in OUT_ARM.ld, where linking with --use-blx
calls the ARM functions with blx.

Symbols that a KEEP.o object also defines stay in OUT_ARM.ld, as before: every overlay links its own copy of
thumb_help.o, and the linker script's address (overlay 129's copy) takes precedence over it, while the object marks
it as Thumb. Moving those to OUT_THUMB.s would make them defined twice.
"""
import re
import subprocess
import sys

ASSIGNMENT = re.compile(r'^\s*([A-Za-z_]\w*)\s*=\s*([^;]+);\s*$')
NM = '/opt/devkitpro/devkitARM/bin/arm-none-eabi-nm' if __import__('os').path.exists('/opt/devkitpro/devkitARM/bin/') else 'arm-none-eabi-nm'
NUMERIC = re.compile(r'[0-9A-Fa-fxX\s+|\-()]+')


def strip_comments(line):
    return re.sub(r'/\*.*?\*/', '', line.split('//')[0])


def defined_symbols(objects):
    names = set()
    for obj in objects:
        for line in subprocess.check_output([NM, '--defined-only', obj], text=True).splitlines():
            parts = line.split()
            if len(parts) == 3:
                names.add(parts[2])
    return names


def main():
    src, out_ld, out_s = sys.argv[1:4]
    keep = defined_symbols(sys.argv[4:])
    arm_lines = []
    thumb_lines = ['.text\n', '.thumb\n']
    for line in open(src, encoding='utf-8'):
        m = ASSIGNMENT.match(strip_comments(line))
        if m and NUMERIC.fullmatch(m.group(2)):
            value = eval(m.group(2), {'__builtins__': {}})
            if value & 1 and m.group(1) not in keep:
                thumb_lines.append(f'.global {m.group(1)}\n.thumb_set {m.group(1)}, 0x{value & ~1:08X}\n')
                continue
        arm_lines.append(line)
    open(out_ld, 'w', encoding='utf-8').writelines(arm_lines)
    open(out_s, 'w', encoding='utf-8').writelines(thumb_lines)


if __name__ == '__main__':
    main()
