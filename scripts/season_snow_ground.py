"""Which plain ground tiles are snow in winter (SEASONAL_MAPS): writes data/seasons/snow_ground.bin
    season_snow_ground.py [preview dir]

Plain ground has one tile type (0x00) whether it is a dirt road or grass, so the game can't tell them apart. This
renders every map chunk of the seasonal areas from above (the Script Inspector's maprender.py, DSPRE project, summer
textures, no buildings) and marks a walkable 0x00 tile when at least half of its pixels are green, i.e. turn into snow
in winter (season_recolor's test). Paths, sand, stone and flower beds stay unmarked. src/seasons.c plays the snow
footstep on marked tiles in winter. Output: 128 bytes (1 bit per tile, bit = z * 32 + x) per map file number, all map
files up to the highest seasonal one; build_seasons.py appends it to a/1/0/3. Re-run after editing maps in DSPRE.
With a preview dir, writes one PNG per area with the marked tiles tinted blue."""
import os
import struct
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import season_gallery as G  # noqa: E402
import season_recolor as R  # noqa: E402

M = G.M
OUT = os.path.join(REPO, 'data', 'seasons', 'snow_ground.bin')
SHARE = 0.5


def green_mask(rgb):
    """Per pixel: green by season_recolor's test (rgb floats 0-1)."""
    mx, mn = rgb.max(2), rgb.min(2)
    d = mx - mn
    s = np.where(mx > 0, d / np.maximum(mx, 1e-9), 0)
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    h = np.where(d == 0, 0,
                 np.where(mx == r, ((g - b) / np.maximum(d, 1e-9)) % 6,
                          np.where(mx == g, (b - r) / np.maximum(d, 1e-9) + 2, (r - g) / np.maximum(d, 1e-9) + 4))) * 60
    return (h >= 60) & (h <= 185) & (s > 0.12) & (((h >= 85) & (h <= 165)) | (s > 0.25))


def ground(renderer, map_id, map_set):
    """Top-down canvas of a map's own model (no buildings) with its summer textures."""
    _, _, mdl = M.split_map_file(renderer.file('maps', '%04d' % map_id))
    canvas = M.Canvas()
    model, own = M.load_model(mdl)
    if model:
        tris = model.triangles()
        ts = own + renderer.texset('mapTextures', map_set)
        for translucent in (False, True):
            canvas.draw(tris, model.materials, ts, translucent)
    return canvas


def tiles(renderer, map_id):
    d = renderer.file('maps', '%04d' % map_id)
    p = 20 + struct.unpack_from('<H', d, 18)[0]
    return struct.unpack_from('<1024H', d, p)


def main():
    preview = sys.argv[1] if len(sys.argv) > 1 else None
    sets = set(G.seasonal_sets())
    arm9 = G.load_arm9()
    renderer = M.Renderer(G.PROJECT)
    maps = {}   # map file -> map texture set
    for h, name, area, map_set in G.outdoor_headers(arm9):
        if map_set not in sets:
            continue
        _, _, _, cells = M.header_chunks(G.PROJECT, arm9, h)
        for _c, _r, mp in cells:
            maps.setdefault(mp, (map_set, name))
    bits = {}
    for mp, (map_set, name) in sorted(maps.items()):
        cv = ground(renderer, mp, map_set)
        green = green_mask(cv.rgb) & (cv.a > 0.5)
        share = green.reshape(32, 16, 32, 16).mean(axis=(1, 3))
        t = tiles(renderer, mp)
        b = bytearray(128)
        n = 0
        for z in range(32):
            for x in range(32):
                v = t[z * 32 + x]
                if (v & 0xFF) == 0 and not (v >> 8) & 0x80 and share[z, x] >= SHARE:
                    i = z * 32 + x
                    b[i >> 3] |= 1 << (i & 7)
                    n += 1
        bits[mp] = bytes(b)
        print('map %4d (%s, set %d): %d snow tiles' % (mp, name, map_set, n), flush=True)
        if preview:
            from PIL import Image
            img = (np.clip(cv.rgb, 0, 1) * 255).astype(np.uint8).copy()
            for z in range(32):
                for x in range(32):
                    i = z * 32 + x
                    if b[i >> 3] >> (i & 7) & 1:
                        blk = img[z * 16:z * 16 + 16, x * 16:x * 16 + 16].astype(float)
                        img[z * 16:z * 16 + 16, x * 16:x * 16 + 16] = (blk * 0.5 + np.array([60, 110, 255]) * 0.5)
            os.makedirs(preview, exist_ok=True)
            Image.fromarray(img).save(os.path.join(preview, '%s_%04d.png' % (name, mp)))
    top = max(bits)
    with open(OUT, 'wb') as f:
        for mp in range(top + 1):
            f.write(bits.get(mp, bytes(128)))
    print('%d maps, %d bytes -> %s' % (len(bits), (top + 1) * 128, OUT))


if __name__ == '__main__':
    main()
