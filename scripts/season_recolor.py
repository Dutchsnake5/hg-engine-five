"""Seasonal recolouring of map texture sets (SEASONAL_MAPS, proof of concept): a BTX0 file's TEX0 palette data is
rewritten colour by colour, so every texture format that uses palettes (all of HeartGold's outdoor ones) follows.
Only greens and teals (foliage, grass, the mint ground) change; water, paths, rock, wood and flowers stay.
Seasons follow hg-engine's GrabCurrentSeason(): 0 spring, 1 summer (the original), 2 autumn, 3 winter."""
import colorsys
import os
import struct

SPRING, SUMMER, AUTUMN, WINTER = 0, 1, 2, 3


# Platinum's snow (map texture set 14, Snowpoint City: conttree2_t, c09_ice, blueglay, fenter), darkest to brightest
WINTER_RAMP = [(0x94, 0x8B, 0xAC), (0xA4, 0xB4, 0xD5), (0xBD, 0xC5, 0xE6), (0xCD, 0xDE, 0xF6), (0xE6, 0xF6, 0xFF), (0xFF, 0xFF, 0xFF)]
WINTER_SHADE = 0.34   # tree greens darker than this stay dark green under the snow
WINTER_DUST = 0.6     # winter sand and dirt paths: how far they blend toward DUST_WHITE
STONE_DUST = 0.15     # winter stone paths (greyish palettes): only a slight whitening (2026-10-31)
STONE_SAT = 0.15      # a path palette whose colours average below this saturation is stone
DUST_WHITE = (0xF0 / 255, 0xF8 / 255, 1.0)


def _ramp(ramp, t):
    t = max(0.0, min(1.0, t)) * (len(ramp) - 1)
    i = min(int(t), len(ramp) - 2)
    f = t - i
    return tuple((ramp[i][k] + (ramp[i + 1][k] - ramp[i][k]) * f) / 255 for k in range(3))


def _rgb(c):
    return ((c & 31) / 31, ((c >> 5) & 31) / 31, ((c >> 10) & 31) / 31)


def _bgr555(r, g, b):
    f = lambda x: max(0, min(31, round(x * 31)))
    return f(r) | (f(g) << 5) | (f(b) << 10)


def recolor(c, season, foliage=False):
    """One BGR555 colour for a season (foliage: a red or gold leaf colour that should follow the season like green)."""
    r, g, b = _rgb(c & 0x7FFF)
    h, s, v = colorsys.rgb_to_hsv(r, g, b)
    deg = h * 360
    green = _green_hsv(deg, s) or (foliage and season == WINTER)
    if season == AUTUMN and green:
        # dark foliage -> red-orange, light ground -> straw gold
        nh = (12 + 38 * v) / 360
        s = min(1.0, s * 1.05 + 0.15)
        r, g, b = colorsys.hsv_to_rgb(nh, s, v * 0.97)
    elif season == WINTER:
        if green and v < WINTER_SHADE:
            # the deepest foliage shade shows through the snow as dark green, as on Platinum's snowy tree tops
            r, g, b = colorsys.hsv_to_rgb(80 / 360, min(s, 0.55), v * 0.95)
        elif green:
            # snow, on Platinum's own snow colours (Snowpoint's tree tops, snow mounds and ground) by brightness
            # biased toward the bright end: HeartGold's map lighting darkens lit materials (the trees) more than
            # Platinum's snowy areas do
            t = (v - WINTER_SHADE) / (1 - WINTER_SHADE)
            r, g, b = _ramp(WINTER_RAMP, 0.35 + 0.65 * max(0.0, t) ** 0.6)
        else:
            r, g, b = colorsys.hsv_to_rgb(h, s * 0.75, min(1.0, v * 1.03))
    elif season == SPRING and green:
        nh = (deg + (95 - deg) * 0.5) / 360
        r, g, b = colorsys.hsv_to_rgb(nh, min(1.0, s * 1.12), min(1.0, v * 1.06))
    else:
        return c
    return (c & 0x8000) | _bgr555(r, g, b)


def _hsv(c):
    return colorsys.rgb_to_hsv(*_rgb(c & 0x7FFF))


def _green_hsv(deg, s):
    """Grass and leaves. Pale yellowish greys (beach sand eeeecd, the Battle Frontier's stone paving c5cdac) and pale
    cyan greys (Olivine's cobblestones acc5c5) sit in the same hue range, so outside 85-165 degrees a colour has to be
    clearly saturated to count."""
    return 60 <= deg <= 185 and s > 0.12 and (85 <= deg <= 165 or s > 0.25)


def _is_green(c):
    h, s, v = _hsv(c)
    return _green_hsv(h * 360, s)


def _is_leaf(c):
    """A red, orange or gold leaf colour (not the dark trunk browns)."""
    h, s, v = _hsv(c)
    deg = h * 360
    return (deg < 60 or deg > 330) and s > 0.35 and v >= 0.45


def _is_petal(c):
    """A flower colour: saturated, neither green nor water blue."""
    h, s, v = _hsv(c)
    deg = h * 360
    return s > 0.3 and not (60 <= deg <= 260)


def _nearest(c, choices):
    v = _hsv(c)[2]
    return min(choices, key=lambda k: abs(_hsv(k)[2] - v))


GROUND_MINT = 0x7B6E   # HeartGold's mint ground (#73deac), for planters whose palette has no ground colour


def _shade(c, sat, val):
    h, s, v = _hsv(c)
    r, g, b = colorsys.hsv_to_rgb(h, min(1.0, s * sat), min(1.0, v * val))
    return (c & 0x8000) | _bgr555(r, g, b)


def wilt(cols, season, ground):
    """Autumn/winter flowers turn into plain grass (2026-10-31: they should just look like grass, not a blob).
    Ground flowers (dots in the grass) melt into the ground. Flower beds and planters: every leaf and petal colour
    becomes one of three shades of the ground colour, by brightness, so the bed reads as grassy ground; the season's
    recolour then treats them exactly like the ground around them."""
    greens = [c for c in cols if _is_green(c)]
    if ground:
        base = [c for c in greens if _hsv(c)[2] <= 0.86] or greens
        if not base:
            return cols
        return [c if c in base else _nearest(c, base) for c in cols]
    soil = [c for c in greens if _hsv(c)[2] >= 0.8]
    g = max(soil, key=lambda c: _hsv(c)[2]) if soil else GROUND_MINT
    shades = [_shade(g, 1.06, 0.88), g, _shade(g, 0.9, 1.05)]   # dark, mid, light
    plants = [c for c in cols if (_is_petal(c) or _is_green(c)) and c not in soil]
    if not plants:
        return cols
    order = sorted(set(plants), key=lambda c: _hsv(c)[2])
    pick = {c: shades[min(2, i * 3 // len(order))] for i, c in enumerate(order)}
    return [pick.get(c, c) for c in cols]


MAPLES = ('tree_en',)   # Ecruteak's red/gold maples: snow on their leaves in winter (the other seasons keep them)
PATHS = ('road', 'fieldkk', 'bf_road')
BEACHES = ('beach',)   # 2026-10-31: sand stays as it is in every season; only the grass edges change   # winter: palettes whose names start like this are paths and dirt fields (dusted)


DUST_CONTRAST = 1.6   # winter paths: the dusting flattens a palette to 40% of its contrast; spread it back this much


def dusted(cols):
    """Winter paths (2026-10-31: snow-dusted): each colour blended toward a cool snow white, then the darker ones
    spread back down from the palette's brightest (DUST_CONTRAST) so stones and sand grain still show while the
    brightest keeps its dusted shade. Greens (grass edges) get snow. Unused-looking entries (white, black, magenta)
    are left out of that."""
    lum = lambda v: 0.3 * v[0] + 0.59 * v[1] + 0.11 * v[2]
    path = [c for c in cols if not _is_green(c) and c & 0x7FFF not in (0x7FFF, 0, 0x7C1F)]
    stone = bool(path) and sum(_hsv(c)[1] for c in path) / len(path) < STONE_SAT
    amount = STONE_DUST if stone else WINTER_DUST
    rgb = {k: [x + (w - x) * amount for x, w in zip(_rgb(c & 0x7FFF), DUST_WHITE)]
           for k, c in enumerate(cols) if not _is_green(c)}
    real = [lum(rgb[k]) for k in rgb if cols[k] & 0x7FFF not in (0x7FFF, 0, 0x7C1F)]
    top = max(real) if real else 0
    out = []
    for k, c in enumerate(cols):
        if k not in rgb:
            out.append(recolor(c, WINTER))
        elif c & 0x7FFF in (0x7FFF, 0, 0x7C1F):
            out.append((c & 0x8000) | _bgr555(*rgb[k]))
        else:
            d = 0 if stone else (lum(rgb[k]) - top) * (DUST_CONTRAST - 1)
            out.append((c & 0x8000) | _bgr555(*(x + d for x in rgb[k])))
    return out


def _flowery(name, cols):
    """Flower palettes go by HeartGold's names: 'flower…' (dots in the grass), '…_sp…' (beds, planters), 'hana'."""
    named = name.startswith('flower') or '_sp' in name or 'hana' in name
    return named and any(_is_petal(c) for c in cols) and any(_is_green(c) for c in cols)


def palettes(data):
    """(offset in data, colour count, name) of every palette in a BTX0/BMD0 file's TEX0 blocks."""
    for i in range(struct.unpack_from('<H', data, 0x0E)[0]):
        t = struct.unpack_from('<I', data, 0x10 + 4 * i)[0]
        if data[t:t + 4] != b'TEX0':
            continue
        end = t + struct.unpack_from('<I', data, t + 4)[0]
        pal = t + struct.unpack_from('<I', data, t + 0x38)[0]
        d = t + struct.unpack_from('<I', data, t + 0x34)[0]
        n = data[d + 1]
        first = d + 4 + struct.unpack_from('<H', data, d + 6)[0]       # NNS dictionary: tree, then entries, then names
        esize = struct.unpack_from('<H', data, first - 4)[0]
        offs = [struct.unpack_from('<H', data, first + esize * k)[0] << 3 for k in range(n)]
        names_at = first + n * esize
        names = [bytes(data[names_at + 16 * k:names_at + 16 * k + 16]).split(b'\0')[0].decode('latin1') for k in range(n)]
        bounds = sorted(set(offs)) + [end - pal]
        for o, name in zip(offs, names):
            nxt = min(x for x in bounds if x > o)
            yield pal + o, (nxt - o) // 2, name


def textures(data):
    """(name, texture parameter, offset of its texels in data) of every texture in a BTX0/BMD0 file's TEX0 blocks."""
    for i in range(struct.unpack_from('<H', data, 0x0E)[0]):
        t = struct.unpack_from('<I', data, 0x10 + 4 * i)[0]
        if data[t:t + 4] != b'TEX0' or not struct.unpack_from('<H', data, t + 0x0E)[0]:
            continue
        tex = t + struct.unpack_from('<I', data, t + 0x14)[0]
        d = t + struct.unpack_from('<H', data, t + 0x0E)[0]
        n = data[d + 1]
        first = d + 4 + struct.unpack_from('<H', data, d + 6)[0]
        esize = struct.unpack_from('<H', data, first - 4)[0]
        names_at = first + n * esize
        for k in range(n):
            param = struct.unpack_from('<I', data, first + esize * k)[0]
            name = bytes(data[names_at + 16 * k:names_at + 16 * k + 16]).split(b'\0')[0].decode('latin1')
            yield name, param, tex + ((param & 0xFFFF) << 3)


# texel formats with palette indices: format -> (bits per texel, index mask)
_INDEXED = {1: (8, 0x1F), 2: (2, 0x3), 3: (4, 0xF), 4: (8, 0xFF), 6: (8, 0x7)}


def load_masks(path):
    """data/seasons/roof_masks.json (scripts/season_roofs.py) -> {texture: (w, h, [palettes], flat 0/1 list),
    '_pairs': {palette: [textures shown with it]}}."""
    import base64
    import json
    import zlib
    out = {}
    over = os.path.join(os.path.dirname(os.path.abspath(path)), 'roof_overrides.json')
    out['_overrides'] = {k: v for k, v in json.load(open(over)).items() if not k.startswith('_')} if os.path.exists(over) else {}
    for name, e in json.load(open(path)).items():
        if name == '_pairs':
            out[name] = e
            continue
        bits = zlib.decompress(base64.b64decode(e['bits']))
        flat = [(bits[i >> 3] >> (7 - (i & 7))) & 1 for i in range(e['w'] * e['h'])]
        out[name] = (e['w'], e['h'], e['pals'], flat)
    return out


SNOW_SHADES = 4          # at most this many snow colours per roof palette
SNOW_LUM = (0.7, 1.0)    # roof colours, by their place in the roof's area, aim for snow of this brightness
STRAY_MAX, STRAY_SHARE = 12, 0.25   # a roof colour shown off the roof at most this often (texels, or share of its roof use) is freed for snow
MERGE_MAX = 0.06         # colours closer than this (RGB distance, 0-1.7) can be merged to free a palette entry for snow
SNOW_LOW = 0.45          # the darkest new snow shade's place on WINTER_RAMP (1 = white); keeps the contrast soft
NEUTRAL_TRIM = 0.12      # on a coloured roof, texels below this saturation are trim (rims, edges) and keep their colour


def _dist(c1, c2):
    return sum((x - y) ** 2 for x, y in zip(_rgb(c1 & 0x7FFF), _rgb(c2 & 0x7FFF))) ** 0.5


def _texel_io(out, off, bpp, imask):
    per = 8 // bpp

    def get(i):
        return (out[off + i // per] >> ((i % per) * bpp)) & imask

    def put(i, k):
        byte, shift = off + i // per, (i % per) * bpp
        out[byte] = (out[byte] & ~(imask << shift) & 0xFF) | (k << shift)
    return get, put


def snow_roof_texels(out, masks, pal_cols):
    """Winter roofs, texel by texel (2026-10-31). Only roof texels change (data/seasons/roof_masks.json: faces
    that face up in the upper part of a model, minus texels a wall or low face also shows), so logos, awnings, ribs and
    rims that share a colour with the roof keep it. Per palette: colours shown only on roof texels and entries no
    texture uses become up to SNOW_SHADES snow colours (Platinum's ramp), and the roof texels are spread over them by
    brightness, so the roof keeps its shading. On a clearly coloured roof, neutral trim texels and the darkest outline
    colour keep their colours. pal_cols = {offset: ([palette names], colours)}; a palette with no free slot stays."""
    pairs = masks.get('_pairs', {})
    overrides = masks.get('_overrides', {})
    texinfo = {name: (param, off) for name, param, off in textures(out)}
    for poff, (names, cols) in pal_cols.items():
        texs = sorted({t for pal in names for t in pairs.get(pal, []) if t in texinfo})
        ov = next((overrides[t] for t in texs if t in overrides), {})   # exceptions (data/seasons/roof_overrides.json)
        shades_max = ov.get('shades', SNOW_SHADES)
        lum_lo, lum_hi = ov.get('lum', SNOW_LUM)
        keep_cols = {int(c, 16) for c in ov.get('keep', [])}
        roofs = []
        used, other, other_n, roofmask = set(), set(), {}, {}
        reach = len(cols)   # entries every texture shown with this palette can point at (16 for 4bpp, 4 for 2bpp)
        for t in texs:
            param, off = texinfo[t]
            fmt = (param >> 26) & 7
            if fmt not in _INDEXED:
                continue
            w, h = 8 << ((param >> 20) & 7), 8 << ((param >> 23) & 7)
            bpp, imask = _INDEXED[fmt]
            reach = min(reach, imask + 1)
            get, put = _texel_io(out, off, bpp, imask)
            m = masks.get(t)
            flat = m[3] if m and (m[0], m[1]) == (w, h) and overrides.get(t, {}).get('snow', True) else None
            roofmask[t] = flat
            tr0 = (param >> 29) & 1
            for i in range(w * h):
                k = get(i)
                used.add(k)
                if flat and flat[i] and not (tr0 and k == 0):
                    roofs.append((t, i, k))
                else:
                    other.add(k)
                    other_n[k] = other_n.get(k, 0) + 1
        if not roofs:
            continue
        roof_ks = {k for _, _, k in roofs if k < len(cols)}
        sat = sum(_hsv(cols[k])[1] for _, _, k in roofs if k < len(cols)) / len(roofs)
        hexrgb = lambda c: int('%02x%02x%02x' % tuple(round(x * 255) for x in _rgb(c & 0x7FFF)), 16)
        keep = {k for k in roof_ks if hexrgb(cols[k]) in keep_cols}
        if sat > 0.25 and ov.get('trim', True):   # a coloured roof: grey/white trim and the darkest outline keep their colours
            keep |= {k for k in roof_ks if _hsv(cols[k])[1] < NEUTRAL_TRIM}
            dark = sorted((k for k in roof_ks - keep), key=lambda k: _hsv(cols[k])[2])
            if len(dark) >= 3 and _hsv(cols[dark[0]])[2] < 0.35:
                keep.add(dark[0])
        # a roof colour shown only a little off the roof (edges, a few trim texels): those texels take the nearest other
        # colour, so the roof colour becomes free for snow
        roof_n = {k: 0 for k in roof_ks}
        for _, _, k in roofs:
            if k in roof_n:
                roof_n[k] += 1
        for k in sorted(roof_ks & other - keep):
            if k >= reach or other_n[k] > max(STRAY_MAX, STRAY_SHARE * roof_n[k]):
                continue
            rest = [j for j in used if j != k and 0 < j < reach and (j not in roof_ks or j in keep or other_n.get(j, 0) > max(STRAY_MAX, STRAY_SHARE * roof_n.get(j, 0)))]
            if not rest:
                continue
            j = min(rest, key=lambda j: _dist(cols[j], cols[k]))
            for t in texs:
                param, off = texinfo[t]
                fmt = (param >> 26) & 7
                if fmt not in _INDEXED:
                    continue
                bpp, imask = _INDEXED[fmt]
                get, put = _texel_io(out, off, bpp, imask)
                rm = roofmask.get(t)
                for i in range((8 << ((param >> 20) & 7)) * (8 << ((param >> 23) & 7))):
                    if get(i) == k and not (rm and rm[i]):
                        put(i, j)
            other.discard(k)
        snow = sorted(((roof_ks - other - keep) | {k for k in range(1, reach) if k not in used}) & set(range(reach)))[:shades_max]
        # too few: free entries by merging colours that look the same (every texel of one points at the other)
        while len(snow) < shades_max:
            cand = [k for k in sorted(used) if 0 < k < reach and k not in snow]
            best = min(((_dist(cols[a], cols[b]), a, b) for a in cand for b in cand if a != b), default=None)
            if not best or best[0] > MERGE_MAX:
                break
            _, a, b = best
            for t in texs:
                param, off = texinfo[t]
                fmt = (param >> 26) & 7
                if fmt in _INDEXED:
                    bpp, imask = _INDEXED[fmt]
                    get, put = _texel_io(out, off, bpp, imask)
                    for i in range((8 << ((param >> 20) & 7)) * (8 << ((param >> 23) & 7))):
                        if get(i) == a:
                            put(i, b)
            roofs = [(t, i, b if k == a else k) for t, i, k in roofs]
            used.discard(a)
            keep.discard(a)
            snow.append(a)
        # the palette's own whites and greys stand in as shades when there are few new ones (they don't change)
        light = [] if len(snow) >= 3 or 'shades' in ov else [k for k in range(1, reach) if k in used and k not in snow
                                          and _hsv(cols[k])[1] < 0.08 and _hsv(cols[k])[2] >= 0.55]
        if not snow and not light:
            continue
        n = len(snow)
        for j, k in enumerate(snow):   # new snow colours, soft contrast (Platinum's ramp, upper part)
            r, g, b = _ramp(WINTER_RAMP, SNOW_LOW + (0.97 - SNOW_LOW) * ((j / (n - 1)) ** 1.3 if n > 1 else 0.6))
            struct.pack_into('<H', out, poff + 2 * k, (cols[k] & 0x8000) | _bgr555(r, g, b))
        cur = [struct.unpack_from('<H', out, poff + 2 * k)[0] for k in range(len(cols))]
        shades = sorted(set(snow) | set(light), key=lambda k: _hsv(cur[k])[2])
        # roof texels: their colours, darkest to brightest, spread over the snow shades by their share of the roof,
        # so neighbouring shades of the roof stay apart and the main colour lands in the middle
        change = {k for _, _, k in roofs if k < len(cols) and k not in keep}
        if not change:
            continue
        lum = lambda c: sum(w * x for w, x in zip((0.3, 0.59, 0.11), _rgb(c & 0x7FFF)))
        order = sorted(change, key=lambda k: lum(cols[k]))
        area = {k: 0 for k in order}
        for _, _, k in roofs:
            if k in area:
                area[k] += 1
        total, before, rank, m = sum(area.values()), 0, {}, len(shades)
        for k in order:   # by the middle of its share of the roof's area, so the main colour sits mid-way
            pos = (before + area[k] / 2) / total
            before += area[k]
            want = lum_lo + (lum_hi - lum_lo) * pos
            rank[k] = min(shades, key=lambda j: abs(lum(cur[j]) - want))
        for t, i, k in roofs:
            if k in rank:
                param, off = texinfo[t]
                bpp, imask = _INDEXED[(param >> 26) & 7]
                _texel_io(out, off, bpp, imask)[1](i, rank[k])


def recolor_btx(data, season, kind='map', roofs=None, masks=None):
    """A copy of a BTX0 file for a season. kind 'map': greens follow the season (recolor()), flowers wilt in autumn and
    winter. kind 'building': only flower planters wilt (autumn, winter) and, in winter, roofs get snow texel by texel
    (masks = load_masks(data/seasons/roof_masks.json), snow_roof_texels). roofs is unused (kept for old callers)."""
    out = bytearray(data)
    if out[:4] != b'BTX0':
        return bytes(out)   # an empty placeholder member
    for off, n, name in palettes(out):
        cols = [struct.unpack_from('<H', out, off + 2 * k)[0] for k in range(n)]
        wilting = season in (AUTUMN, WINTER) and _flowery(name, cols)
        if wilting:
            cols = wilt(cols, season, ground=name.startswith('flower'))
        if kind == 'map' and season == WINTER and name.startswith(PATHS):
            cols = dusted(cols)
        elif kind == 'map' and name.startswith(BEACHES):
            cols = [recolor(c, season) if _is_green(c) else c for c in cols]
        elif kind == 'map' and name.startswith(MAPLES):
            cols = [recolor(c, season, _is_leaf(c)) for c in cols]
        elif kind == 'map' or wilting:
            cols = [recolor(c, season) for c in cols]
        for k, c in enumerate(cols):
            struct.pack_into('<H', out, off + 2 * k, c)
    if kind == 'building' and season == WINTER and masks:
        cur = {}   # names that share an offset are one palette: plan them together
        for off, n, name in palettes(out):
            cur.setdefault(off, ([], [struct.unpack_from('<H', out, off + 2 * k)[0] for k in range(n)]))[0].append(name)
        snow_roof_texels(out, masks, cur)
    return bytes(out)
