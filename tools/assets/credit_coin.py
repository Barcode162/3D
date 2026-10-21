# The credit coin (the world's money, "credits" for now): a thick gold
# token with a raised rim, a ridged edge, a glowing pink enamel ring and the
# Looped loop (an infinity sign) raised on both faces. It pops out and spins
# when you earn credits (bar/coin_pop.cpp).
#
#   blender -b --factory-startup --python tools/assets/credit_coin.py -- <out.glb>
#
# Centered on its origin, faces toward +z and -z, 12 cm across (it's shown
# big on purpose, like a game reward).
import math
import os
import sys

from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402

out = sys.argv[sys.argv.index("--") + 1:][0]

R = 0.06        # radius
T = 0.014       # thickness
FACE = 0.0045   # how deep the face sits inside the rim

common.reset_scene()
gold = common.plain_material("Gold", (1.0, 0.74, 0.30), 1.0, 0.22)
brushed = common.plain_material("Gold face", (0.93, 0.64, 0.25), 1.0, 0.42)
enamel = common.plain_material("Pink enamel", (0.9, 0.12, 0.5), 0.0, 0.25, emission=(1.0, 0.22, 0.62), strength=3.0)
polished = common.plain_material("Gold emblem", (1.0, 0.82, 0.42), 1.0, 0.12)

flip = Matrix.Rotation(math.pi / 2, 4, "X")  # lathe spins around y; the coin faces z

# The body: rim rounded on both sides, the faces sunk inside it.
h = T / 2
profile = [(0.0, -h + FACE), (R - 0.012, -h + FACE), (R - 0.0105, -h), (R - 0.003, -h), (R, -h + 0.003),
           (R, h - 0.003), (R - 0.003, h), (R - 0.0105, h), (R - 0.012, h - FACE), (0.0, h - FACE)]
body = common.lathe("Body", [(r, y) for (r, y) in profile], gold, segments=72)
common.transform(body, flip)

# The faces' brushed metal: thin discs on the sunk faces.
for side in (-1, 1):
    disc = common.cylinder("Face", Vector((0, 0, side * (h - FACE + 0.0004))), R - 0.0122, 0.0008, "z", brushed,
                           segments=64, bevel=0.0003)

# Ridged edge: little bars all the way round.
ridges = 84
for k in range(ridges):
    a = 2 * math.pi * k / ridges
    c = Vector((math.cos(a) * (R + 0.0004), math.sin(a) * (R + 0.0004), 0))
    bar = common.box("Ridge", Vector((-0.0007, -0.0008, -(h - 0.0035))), Vector((0.0007, 0.0008, h - 0.0035)), gold, bevel=0.0)
    common.transform(bar, Matrix.Translation(c) @ Matrix.Rotation(a, 4, "Z"))

# The glowing enamel ring and the loop emblem, on both faces.
for side in (-1, 1):
    z = side * (h - FACE + 0.0012)
    ring = common.torus("Enamel ring", Vector((0, 0, 0)), 0.041, 0.0021, enamel, axis="z", segments=64, sides=10)
    common.transform(ring, Matrix.Translation(Vector((0, 0, z))))
    # The loop: a lemniscate (infinity sign), 5.6 cm wide, raised.
    pts = []
    steps = 96
    for i in range(steps):
        t = 2 * math.pi * i / steps
        d = 1 + math.sin(t) ** 2
        pts.append(Vector((0.028 * math.cos(t) / d, 0.028 * math.sin(t) * math.cos(t) / d, z + side * 0.0012)))
    common.tube("Loop", pts, 0.0032, polished, sides=12, closed=True)
    # Four small studs between the ring and the edge.
    for k in range(4):
        a = math.pi / 4 + k * math.pi / 2
        common.sphere("Stud", Vector((math.cos(a) * 0.0335, math.sin(a) * 0.0335, z)), 0.0022, polished, 12, 6)

common.finish_and_export(out, {})
