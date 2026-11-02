"""Platinum's snow footstep for SEQ_SE_DP_YUKIASHI (SEASONAL_MAPS: tall grass in winter):
    season_snow_step.py <unpacked sdat dir (build/sdat)> <data/seasons/snow_step.swav>

HeartGold kept Diamond/Pearl's sequence but pointed it at BANK_SE_FIELD instrument 82, which in HeartGold's bank plays
a splash (WAVE_ARC_SE_FIELD wave 13). Platinum's sequence uses instrument 112 = its WAVE_ARC_SE_FIELD wave 22, a short
crunch; HeartGold only has that sample in the Nut Mixer's archive, which isn't loaded in the overworld. So this adds
Platinum's sample to WAVE_ARC_SE_FIELD (snow_step.swav, extracted once from the Platinum ROM), a Single instrument with
Platinum's envelope in a free BANK_SE_FIELD slot, and points the sequence at it. Runs on the files SDATTool unpacked,
before it packs them again; does nothing if already applied."""
import json
import os
import struct
import sys

sdat, swav = sys.argv[1:3]
files = os.path.join(sdat, 'Files')
SLOT = 61                                           # NULL in HeartGold's BANK_SE_FIELD
PT_ENVELOPE = (60, 125, 127, 127, 120, 64)          # pitch, attack, decay, sustain, release, pan (Platinum inst 112)

# the wave: next free number in WAVE_ARC_SE_FIELD
fb_path = os.path.join(sdat, 'FileBlock.json')
fb = json.load(open(fb_path))
arc = next(f for f in fb['file'] if f['name'] == 'WAVE_ARC_SE_FIELD.swar')
data = open(swav, 'rb').read()
folder = os.path.join(files, 'WAVARC', 'WAVE_ARC_SE_FIELD')
wave = next((i for i, n in enumerate(arc['subFile']) if open(os.path.join(folder, n), 'rb').read() == data), None)
if wave is None:
    wave = len(arc['subFile'])
    name = '%02X.swav' % wave
    open(os.path.join(folder, name), 'wb').write(data)
    arc['subFile'].append(name)
    json.dump(fb, open(fb_path, 'w'), indent=4)
    swar = os.path.join(files, 'WAVARC', 'WAVE_ARC_SE_FIELD.swar')
    if os.path.exists(swar):
        os.remove(swar)                             # SDATTool rebuilds it from the swav list

# the instrument: a Single in the free slot, appended to the bank's data
sbnk_path = os.path.join(files, 'BANK', 'BANK_SE_FIELD.sbnk')
b = bytearray(open(sbnk_path, 'rb').read())
rec = 0x3C + 4 * SLOT
warc_slot = 1                                       # BANK_SE_FIELD's wave archives: [WAVE_ARC_BASIC, WAVE_ARC_SE_FIELD]
entry = struct.pack('<HH6B', wave, warc_slot, *PT_ENVELOPE)
if b[rec] == 0:
    while len(b) % 4:
        b.append(0)
    off = len(b)
    b += entry
    while len(b) % 4:
        b.append(0)
    struct.pack_into('<BHB', b, rec, 1, off, 0)     # type 1 = Single
    struct.pack_into('<I', b, 8, len(b))            # file size
    struct.pack_into('<I', b, 0x14, len(b) - 0x10)  # DATA block size
    open(sbnk_path, 'wb').write(b)
    bank_txt = sbnk_path[:-5] + '.txt'
    if os.path.exists(bank_txt):                    # keep the readable copy in step
        lines = open(bank_txt).read().split('\n')
        lines = ['%d, Single, %d, %d, %d, %d, %d, %d, %d, %d' % ((SLOT, wave, warc_slot) + PT_ENVELOPE)
                 if l.strip() == '%d, NULL' % SLOT else l for l in lines]
        open(bank_txt, 'w').write('\n'.join(lines))
elif b[rec] != 1 or struct.unpack_from('<HH6B', b, b[rec + 1] | (b[rec + 2] << 8)) != struct.unpack('<HH6B', entry):
    sys.exit('season_snow_step.py: BANK_SE_FIELD slot %d is already used by something else' % SLOT)

# the sequence: instrument switch 0x81 82 -> 0x81 SLOT
seq_path = os.path.join(files, 'SEQ', 'SEQ_SE_DP_YUKIASHI.sseq')
s = bytearray(open(seq_path, 'rb').read())
i = s.find(b'\x81\x52', 0x1C)
if i >= 0:
    s[i + 1] = SLOT
    open(seq_path, 'wb').write(s)
elif s.find(bytes([0x81, SLOT]), 0x1C) < 0:
    sys.exit('season_snow_step.py: SEQ_SE_DP_YUKIASHI has no instrument switch to change')
print('snow footstep: WAVE_ARC_SE_FIELD wave %d, BANK_SE_FIELD instrument %d' % (wave, SLOT))
