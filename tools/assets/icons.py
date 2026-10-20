# The menu's icons, modelled in 3D and rendered to pictures (instead of
# emojis): the main menu (emotes, avatar, profile, voice, settings, piano,
# bartender, help, stand up) and one smiley for each emote. Same toy look as
# the badges (toy.py). One PNG each into bar/web/icons/.
#
#   blender -b --factory-startup --python tools/assets/icons.py -- <out dir> [size] [names...]
import math
import os
import sys

import bpy  # noqa: F401
from mathutils import Matrix  # noqa: F401

sys.path.insert(0, os.path.dirname(__file__))
from toy import *  # noqa: E402,F401,F403

args = sys.argv[sys.argv.index("--") + 1:]
out_dir = args[0]
SIZE = int(args[1]) if len(args) > 1 else 192
os.makedirs(out_dir, exist_ok=True)


def yellow():
    return mat("Smiley", (1.0, 0.78, 0.12), 0.0, 0.3, coat=1.0)


def ink():
    return mat("Ink", (0.08, 0.04, 0.06), 0.0, 0.2, coat=1.0)


def ellipsoid(name, c, r, scale, m):
    o = sphere(name, (0, 0, 0), r, m)
    o.scale = scale
    o.location = c
    return o


# ---- A smiley face and its parts ----
def face(eyes="dots", mouth="smile", tilt=0.0):
    head = sphere("Head", (0, 0, 0), 0.7, yellow())
    head.rotation_euler = (tilt, 0, 0)
    k = ink()
    for s in (-1, 1):
        x = s * 0.24
        if eyes == "dots":
            ellipsoid("Eye", (x, 0.16, 0.6), 0.1, (0.8, 1.2, 0.5), k)
        elif eyes == "happy":  # closed, curved up: ^ ^
            tube("Eye", arc_pts(x, 0.1, 0.11, math.radians(20), math.radians(160), 10, 0.66), 0.035, k)
        elif eyes == "closed":
            tube("Eye", arc_pts(x, 0.22, 0.11, math.radians(200), math.radians(340), 10, 0.64), 0.035, k)
    if mouth == "smile":
        tube("Mouth", arc_pts(0, -0.05, 0.3, math.radians(205), math.radians(335), 16, 0.63), 0.045, k)
    elif mouth == "open":
        ellipsoid("Mouth", (0, -0.24, 0.58), 0.18, (1.2, 0.9, 0.4), mat("Mouth", (0.45, 0.05, 0.1), 0, 0.3, coat=1))
    elif mouth == "flat":
        tube("Mouth", [(-0.18, -0.25, 0.64), (0.18, -0.25, 0.64)], 0.04, k)
    elif mouth == "frown":
        tube("Mouth", arc_pts(0, -0.45, 0.28, math.radians(35), math.radians(145), 16, 0.6), 0.045, k)
    elif mouth == "grin":
        ellipsoid("Mouth", (0, -0.2, 0.56), 0.24, (1.25, 0.75, 0.4), mat("Mouth", (0.45, 0.05, 0.1), 0, 0.3, coat=1))
        ellipsoid("Teeth", (0, -0.08, 0.66), 0.17, (1.3, 0.35, 0.3), mat("Teeth", (1, 1, 1), 0, 0.2, coat=1))
    return head


def mitten(c, rot_z=0.0, thumb=True, size=1.0):
    y = mat("Hand", (1.0, 0.7, 0.1), 0.0, 0.3, coat=1.0)
    hand = ellipsoid("Hand", c, 0.2 * size, (1.0, 1.25, 0.8), y)
    hand.rotation_euler = (0, 0, rot_z)
    if thumb:
        ox, oy = math.cos(rot_z + math.pi * 0.75) * 0.2 * size, math.sin(rot_z + math.pi * 0.75) * 0.2 * size
        ellipsoid("Thumb", (c[0] + ox, c[1] + oy, c[2] + 0.02), 0.08 * size, (1, 1.5, 0.9), y).rotation_euler = (0, 0, rot_z + 0.6)
    return hand


def lines(c, n=3, r=0.3, a0=20, a1=70):
    w = mat("Motion", (1, 1, 1), 0, 0.2, emit=(1, 1, 1), strength=0.6)
    for i in range(n):
        tube("Line", arc_pts(c[0], c[1], r + i * 0.1, math.radians(a0), math.radians(a1), 8, c[2]), 0.022, w)


def music_note(c, s=1.0, m=None):
    m = m or mat("Note pink", (1, 0.3, 0.7), 0, 0.2, coat=1)
    ellipsoid("Note head", c, 0.1 * s, (1.3, 1, 0.8), m)
    tube("Note stem", [(c[0] + 0.12 * s, c[1], c[2]), (c[0] + 0.12 * s, c[1] + 0.4 * s, c[2])], 0.025 * s, m)
    tube("Note flag", [(c[0] + 0.12 * s, c[1] + 0.4 * s, c[2]), (c[0] + 0.28 * s, c[1] + 0.25 * s, c[2])], 0.025 * s, m)


def sparkle(c, s=0.12):
    extrude("Sparkle", star_outline(s * 0.35, s, 4), c[2], c[2] + 0.04,
            mat("Sparkle", (1, 0.95, 0.6), 0, 0.1, emit=(1, 0.9, 0.5), strength=2), 0.01)
    bpy.context.collection.objects[-1].location = (c[0], c[1], 0)


EMOTES = {
    "wave": lambda: (face(), mitten((0.78, 0.35, 0.3), -0.4), lines((0.62, 0.3, 0.3))),
    "dance": lambda: (face("happy", "smile"), music_note((0.55, 0.45, 0.5)), music_note((-0.8, 0.25, 0.4), 0.7)),
    "clap": lambda: (face("happy", "open"), mitten((-0.16, -0.62, 0.62), 0.5, False), mitten((0.16, -0.62, 0.66), -0.5, False),
                     sparkle((0.0, -0.25, 0.85))),
    "cheer": lambda: (face("happy", "open"), mitten((-0.75, 0.55, 0.2), 0.5), mitten((0.75, 0.55, 0.2), -0.5)),
    "bow": lambda: face("closed", "smile", tilt=0.45),
    "point": lambda: (face(), mitten((0.62, -0.35, 0.55), -1.57, False),
                      tube("Finger", [(0.75, -0.35, 0.6), (1.05, -0.35, 0.6)], 0.07, mat("Hand", (1, 0.7, 0.1), 0, 0.3, coat=1))),
    "shrug": lambda: (face("dots", "flat"), mitten((-0.85, -0.2, 0.2), 1.9), mitten((0.85, -0.2, 0.2), -1.9),
                      tube("Q", arc_pts(0.0, 1.0, 0.13, math.radians(180), math.radians(-60), 12, 0.2), 0.04, ink())),
    "laugh": lambda: (face("happy", "grin"),
                      [ellipsoid("Tear", (s * 0.52, 0.0, 0.45), 0.08, (0.8, 1.3, 0.6), mat("Tear", (0.3, 0.7, 1), 0, 0.05, coat=1))
                       for s in (-1, 1)]),
    "salute": lambda: (face("dots", "smile"), mitten((0.5, 0.48, 0.5), -0.9, False)),
    "thumbsup": lambda: (face("happy", "smile"), mitten((0.72, -0.25, 0.45), 0.0, False),
                         ellipsoid("Thumb", (0.72, 0.02, 0.5), 0.09, (1, 1.8, 0.9), mat("Hand", (1, 0.7, 0.1), 0, 0.3, coat=1))),
    "facepalm": lambda: (face("dots", "frown"), mitten((0.24, 0.2, 0.7), 0.3, False)),
}


# ---- Menu icons ----
def figure():
    skin = mat("Skin", (0.75, 0.48, 0.32), 0, 0.4, coat=0.3)
    shirt = mat("Shirt", (1.0, 0.3, 0.6), 0, 0.35, coat=1)
    sphere("Head", (0, 0.45, 0), 0.3, skin)
    ellipsoid("Hair", (0, 0.6, -0.04), 0.3, (1.05, 0.65, 1.0), mat("Hair", (0.1, 0.06, 0.04), 0, 0.5, coat=0.5))
    lathe("Body", [(0.0, -0.75), (0.45, -0.75), (0.5, -0.5), (0.42, 0.0), (0.18, 0.12), (0.0, 0.14)], shirt)


def id_card():
    extrude("Card", [(-0.75, -0.48), (0.75, -0.48), (0.75, 0.48), (-0.75, 0.48)], 0, 0.1, enamel("Card", (0.2, 0.5, 1.0)), 0.08, 5)
    cylinder_z("Photo", (-0.38, 0.0, 0.12), 0.24, 0.06, mat("Photo", (1, 0.85, 0.6), 0, 0.3, coat=1))
    for i, w in enumerate((0.5, 0.42, 0.3)):
        tube("Line", [(0.0, 0.2 - i * 0.2, 0.14), (w, 0.2 - i * 0.2, 0.14)], 0.04, gold())


def microphone():
    s = silver()
    sphere("Grille", (0, 0.35, 0), 0.34, mat("Grille", (0.55, 0.58, 0.62), 1, 0.35))
    cylinder_z("Band", (0, 0.1, 0), 0.0, 0.0, s) if False else None
    tube("Handle", [(0, 0.08, 0), (0, -0.7, 0)], 0.15, ink())
    tube("Ring", arc_pts(0, 0.05, 0.17, 0, math.tau, 24, 0.0), 0.03, gold(), closed=True)
    sphere("Light", (0, -0.25, 0.14), 0.05, mat("Rec", (1, 0.1, 0.1), 0, 0.2, emit=(1, 0.1, 0.1), strength=3))


def gear():
    pts = []
    teeth = 10
    for i in range(teeth * 4):
        a = i * math.tau / (teeth * 4)
        r = 0.72 if (i % 4) in (1, 2) else 0.56
        pts.append((r * math.cos(a), r * math.sin(a)))
    extrude("Gear", pts, -0.1, 0.12, silver(), 0.04)
    cylinder_z("Hub", (0, 0, 0.12), 0.24, 0.1, ink())


def keys():
    extrude("Case", [(-0.85, -0.45), (0.85, -0.45), (0.85, 0.4), (-0.85, 0.4)], -0.1, 0.08, ink(), 0.06)
    white = mat("Key white", (0.97, 0.96, 0.92), 0, 0.2, coat=1)
    for i in range(7):
        x = -0.72 + i * 0.24
        extrude("Key", [(x - 0.105, -0.38), (x + 0.105, -0.38), (x + 0.105, 0.3), (x - 0.105, 0.3)], 0.08, 0.16, white, 0.03)
    for i in (0, 1, 3, 4, 5):
        x = -0.6 + i * 0.24
        extrude("Black key", [(x - 0.07, -0.02), (x + 0.07, -0.02), (x + 0.07, 0.3), (x - 0.07, 0.3)], 0.16, 0.26, ink(), 0.025)


def cocktail_icon():
    glass = mat("Glass", (0.9, 0.95, 1.0), 0, 0.05, coat=1, transmission=0.7)
    drink = mat("Drink", (1.0, 0.25, 0.55), 0, 0.15, coat=1, emit=(1.0, 0.2, 0.5), strength=0.6)
    lathe("Martini", [(0.0, -0.05), (0.05, -0.02), (0.6, 0.55), (0.64, 0.58), (0.0, 0.58)], glass, (0, 0.05, 0))
    lathe("Drink", [(0.0, 0.02), (0.5, 0.46), (0.0, 0.46)], drink, (0, 0.05, 0))
    tube("Stem", [(0, 0.0, 0), (0, -0.55, 0)], 0.045, silver())
    lathe("Foot", [(0.0, -0.62), (0.3, -0.62), (0.3, -0.56), (0.05, -0.52), (0.0, -0.52)], silver())
    sphere("Olive", (0.2, 0.48, 0.2), 0.1, mat("Olive", (0.35, 0.6, 0.1), 0, 0.25, coat=1))


def question():
    b = enamel("Question", (0.2, 0.55, 1.0))
    tube("Hook", arc_pts(0, 0.32, 0.3, math.radians(170), math.radians(-60), 24, 0) + [(0.0, -0.18, 0)], 0.12, b)
    sphere("Dot", (0, -0.55, 0), 0.14, b)


def chair():
    wood = mat("Chair wood", (0.6, 0.32, 0.12), 0, 0.35, coat=1)
    red = enamel("Cushion", (0.9, 0.15, 0.2))
    extrude("Seat", [(-0.5, -0.1), (0.5, -0.1), (0.5, 0.05), (-0.5, 0.05)], -0.4, 0.4, red, 0.06)
    extrude("Back", [(-0.5, 0.05), (0.5, 0.05), (0.45, 0.75), (-0.45, 0.75)], -0.4, -0.22, wood, 0.06)
    for x in (-0.42, 0.42):
        for z in (-0.32, 0.32):
            tube("Leg", [(x, -0.1, z), (x * 1.08, -0.75, z * 1.1)], 0.05, wood)


ICONS = {
    "menu_emotes": EMOTES["wave"], "menu_avatar": figure, "menu_profile": id_card, "menu_voice": microphone,
    "menu_settings": gear, "menu_piano": keys, "menu_bartender": cocktail_icon, "menu_help": question,
    "menu_standup": chair,
}
for k, f in EMOTES.items():
    ICONS["emote_" + k] = f


def build(name):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ICONS[name]()
    turn_all(-12, 18)
    render(os.path.join(out_dir, name + ".png"), SIZE, ortho=2.5, cam_y=0.0)
    print("ICON", name)


for n in (args[2:] if len(args) > 2 else ICONS):
    build(n)
