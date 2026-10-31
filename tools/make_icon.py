#!/usr/bin/env python3
"""Groove Grabber icon: the claw + snare zoomed in so they read at small sizes, inside a neon rounded frame
(blue left, orange right, purple at the bottom) with clear corners.

    python3 tools/make_icon.py <square source art.png> Resources/icon.png Resources/badge_256.png
"""
import sys
from PIL import Image, ImageDraw, ImageFilter, ImageEnhance

SRC, OUT_BIG, OUT_BADGE = sys.argv[1], sys.argv[2], sys.argv[3]
N = 1024
CROP = (110, 40, 914, 844)          # the claw and the drum, out of the 1024 source art
INSET, RADIUS, LINE = 20, 196, 9    # the frame

src = Image.open(SRC).convert('RGB').resize((1024, 1024), Image.LANCZOS)
art = src.crop(CROP).resize((N, N), Image.LANCZOS)
art = ImageEnhance.Contrast(art).enhance(1.18)
art = ImageEnhance.Color(art).enhance(1.12)

# darken the edges a little so the claw and drum stand out
vig = Image.new('L', (N, N), 0)
d = ImageDraw.Draw(vig)
for i in range(40):
    d.ellipse([-180 + i * 9, -120 + i * 9, N + 180 - i * 9, N + 120 - i * 9], fill=int(255 * i / 40))
vig = vig.filter(ImageFilter.GaussianBlur(60))
art = Image.composite(art, ImageEnhance.Brightness(art).enhance(0.35), vig)

# the neon colours around the frame
def colour(t, v):
    if t < 0.5:
        k = t * 2
        c = (int(30 + (170 - 30) * k), int(170 - (170 - 50) * k), 255)
    else:
        k = (t - 0.5) * 2
        c = (int(170 + (255 - 170) * k), int(50 + (130 - 50) * k), int(255 - (255 - 20) * k))
    if v > 0.72:
        k = (v - 0.72) / 0.28
        c = (int(c[0] * (1 - k) + 210 * k), int(c[1] * (1 - k) + 60 * k), int(c[2] * (1 - k) + 255 * k))
    return c

small = Image.new('RGB', (64, 64))
px = small.load()
for y in range(64):
    for x in range(64):
        px[x, y] = colour(x / 63, y / 63)
neon = small.resize((N, N), Image.BICUBIC)

S = 4
line = Image.new('L', (N * S, N * S), 0)
ImageDraw.Draw(line).rounded_rectangle([INSET * S, INSET * S, (N - INSET) * S, (N - INSET) * S],
                                       radius=RADIUS * S, outline=255, width=LINE * S)
line = line.resize((N, N), Image.LANCZOS)
glow = line.filter(ImageFilter.GaussianBlur(16))

img = Image.composite(neon, art, glow.point(lambda a: min(255, int(a * 1.1))))
img = Image.composite(neon, img, line)
core = line.filter(ImageFilter.GaussianBlur(1.2)).point(lambda a: int(a * 0.2))
img = Image.composite(Image.new('RGB', (N, N), (255, 255, 255)), img, core)

# clear corners just outside the frame's glow
alpha = Image.new('L', (N * S, N * S), 0)
o = INSET - 5
ImageDraw.Draw(alpha).rounded_rectangle([o * S, o * S, (N - o) * S, (N - o) * S], radius=(RADIUS + 5) * S, fill=255)
out = img.convert('RGBA')
out.putalpha(alpha.resize((N, N), Image.LANCZOS))
out.save(OUT_BIG, optimize=True)

# the in-app badge: smaller, with a touch of sharpening so it reads at header size
badge = out.resize((256, 256), Image.LANCZOS)
rgb = badge.convert('RGB').filter(ImageFilter.UnsharpMask(radius=1.2, percent=60, threshold=2))
badge = Image.merge('RGBA', (*rgb.split(), badge.split()[3]))
badge.save(OUT_BADGE, optimize=True)
print('wrote', OUT_BIG, OUT_BADGE)
