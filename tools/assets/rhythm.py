# The rhythm corner on the diagonal wall (docs/BAR_REDESIGN.md): a 4-key
# rhythm game cabinet in the spirit of osu!mania / Quaver (a glowing
# marquee, a big portrait screen with four lanes of falling notes, four big
# lit keys, start and back buttons, speakers with woofers, a subwoofer, a
# coin door, a card reader, LED edge strips, a headphone hook), plus a neon
# "RHYTHM" sign, speaker stacks, a beanbag and a small high table.
#
#   blender -b --factory-startup --python tools/assets/rhythm.py -- <texture dir> <out.glb>
#
# Model space: x along the diagonal wall, +z out of the wall into the room,
# the wall's middle at the origin, floor y = 0.
import math
import os
import sys

import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402
import props_lib  # noqa: E402

tex_dir, out_path = sys.argv[sys.argv.index("--") + 1:][:2]
common.reset_scene()
P = props_lib.Props(tex_dir, seed=55)

# ---- Images ----
LANE_COLS = [(0.85, 0.9, 1.0), (1.0, 0.35, 0.75), (1.0, 0.35, 0.75), (0.85, 0.9, 1.0)]


def game_screen(w=256, h=448):
    """Four lanes, notes falling to a judgement line, score and combo."""
    rng = np.random.default_rng(12)
    img = np.zeros((h, w, 3), np.float32)
    yy, xx = np.mgrid[0:h, 0:w]
    img[:] = np.stack([0.02 + 0.05 * yy / h, 0.01 + 0.02 * yy / h, 0.06 + 0.12 * yy / h], -1)
    lane_w, x0 = 40, (w - 160) // 2
    img[:, x0:x0 + 160] = (0.01, 0.01, 0.02)
    for k in range(5):
        img[:, x0 + k * lane_w - 1:x0 + k * lane_w + 1] = (0.25, 0.25, 0.35)
    judge = 70
    for k in range(4):
        lx = x0 + k * lane_w
        col = np.array(common.srgb(LANE_COLS[k]))
        img[judge - 6:judge + 6, lx + 3:lx + lane_w - 3] = col * 0.6  # receptors
        y = judge + 20 + rng.integers(0, 40)
        while y < h - 40:
            if rng.random() < 0.15:  # a long note
                ln = rng.integers(40, 110)
                img[y:min(h, y + ln), lx + 8:lx + lane_w - 8] = col * 0.55
                img[y:y + 10, lx + 3:lx + lane_w - 3] = col
                y += ln + 30
            else:
                img[y:y + 10, lx + 3:lx + lane_w - 3] = col
                y += rng.integers(25, 80)
    img[judge - 1:judge + 1, x0:x0 + 160] = 1.0
    # Key-press glow in two lanes.
    for k in (1, 2):
        lx = x0 + k * lane_w
        g = np.exp(-((yy - judge) / 40.0) ** 2)[..., None] * (np.abs(xx - (lx + lane_w / 2)) < lane_w / 2)[..., None]
        img += g * np.array(common.srgb(LANE_COLS[k])) * 0.5
    white = common.srgb((1, 1, 1))
    common.draw_text(img, "0987654", w - 6 - common.text_width("0987654", 2), h - 8, 2, white)
    common.draw_text(img, "98.76%", w - 6 - common.text_width("98.76%", 1), h - 28, 1, white)
    common.draw_text(img, "PERFECT", w // 2, judge + 110, 2, common.srgb((0.5, 0.9, 1.0)), center=True)
    common.draw_text(img, "128X", w // 2, judge + 150, 3, white, center=True)
    common.draw_text(img, "NIGHT DRIVE  [4K HARD]", 6, h - 8, 1, white)
    img[h - 24:h - 20, 6:6 + int((w - 12) * 0.64)] = common.srgb((0.4, 0.9, 1.0))  # song progress
    img[judge + 12:judge + 300, x0 - 10:x0 - 6] = common.srgb((0.2, 0.9, 0.3))  # health bar
    return np.clip(img, 0, 1)


def marquee_img(w=256, h=64):
    img = np.zeros((h, w, 3), np.float32)
    yy, xx = np.mgrid[0:h, 0:w]
    img[:] = np.stack([0.15 + 0.6 * xx / w, 0.05 + 0.0 * yy, 0.5 - 0.3 * xx / w], -1)
    common.draw_text(img, "4K MANIA", w // 2, 48, 4, common.srgb((1, 1, 1)), center=True)
    for k in range(24):  # little falling-note marks
        x = 6 + k * 10
        img[6:10, x:x + 6] = common.srgb((0.5, 0.95, 1.0)) if k % 3 else common.srgb((1.0, 0.5, 0.85))
    return img


def side_art(w=128, h=256):
    img = np.zeros((h, w, 3), np.float32)
    yy, xx = np.mgrid[0:h, 0:w] / np.array([h, w])[:, None, None]
    img[:] = np.stack([0.05 + 0.5 * yy, 0.02 + 0.1 * xx, 0.25 + 0.4 * (1 - yy)], -1)
    for k in range(4):
        x0 = int(w * (0.15 + 0.2 * k))
        img[:, x0:x0 + 3] = common.srgb(LANE_COLS[k])
        for y in range(10 + k * 37, h, 60):
            img[y:y + 8, x0 - 8:x0 + 11] = common.srgb(LANE_COLS[k])
    common.draw_text(img, "4K", w // 2, int(h * 0.25), 6, common.srgb((1, 1, 1)), center=True)
    return img


screen = common.glow_image_material("Rhythm screen", game_screen(), strength=1.5, base_scale=0.15)
marquee = common.glow_image_material("Marquee", marquee_img(), strength=2.0, base_scale=0.4)
art = common.glow_image_material("Cabinet art", side_art(), strength=0.25, base_scale=1.0)
body = common.plain_material("Cabinet black", (0.02, 0.02, 0.025), 0.2, 0.35)
trim = common.plain_material("Cabinet chrome", (0.85, 0.85, 0.88), 1.0, 0.15)
grill = common.plain_material("Speaker grill", (0.03, 0.03, 0.03), 0.5, 0.6)
cone_m = common.plain_material("Woofer cone", (0.06, 0.06, 0.065), 0.0, 0.6)
led_cyan = common.plain_material("LED cyan", (0.3, 0.9, 1.0), 0.0, 0.3, emission=(0.2, 0.85, 1.0), strength=2.4)
led_pink = common.plain_material("LED pink", (1.0, 0.3, 0.7), 0.0, 0.3, emission=(1.0, 0.2, 0.65), strength=2.4)
key_mats = [common.plain_material(f"Key {i}", c, 0.0, 0.25, emission=c, strength=1.8) for i, c in enumerate(LANE_COLS)]
coin_txt = np.zeros((16, 64, 3), np.float32)
common.draw_text(coin_txt, "INSERT COIN", 32, 12, 1, common.srgb((1.0, 0.3, 0.2)), center=True)
coin_mat = common.glow_image_material("Insert coin", coin_txt, strength=2.0)


def box(name, lo, hi, mat, bevel=0.006, seg=2):
    return common.box(name, lo, hi, mat, bevel, seg)


# ---- The cabinet (back against the wall at z = 0, front at z = 0.78) ----
W, D = 0.9, 0.78
hw = W / 2
box("Cabinet lower", (-hw, 0.05, 0.02), (hw, 0.95, D), body, 0.012)
box("Plinth", (-hw + 0.02, 0.0, 0.05), (hw - 0.02, 0.05, D - 0.05), grill, 0.004)
# Side art panels (both sides) and chrome T-molding along the edges.
for side in (-1, 1):
    x = side * (hw + 0.001)
    common.panel("Side art", (x, 1.0, D / 2), (0, 0, side * 1.0), (0, 1, 0), D - 0.08, 1.8, art)
for side in (-1, 1):
    for (lo, hi) in (((side * hw - 0.012, 0.05, D - 0.012), (side * hw + 0.012, 0.95, D + 0.004)),):
        box("T molding", lo, hi, trim, 0.004)
# Control deck: slanted, with the four big keys and two round buttons.
deck = Matrix.Translation((0, 0.95, D - 0.02)) @ Matrix.Rotation(0.18, 4, "X")
d = box("Deck", (-hw - 0.03, 0.0, -0.42), (hw + 0.03, 0.06, 0.06), body, 0.015, 3)
d.data.transform(deck)
dp = box("Deck plate", (-hw + 0.02, 0.06, -0.38), (hw - 0.02, 0.062, 0.02), trim, 0.002)
dp.data.transform(deck)
for k in range(4):
    x = -0.255 + k * 0.17
    kb = box("Key", (x - 0.068, 0.062, -0.24), (x + 0.068, 0.095, -0.08), key_mats[k], 0.01, 3)
    kb.data.transform(deck)
    ring = box("Key ring", (x - 0.078, 0.06, -0.25), (x + 0.078, 0.07, -0.07), trim, 0.004)
    ring.data.transform(deck)
for k, x in enumerate((-0.2, 0.2)):
    b = common.cylinder("Start button", Vector((x, 0.07, -0.33)), 0.025, 0.02, "y", led_cyan if k else led_pink,
                        segments=14)
    b.data.transform(deck)
cr = box("Card reader", (0.25, 0.062, -0.36), (0.38, 0.08, -0.28), grill, 0.006)
cr.data.transform(deck)
hj = common.cylinder("Headphone jack", Vector((-0.34, 0.07, -0.32)), 0.008, 0.02, "y", trim, segments=10)
hj.data.transform(deck)
# Coin door with two slots, a lock and a lit sign.
box("Coin door", (-0.18, 0.3, D), (0.18, 0.72, D + 0.012), trim, 0.006)
for x in (-0.07, 0.07):
    box("Coin slot", (x - 0.025, 0.58, D + 0.012), (x + 0.025, 0.66, D + 0.02), grill, 0.004)
    box("Coin return", (x - 0.03, 0.4, D + 0.012), (x + 0.03, 0.46, D + 0.022), grill, 0.004)
common.cylinder("Lock", Vector((0.0, 0.52, D + 0.015)), 0.012, 0.012, "z", P.mat("brass"), segments=10)
common.panel("Insert coin", (0.0, 0.69, D + 0.0125), (1, 0, 0), (0, 1, 0), 0.2, 0.05, coin_mat)
# Subwoofer grill in the lower front.
common.cylinder("Subwoofer", Vector((0.0, 0.17, D + 0.002)), 0.11, 0.012, "z", cone_m, segments=24)
common.torus("Sub ring", Vector((0.0, 0.17, D + 0.008)), 0.11, 0.01, trim, axis="z", segments=24, sides=6)
# Upper housing: tilted back screen with speakers either side.
up = Matrix.Translation((0, 1.02, 0.12)) @ Matrix.Rotation(-0.12, 4, "X")
u = box("Upper housing", (-hw, 0.0, 0.0), (hw, 1.2, 0.42), body, 0.015)
u.data.transform(up)
SW, SH = 0.56, 0.98
sc = common.quad("Screen", [(-SW / 2, 0.12, 0.421), (SW / 2, 0.12, 0.421), (SW / 2, 0.12 + SH, 0.421),
                            (-SW / 2, 0.12 + SH, 0.421)], screen)
sc.data.transform(up)
bz = box("Screen bezel", (-SW / 2 - 0.02, 0.1, 0.415), (SW / 2 + 0.02, 0.12 + SH + 0.02, 0.42), grill, 0.004)
bz.data.transform(up)
for side in (-1, 1):
    x = side * (SW / 2 + 0.085)
    for y in (0.3, 0.8):
        s1 = common.cylinder("Speaker", Vector((x, y, 0.425)), 0.055, 0.01, "z", cone_m, segments=18)
        s1.data.transform(up)
        s2 = common.torus("Speaker ring", Vector((x, y, 0.43)), 0.058, 0.007, trim, axis="z", segments=18, sides=5)
        s2.data.transform(up)
        s3 = common.sphere("Speaker cap", Vector((x, y, 0.43)), 0.018, grill, 10, 6)
        s3.data.transform(up)
    # LED strips down the front edges.
    led = box("LED edge", (side * hw - 0.012, 0.0, 0.415), (side * hw + 0.012, 1.2, 0.43), led_cyan if side < 0 else
              led_pink, 0.004)
    led.data.transform(up)
# Marquee on top, glowing.
mq = Matrix.Translation((0, 2.2, 0.1)) @ Matrix.Rotation(-0.12, 4, "X")
mb = box("Marquee box", (-hw - 0.02, 0.0, 0.0), (hw + 0.02, 0.26, 0.5), body, 0.012)
mb.data.transform(mq)
mm = common.quad("Marquee", [(-hw + 0.02, 0.03, 0.501), (hw - 0.02, 0.03, 0.501), (hw - 0.02, 0.23, 0.501),
                             (-hw + 0.02, 0.23, 0.501)], marquee)
mm.data.transform(mq)
for y in (0.0, 0.26):
    t = box("Marquee trim", (-hw - 0.025, y - 0.006, 0.49), (hw + 0.025, y + 0.006, 0.51), trim, 0.003)
    t.data.transform(mq)
# Headphone hook with headphones on the side.
common.tube("Hook", [(hw, 1.1, 0.4), (hw + 0.06, 1.1, 0.4), (hw + 0.07, 1.15, 0.4)], 0.006, trim, sides=6)
common.torus("Headphones", Vector((hw + 0.065, 1.02, 0.4)), 0.085, 0.012, P.mat("black"), axis="z", segments=20, sides=6)
for side in (-1, 1):
    common.cylinder("Ear cup", Vector((hw + 0.065 + side * 0.085, 0.95, 0.4)), 0.04, 0.035, "x", P.mat("black"),
                    segments=14)

# ---- The corner around it ----
# Neon "RHYTHM" sign on the wall (tubes following pixel-font letters).
neon_y, neon_s = 3.55, 0.05
glyphs = common._glyphs()
word = "RHYTHM"
x_start = -len(word) * 6 * neon_s / 2
for i, ch in enumerate(word):
    rows = glyphs[ch]
    for r, row in enumerate(rows):
        for c, dot in enumerate(row):
            if dot != "#":
                continue
            x = x_start + (i * 6 + c) * neon_s
            y = neon_y - r * neon_s
            common.box("Neon", (x - neon_s * 0.32, y - neon_s * 0.32, 0.05), (x + neon_s * 0.32, y + neon_s * 0.32, 0.07),
                       led_pink if i % 2 else led_cyan, 0.008, 2)
common.box("Neon backer", (x_start - 0.08, neon_y - 7 * neon_s - 0.05, 0.0),
           (-x_start + 0.05, neon_y + 0.08, 0.04), common.glass_material("Acrylic", color=(0.6, 0.6, 0.7), alpha=0.2),
           0.01)
# Speaker stacks either side.
for side in (-1, 1):
    x = side * 1.45
    for k, (w, h) in enumerate(((0.55, 0.6), (0.5, 0.55))):
        y0 = 0.0 if k == 0 else 0.6
        box("Speaker cab", (x - w / 2, y0, 0.05), (x + w / 2, y0 + h, 0.5), body, 0.012)
        common.cylinder("Woofer", Vector((x, y0 + h * 0.45, 0.502)), 0.17 if k == 0 else 0.14, 0.012, "z", cone_m,
                        segments=24)
        common.torus("Woofer ring", Vector((x, y0 + h * 0.45, 0.508)), 0.17 if k == 0 else 0.14, 0.012, trim,
                     axis="z", segments=24, sides=6)
        common.cylinder("Tweeter", Vector((x, y0 + h * 0.85, 0.502)), 0.04, 0.012, "z", cone_m, segments=14)
    box("LED strip", (x - 0.28, 1.15, 0.48), (x + 0.28, 1.17, 0.5), led_cyan if side < 0 else led_pink, 0.0)
# A beanbag and a small high table with a drink.
bean = common.sphere("Beanbag", Vector((1.15, 0.28, 1.35)), 0.45, common.pbr_material(
    "Beanbag fabric", tex_dir, "fabric_pattern_07", tint=(0.35, 0.1, 0.4), size=256), 20, 12)
bean.data.transform(Matrix.Translation((1.15, 0.0, 1.35)) @ Matrix.Diagonal((1.0, 0.62, 1.0, 1.0)) @
                    Matrix.Translation((-1.15, 0.0, -1.35)))
common.cylinder("Table top", Vector((-1.2, 1.05, 1.1)), 0.25, 0.03, "y", P.mat("dark_wood"), segments=24)
common.cylinder("Table post", Vector((-1.2, 0.52, 1.1)), 0.03, 1.04, "y", trim, segments=10)
common.cylinder("Table foot", Vector((-1.2, 0.01, 1.1)), 0.22, 0.02, "y", trim, segments=20)
P.glass(-1.12, 1.065, 1.05, "highball", "blue", ice=True)
P.glass(-1.28, 1.065, 1.18, "pint", "beer")

common.finish_and_export(out_path, uv_scale={"Beanbag fabric": 3.0})
