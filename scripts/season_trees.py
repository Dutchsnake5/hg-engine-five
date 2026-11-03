"""Autumn trees in several colours (SEASONAL_MAPS).

Every ordinary tree on a map draws from one tree texture, so recolouring it gives every tree the same colour. In
autumn, the tree textures (TREE_TEXTURES) of each seasonal map texture set are replaced by a 64 x 256, 256-colour
texture holding VARIANTS colour versions stacked from top to bottom (the strip texture used for rows of trees holds two
trees per version, coloured differently), and src/seasons.c moves each tree quad of a map model onto one version when
the model is loaded (its texture coordinate V + 64 * version; every tree quad uses V 0..64). The texture keeps its
original size in its dictionary entry (extraParam), so NNS doesn't scale the coordinates. This module does both halves
for the build (autumn_trees) and for previews (patch_model, the same placement as the game's)."""
import colorsys
import struct

import season_recolor as R

TREE_TEXTURES = (b'tree01gs', b'tree01_re', b'treekn', b'treekn_re',
                 b'd05tree01', b'd05tree01_re',   # Ilex Forest
                 b'dtf_f09', b'dtf_f10')          # Viridian Forest
STRIPS = (b'tree01_re', b'treekn_re', b'd05tree01_re', b'dtf_f10')   # two trees side by side per 64 texels
STRIP_SINGLE = {b'tree01_re': b'tree01gs', b'treekn_re': b'treekn', b'd05tree01_re': b'd05tree01',
                b'dtf_f10': b'dtf_f09'}   # the strip = this tree at x 16 and 48
VARIANTS = 4
# (hue from, hue to) in degrees, dark foliage to light, then saturation and brightness factors
STYLES = [
    ((352, 8), 0.82, 0.95),    # red
    ((16, 30), 0.88, 1.02),    # orange
    ((22, 44), 0.92, 1.08),    # gold (warm brown shadows: dark yellow reads olive)
    ((28, 56), 0.88, 1.14),    # yellow
]
OUTLINE_HUE, OUTLINE_SAT = 18, 0.40
STRIP_RIGHT = 2   # the strip's right-hand tree takes version (k + STRIP_RIGHT) % VARIANTS


def shade_colours(cols):
    """Palette indices of cool shading colours on the lower leaves: Ilex's and Viridian's trees shade their foliage with
    navy blues, the darkest of which is the outline. Those should follow the leaves (they made a grey-brown band as
    outline colour)."""
    hsv = {i: colorsys.rgb_to_hsv(*R._rgb(c & 0x7FFF)) for i, c in enumerate(cols) if i}
    cool = [i for i, (h, s, v) in hsv.items() if 180 <= h * 360 <= 270 and s > 0.2 and not R._green_hsv(h * 360, s)]
    if len(cool) < 2:
        return set()
    darkest = min(cool, key=lambda i: hsv[i][2])
    return {i for i in cool if i != darkest}


def leaf_colour(c, style, shade=False):
    """A summer foliage colour (BGR555) in an autumn colour: hue by brightness (darker = redder), like the leaves of
    one tree catching the light; the dark outline greys get one brown; shade (see shade_colours) = a deep leaf colour."""
    r, g, b = R._rgb(c & 0x7FFF)
    h, s, v = colorsys.rgb_to_hsv(r, g, b)
    (h0, h1), sf, vf = STYLES[style]
    if shade:
        r, g, b = colorsys.hsv_to_rgb(h0 / 360, min(1.0, sf), min(1.0, v * vf * 0.88))
    elif R._green_hsv(h * 360, s):
        t = max(0.0, min(1.0, (v - 0.40) / 0.40))
        hue = (h0 + ((h1 - h0) % 360) * t) % 360
        r, g, b = colorsys.hsv_to_rgb(hue / 360, min(1.0, sf), min(1.0, v * vf))
    else:   # the outline: one dark brown for every version (where trees overlap, one tree's outline lies on the other)
        r, g, b = colorsys.hsv_to_rgb(OUTLINE_HUE / 360, OUTLINE_SAT, v * 0.9)
    return (c & 0x8000) | R._bgr555(r, g, b)


# ---------------------------------------------------------------- the texture block (TEX0)
def _dict(data, off):
    """[(name, entry position, entry size)] of an NNS dictionary."""
    _, count, _, _, ofs_entry = struct.unpack_from('<BBHHH', data, off)
    p = off + ofs_entry
    size, names = struct.unpack_from('<HH', data, p)
    return [(bytes(data[p + names + 16 * i:p + names + 16 * i + 16]).rstrip(b'\0'), p + 4 + size * i, size) for i in range(count)]


def _tex_bytes(param):
    w, h, fmt = 8 << ((param >> 20) & 7), 8 << ((param >> 23) & 7), (param >> 26) & 7
    return w * h * {1: 8, 2: 2, 3: 4, 4: 8, 6: 8, 7: 16}[fmt] // 8


def strip_owner(summer, st, s_texes, name, idx):
    """For each texel of a strip texture: True if it belongs to the right-hand tree. The strip is the single-tree
    texture drawn at x 16 and x 48 (overlapping, wrapping every 64): a texel matching only one copy is that tree's;
    where both copies are opaque, the tree drawn in front (the one winning where they differ) owns it."""
    single = STRIP_SINGLE.get(name)
    if single not in s_texes:
        return [x >= 32 for y in range(64) for x in range(64)]
    sparam = struct.unpack_from('<I', summer, s_texes[single])[0]
    o = st + struct.unpack_from('<I', summer, st + 0x14)[0] + ((sparam & 0xFFFF) << 3)
    one = [(summer[o + i // 2] >> (4 * (i & 1))) & 15 for i in range(64 * 64)]
    left = lambda y, x: one[y * 64 + (x + 16) % 64]    # the tree centred at 16
    rite = lambda y, x: one[y * 64 + (x + 48) % 64]    # the tree centred at 48
    front_left = front_right = 0
    for y in range(64):
        for x in range(64):
            i, a, b = idx[y * 64 + x], left(y, x), rite(y, x)
            if i and a and b and a != b:
                front_left += i == a
                front_right += i == b
    own = {}   # texel -> True (right tree) / False (left tree); undecided ones (both copies agree) settled after
    for y in range(64):
        for x in range(64):
            i, a, b = idx[y * 64 + x], left(y, x), rite(y, x)
            if not i:
                continue
            if a and i == a and not (b and i == b):
                own[(x, y)] = False
            elif b and i == b and not (a and i == a):
                own[(x, y)] = True
            elif a and b and i == a == b:
                own[(x, y)] = None
            elif a and b:
                own[(x, y)] = front_right > front_left
            elif a or b:
                own[(x, y)] = bool(b)
            else:
                own[(x, y)] = abs((x - 48 + 32) % 64 - 32) < abs((x - 16 + 32) % 64 - 32)
    for _ in range(8):   # an undecided texel takes the tree most of its decided neighbours belong to
        for (x, y), o in list(own.items()):
            if o is None:
                votes = [own.get(((x + dx) % 64, y + dy)) for dx in (-1, 0, 1) for dy in (-1, 0, 1) if dx or dy]
                votes = [v for v in votes if v is not None]
                if votes:
                    own[(x, y)] = sum(votes) * 2 > len(votes) if sum(votes) * 2 != len(votes) else front_right > front_left
    return [bool(own.get((x, y))) for y in range(64) for x in range(64)]


def autumn_trees(summer, autumn):
    """The autumn texture set (BTX0 bytes) with its tree textures replaced by the stacked colour versions, made from
    the summer set's textures. Unchanged when the set has none."""
    if summer[:4] != b'BTX0':
        return autumn
    st = struct.unpack_from('<I', summer, 0x10)[0]
    d = bytearray(autumn)
    t = struct.unpack_from('<I', d, 0x10)[0]
    tex_dict, pal_dict = struct.unpack_from('<H', d, t + 0x0E)[0], struct.unpack_from('<H', d, t + 0x34)[0]
    if not tex_dict or not pal_dict:
        return autumn
    texes = _dict(d, t + tex_dict)
    s_texes = {n: p for n, p, _ in _dict(summer, st + struct.unpack_from('<H', summer, st + 0x0E)[0])}
    pals = {n: p for n, p, _ in _dict(d, t + pal_dict)}
    s_pals = {n: p for n, p, _ in _dict(summer, st + struct.unpack_from('<H', summer, st + 0x34)[0])}
    wanted = [(n, p) for n, p, _ in texes if n in TREE_TEXTURES]
    if not wanted:
        return autumn
    tex_off, cmp_size = struct.unpack_from('<I', d, t + 0x14)[0], struct.unpack_from('<H', d, t + 0x1C)[0] << 3
    cmp_off, cmp_idx, pal_size, pal_off = (struct.unpack_from('<I', d, t + 0x24)[0], struct.unpack_from('<I', d, t + 0x28)[0],
                                           struct.unpack_from('<H', d, t + 0x30)[0] << 3, struct.unpack_from('<I', d, t + 0x38)[0])
    tex_size = struct.unpack_from('<H', d, t + 0x0C)[0] << 3
    assert tex_dict < tex_off and pal_dict < tex_off, 'texture set layout'
    assert tex_off + tex_size <= cmp_off <= cmp_idx <= pal_off or cmp_size == 0, 'texture set layout'

    # normal texture data blocks in their original order (entries sharing data keep sharing it)
    blocks = {}
    for n, p, _ in texes:
        param = struct.unpack_from('<I', d, p)[0]
        if (param >> 26) & 7 == 5:
            continue
        o = (param & 0xFFFF) << 3
        blocks.setdefault(o, bytes(d[t + tex_off + o:t + tex_off + o + _tex_bytes(param)]))
    new_pals = {}
    replaced = {}
    for n, p in wanted:
        sp = s_texes[n]
        sparam = struct.unpack_from('<I', summer, sp)[0]
        w, h, fmt = 8 << ((sparam >> 20) & 7), 8 << ((sparam >> 23) & 7), (sparam >> 26) & 7
        if fmt != 3 or w != 64 or h != 64:
            continue
        pal_name = n if n in s_pals else (b'tree01' if n == b'tree01gs' else n + b'_pl')
        if pal_name not in s_pals or pal_name not in pals:
            continue
        s_tex = st + struct.unpack_from('<I', summer, st + 0x14)[0] + ((sparam & 0xFFFF) << 3)
        idx = [(summer[s_tex + i // 2] >> (4 * (i & 1))) & 15 for i in range(64 * 64)]
        s_pal = st + struct.unpack_from('<I', summer, st + 0x38)[0] + (struct.unpack_from('<H', summer, s_pals[pal_name])[0] << 3)
        cols = struct.unpack_from('<16H', summer, s_pal)
        pal = [0] * 256
        shades = shade_colours(cols)
        for k in range(VARIANTS):
            for i in range(1, 16):
                pal[k * 16 + i] = leaf_colour(cols[i], k, i in shades)
        right = [False] * (64 * 64)
        if n in STRIPS:
            right = strip_owner(summer, st, s_texes, n, idx)
        out = bytearray(64 * 256)
        for k in range(VARIANTS):
            for y in range(64):
                for x in range(64):
                    i = idx[y * 64 + x]
                    if i:
                        v = (k + STRIP_RIGHT) % VARIANTS if right[y * 64 + x] else k
                        out[(k * 64 + y) * 64 + x] = v * 16 + i
        # 8 bpp, 256 rows; keep repeat/flip/colour-0 flags; the dictionary's original size (extraParam) stays 64 x 64
        nparam = (sparam & ~((7 << 23) | (7 << 26) | 0xFFFF)) | (5 << 23) | (4 << 26)
        replaced[n] = (p, nparam, bytes(out))
        new_pals[pal_name] = struct.pack('<256H', *pal)

    if not replaced:
        return autumn
    # lay the texture data out again: old blocks (minus replaced trees no other entry uses), then the new textures
    used_by = {}
    for n, p, _ in texes:
        param = struct.unpack_from('<I', d, p)[0]
        if (param >> 26) & 7 != 5:
            used_by.setdefault((param & 0xFFFF) << 3, set()).add(n)
    data = bytearray()
    where = {}
    for o in sorted(blocks):
        if used_by[o] <= set(replaced):
            continue
        where[o] = len(data)
        data += blocks[o]
        data += bytes(-len(data) % 8)
    for n, p, _ in texes:
        param = struct.unpack_from('<I', d, p)[0]
        if (param >> 26) & 7 == 5:
            continue
        if n in replaced:
            _, nparam, texels = replaced[n]
            off = len(data)
            data += texels
            data += bytes(-len(data) % 8)
            param = nparam
        else:
            off = where[(param & 0xFFFF) << 3]
        struct.pack_into('<I', d, p, (param & ~0xFFFF) | (off >> 3))
    # palettes: the old block, then the new 256-colour ones
    pal_data = bytearray(d[t + pal_off:t + pal_off + pal_size])
    for name, raw in new_pals.items():
        pal_data += bytes(-len(pal_data) % 8)
        struct.pack_into('<H', d, pals[name], len(pal_data) >> 3)
        pal_data += raw
    assert len(data) < 0x80000 and len(pal_data) < 0x80000
    cmp = bytes(d[t + cmp_off:t + cmp_off + cmp_size]) if cmp_size else b''
    cmp_i = bytes(d[t + cmp_idx:t + cmp_idx + cmp_size // 2]) if cmp_size else b''
    head = d[t:t + tex_off]
    body = bytearray(head)
    body += data
    n_cmp = len(body)
    body += cmp
    n_idx = len(body)
    body += cmp_i
    body += bytes(-len(body) % 8)
    n_pal = len(body)
    body += pal_data
    struct.pack_into('<H', body, 0x0C, len(data) >> 3)
    struct.pack_into('<I', body, 0x24, n_cmp if cmp_size else n_cmp)
    struct.pack_into('<I', body, 0x28, n_idx if cmp_size else n_idx)
    struct.pack_into('<H', body, 0x30, len(pal_data) >> 3)   # u16 size, then a flag
    struct.pack_into('<I', body, 0x38, n_pal)
    struct.pack_into('<I', body, 0x04, len(body))
    out = bytearray(d[:t]) + body
    struct.pack_into('<I', out, 0x08, len(out))
    return bytes(out)


# ---------------------------------------------------------------- placing trees on versions (src/seasons.c mirror)
GX_PARAMS = {0x00: 0, 0x10: 1, 0x11: 0, 0x12: 1, 0x13: 1, 0x14: 1, 0x15: 0, 0x16: 16, 0x17: 12, 0x18: 16, 0x19: 12,
             0x1A: 9, 0x1B: 3, 0x1C: 3, 0x20: 1, 0x21: 1, 0x22: 1, 0x23: 2, 0x24: 1, 0x25: 1, 0x26: 1, 0x27: 1,
             0x28: 1, 0x29: 1, 0x2A: 1, 0x2B: 1, 0x30: 1, 0x31: 1, 0x32: 1, 0x33: 1, 0x34: 32, 0x40: 1, 0x41: 0,
             0x50: 1, 0x60: 1, 0x70: 3, 0x71: 2, 0x72: 1}
M32 = 0xFFFFFFFF


def _s(v, bits):
    v &= (1 << bits) - 1
    return v - (1 << bits) if v & (1 << (bits - 1)) else v


def variant(x, z, seed):
    """The version (0..VARIANTS-1) of a group of trees whose leftmost piece starts at x, z (model units, fx12)."""
    h = (((x >> 12) * 0x9E3779B1) ^ ((z >> 12) * 0x85EBCA77) ^ (seed * 0xC2B2AE3D)) & M32
    h ^= h >> 16                       # murmur3's finaliser: positions are whole units, so mix well
    h = (h * 0x85EBCA6B) & M32
    h ^= h >> 13
    h = (h * 0xC2B2AE35) & M32
    h ^= h >> 16
    return (h >> 16) % VARIANTS


ROW = 4 << 12     # pieces whose bases are this close (model units, fx12) stand in one row
TOUCH = 2 << 12   # and this close side by side touch


def _quads(d, start, size, strip, scale):
    """The quads of a tree shape's display list: dicts with the positions of their texture coordinate words, their
    x range and base z (model units, fx12) and whether they are a piece of a tree (a strip piece, or a part of the
    single-tree texture) rather than a whole tree."""
    out = []
    p, end = start, start + size
    x = z = 0
    tc = None
    quad = []
    prim = -1
    while p + 4 <= end:
        cmds = d[p:p + 4]
        p += 4
        for c in cmds:
            n = GX_PARAMS.get(c)
            if n is None:
                return out
            w0 = struct.unpack_from('<I', d, p)[0] if n else 0
            if c == 0x40:
                prim, quad = w0 & 3, []
            elif c == 0x22:
                tc = p
            elif c in (0x23, 0x24, 0x25, 0x26, 0x27, 0x28):
                if c == 0x23:
                    x, z = _s(w0, 16), _s(struct.unpack_from('<I', d, p + 4)[0], 16)
                elif c == 0x24:
                    x, z = _s(w0, 10) << 6, _s(w0 >> 20, 10) << 6
                elif c == 0x25:
                    x = _s(w0, 16)
                elif c == 0x26:
                    x, z = _s(w0, 16), _s(w0 >> 16, 16)
                elif c == 0x27:
                    z = _s(w0 >> 16, 16)
                else:
                    x, z = x + _s(w0, 10), z + _s(w0 >> 20, 10)
                if prim == 1 and tc is not None:
                    quad.append((tc, x * scale >> 12, z * scale >> 12))
                    if len(quad) == 4:
                        us = [_s(struct.unpack_from('<I', d, q[0])[0], 16) for q in quad]
                        ia, ib = us.index(min(us)), us.index(max(us))
                        out.append({'tc': sorted({q[0] for q in quad}),
                                    'x0': min(q[1] for q in quad), 'x1': max(q[1] for q in quad),
                                    'zb': max(q[2] for q in quad),
                                    'ua': us[ia], 'ub': us[ib], 'xa': quad[ia][1], 'xb': quad[ib][1],
                                    'strip': strip, 'piece': strip or min(us) != 0 or max(us) != 64 * 16})
                        quad = []
            p += 4 * n
    return out


def place(quads, seed):
    """Version per quad: pieces in one row that touch, at least one of them a piece of a tree, share their group's
    version (a tree can be drawn as the end of one quad and the start of the next); whole trees each get their own."""
    n = len(quads)
    parent = list(range(n))

    def root(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    for i in range(n):
        a = quads[i]
        for j in range(i + 1, n):
            b = quads[j]
            if (a['piece'] or b['piece']) and abs(a['zb'] - b['zb']) <= ROW \
                    and a['x0'] <= b['x1'] + TOUCH and b['x0'] <= a['x1'] + TOUCH:
                ri, rj = root(i), root(j)
                if ri != rj:
                    parent[max(ri, rj)] = min(ri, rj)
    first = {}
    for i in range(n):
        r = root(i)
        q = quads[i]
        if r not in first or (q['x0'], q['zb']) < first[r]:
            first[r] = (q['x0'], q['zb'])
    out = []
    for i in range(n):
        q = quads[i]
        k = variant(first[root(i)][0], first[root(i)][1], seed)
        if not q['strip'] and q['ub'] != q['ua']:
            # a single tree in a group with strip pieces: the nearest strip's texture, carried on to the tree's middle,
            # says whether it is one of the strip's left trees or right trees
            xc = q['xa'] + (32 * 16 - q['ua']) * (q['xb'] - q['xa']) // (q['ub'] - q['ua'])
            best = None
            for j in range(n):
                t = quads[j]
                if t['strip'] and root(j) == root(i) and t['xb'] != t['xa']:
                    dist = max(0, min(t['xa'], t['xb']) - xc, xc - max(t['xa'], t['xb']))
                    if best is None or dist < best[0]:
                        best = (dist, j)
            if best is not None:
                t = quads[best[1]]
                u = t['ua'] + (xc - t['xa']) * (t['ub'] - t['ua']) // (t['xb'] - t['xa'])
                k = (k + STRIP_RIGHT * ((u // (32 * 16)) & 1)) % VARIANTS
        out.append(k)
    return out


def _move(d, quad, k):
    """Puts a quad's texture coordinates on version k (keeps where it was within the 64 rows: repeatable)."""
    ts = [struct.unpack_from('<I', d, q)[0] >> 16 for q in quad['tc']]
    base = (min(ts) // 1024) * 1024
    for q, tv in zip(quad['tc'], ts):
        w = struct.unpack_from('<I', d, q)[0]
        struct.pack_into('<I', d, q, (w & 0xFFFF) | (((tv - base + k * 1024) & 0xFFFF) << 16))


def patch_model(bmd):
    """The map model (BMD0 bytes) with every tree quad moved onto its group's colour version, as src/seasons.c does."""
    d = bytearray(bmd)
    if d[:4] != b'BMD0':
        return bmd
    mdl0 = struct.unpack_from('<I', d, 0x10)[0]
    m = mdl0 + struct.unpack_from('<I', d, _dict(d, mdl0 + 8)[0][1])[0]
    size, ofs_sbc, ofs_mat, ofs_shp = struct.unpack_from('<4I', d, m)
    base = m + ofs_mat
    scale = struct.unpack_from('<i', d, m + 0x1C)[0]
    is_tree, is_strip, quads = set(), set(), []
    for n, e, _ in _dict(d, base + struct.unpack_from('<H', d, base)[0]):
        if n in TREE_TEXTURES:
            lst, num = struct.unpack_from('<HB', d, e)
            is_tree.update(d[base + lst:base + lst + num])
            if n in STRIPS:
                is_strip.update(d[base + lst:base + lst + num])
    if not is_tree:
        return bmd
    shapes = [m + ofs_shp + struct.unpack_from('<I', d, e)[0] for _, e, _ in _dict(d, m + ofs_shp)]
    p, end, mat, done = m + ofs_sbc, m + ofs_mat, None, set()
    while p < end:
        op = d[p]
        c, var = op & 0x1F, op >> 5
        if c == 0x00:
            p += 1
        elif c == 0x01:
            break
        elif c == 0x02:
            p += 3
        elif c == 0x03:
            p += 2
        elif c == 0x04:
            mat = d[p + 1]
            p += 2
        elif c == 0x05:
            s = d[p + 1]
            if mat in is_tree and s not in done and s < len(shapes):
                done.add(s)
                hdr = shapes[s]
                dl_rel, dl_size = struct.unpack_from('<II', d, hdr + 8)
                quads += _quads(d, hdr + dl_rel, dl_size, mat in is_strip, scale)
            p += 2
        elif c == 0x06:
            p += 4 + (var & 1) + ((var >> 1) & 1)
        elif c in (0x07, 0x08):
            p += 2 + (var & 1) + ((var >> 1) & 1)
        elif c == 0x09:
            p += 3 + d[p + 2] * 3
        elif c == 0x0B:
            p += 1
        elif c in (0x0C, 0x0D):
            p += 3
        else:
            break
    for q, k in zip(quads, place(quads, size)):
        _move(d, q, k)
    return bytes(d)
