"""Seasonal map textures (SEASONAL_MAPS, proof of concept):
    build_seasons.py <rom.nds> <src/seasons.c> <roof json> <output map texture narc> <output building texture narc>
                     <output field effect narc>

Reads the map texture archive a/0/4/4 and the building texture archive a/0/7/0 from rom.nds itself (never from
base/root, which holds the previous build's copies) and appends seasonal versions where src/seasons.c expects them:
- map textures: for the n-th set in sSeasonalMapTexSets, SEASONAL_FIRST_MEMBER + 3n (spring), + 3n + 1 (autumn),
  + 3n + 2 (winter), recoloured by scripts/season_recolor.py (greens follow the season, flowers wilt);
- building textures: for every set s, SEASONAL_FIRST_BUILDING + 2s (autumn: planters wilt) and + 2s + 1 (winter:
  planters wilt, roofs get snow, roof colours and roof texels from the json files written by scripts/season_roofs.py).
Hand-painted winter building textures in data/seasons/roofs/ (scripts/season_roofpaint.py) go on top of the
automatic snow. Then hand-edited cells of data/seasons/flower_sheet.png (scripts/season_sheet.py) replace the automatic versions.
- field effects (a/1/0/3): for the n-th member in sSeasonalEffectMembers (the tall grass shake, leaves and the grass
  over the player's legs), SEASONAL_FIRST_EFFECT + 3n (spring), + 3n + 1 (autumn), + 3n + 2 (winter), every palette
  colour recoloured like the map's grass. After them, data/seasons/snow_ground.bin (scripts/season_snow_ground.py)
  with a header: u16 map file count, u16 0 (the plain ground tiles that play the snow footstep in winter).
- hand-painted map textures (data/seasons/maptex/<season>/) go on last.
- autumn map sets: the tree textures become four colour versions stacked in one taller texture (scripts/season_trees.py);
  src/seasons.c puts each tree of a loaded map model on one of them."""
import json
import os
import re
import struct
import sys

import ndspy.narc
import ndspy.rom

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import season_recolor as R  # noqa: E402

rom_path, src_path, roof_path, out_map, out_building, out_effect = sys.argv[1:7]
src = open(src_path, encoding='utf-8').read()
body = re.search(r'sSeasonalMapTexSets\[\] = \{(.*?)\};', src, re.S).group(1)
sets = [int(x) for x in re.findall(r'\d+', re.sub(r'//[^\n]*', '', body))]
first = int(re.search(r'#define SEASONAL_FIRST_MEMBER\s+(\d+)', src).group(1))
first_building = int(re.search(r'#define SEASONAL_FIRST_BUILDING\s+(\d+)', src).group(1))
first_effect = int(re.search(r'#define SEASONAL_FIRST_EFFECT\s+(\d+)', src).group(1))
effect_body = re.search(r'sSeasonalEffectMembers\[\] = \{(.*?)\};', src, re.S).group(1)
effect_members = [int(x) for x in re.findall(r'\d+', effect_body)]
roofs = json.load(open(roof_path))
masks = R.load_masks(os.path.join(os.path.dirname(os.path.abspath(roof_path)), 'roof_masks.json'))

rom = ndspy.rom.NintendoDSRom.fromFile(rom_path)
narc = ndspy.narc.NARC(rom.getFileByName('a/0/4/4'))
if len(narc.files) != first:
    sys.exit('build_seasons.py: a/0/4/4 has %d members, src/seasons.c expects %d' % (len(narc.files), first))
import season_trees as T  # noqa: E402
for s in sets:
    for season in (R.SPRING, R.AUTUMN, R.WINTER):
        d = R.recolor_btx(narc.files[s], season, 'map', frost=s in R.FROST_SETS)
        if season == R.AUTUMN:
            d = T.autumn_trees(narc.files[s], d)   # trees in several colours (scripts/season_trees.py)
        narc.files.append(d)
bnarc = ndspy.narc.NARC(rom.getFileByName('a/0/7/0'))
if len(bnarc.files) != first_building:
    sys.exit('build_seasons.py: a/0/7/0 has %d members, src/seasons.c expects %d' % (len(bnarc.files), first_building))
import season_roofpaint as RP  # noqa: E402
paint_problems = []
for s in range(first_building):
    for season in (R.AUTUMN, R.WINTER):
        d = R.recolor_btx(bnarc.files[s], season, 'building', roofs.get(str(s), {}), masks, frost=s in R.FROST_BUILDING_SETS)
        if season == R.WINTER and d[:4] == b'BTX0':
            d, problems = RP.apply_all(d, s, masks['_pairs'])   # hand-painted roofs (data/seasons/roofs/)
            paint_problems += ['building set %d: %s' % (s, p) for p in problems]
        bnarc.files.append(d)
if paint_problems:
    sys.exit('build_seasons.py: hand-painted roofs that don\'t fit:\n  ' + '\n  '.join(paint_problems))

# hand-edited cells from data/seasons/flower_sheet.png (scripts/season_sheet.py) replace the automatic versions
here = os.path.dirname(os.path.abspath(__file__))
sheet = os.path.join(here, '..', 'data', 'seasons', 'flower_sheet.png')
if os.path.exists(sheet):
    import season_sheet as SH

    def member_of(kind, s, season):
        if kind == 'map':
            v = {'spring': 0, 'autumn': 1, 'winter': 2}[season]
            return first + sets.index(s) * 3 + v if s in sets else None
        v = {'autumn': 0, 'winter': 1}.get(season)
        return None if v is None else first_building + s * 2 + v

    orig = {'map': list(narc.files[:first]), 'building': list(bnarc.files[:first_building])}
    out = {'map': narc.files, 'building': bnarc.files}
    SH.apply_sheet(out, sheet, os.path.join(here, '..', 'data', 'seasons', 'flower_sheet.json'), orig, roofs, member_of)

# hand-painted map textures (data/seasons/maptex/<season>/, scripts/season_roofpaint.py), on top of everything else;
# before them, the very tall grass in winter takes the hand-painted wild grass's colours
map_problems = []
map_pairs = RP.load_map_pairs()
for i, s in enumerate(sets):
    for v, season in enumerate(RP.MAP_SEASONS):
        k = first + i * 3 + v
        if season == 'winter':
            narc.files[k] = R.regrass_btx(narc.files[k], RP.winter_grass_ramp(narc.files[s], s), summer=narc.files[s])
        d, problems = RP.apply_map_all(narc.files[k], s, season, map_pairs)
        narc.files[k] = d
        map_problems += ['%s map set %d: %s' % (season, s, p) for p in problems]
if map_problems:
    sys.exit('build_seasons.py: hand-painted map textures that don\'t fit:\n  ' + '\n  '.join(map_problems))

# field effect grass (the shake in tall grass, leaves, the grass over the player's legs): textures and models alike
enarc = ndspy.narc.NARC(rom.getFileByName('a/1/0/3'))
if len(enarc.files) != first_effect:
    sys.exit('build_seasons.py: a/1/0/3 has %d members, src/seasons.c expects %d' % (len(enarc.files), first_effect))


def recolor_effect(data, season):
    out = bytearray(data)
    for off, n, name in R.palettes(out):
        for k in range(n):
            c = struct.unpack_from('<H', out, off + 2 * k)[0]
            struct.pack_into('<H', out, off + 2 * k, R.recolor(c, season))
    return bytes(out)


# in winter the grass you walk through takes the hand-painted wild grass's colours (set 2's painting) where there is one
effect_ramp = RP.winter_grass_ramp(narc.files[2], 2)
for m in effect_members:
    for season in (R.SPRING, R.AUTUMN, R.WINTER):
        if season == R.WINTER and effect_ramp:
            enarc.files.append(R.regrass_btx(enarc.files[m], effect_ramp, None))
        else:
            enarc.files.append(recolor_effect(enarc.files[m], season))
snow = open(os.path.join(os.path.dirname(roof_path), 'snow_ground.bin'), 'rb').read()
if len(snow) % 128:
    sys.exit('build_seasons.py: data/seasons/snow_ground.bin is not 128 bytes per map')
enarc.files.append(struct.pack('<HH', len(snow) // 128, 0) + snow)

open(out_map, 'wb').write(narc.save())
open(out_effect, 'wb').write(enarc.save())
open(out_building, 'wb').write(bnarc.save())
print('seasonal textures: map sets %s -> members %d-%d; building sets -> members %d-%d; field effects %s -> members %d-%d'
      % (sets, first, len(narc.files) - 1, first_building, len(bnarc.files) - 1, effect_members, first_effect,
         len(enarc.files) - 1))
