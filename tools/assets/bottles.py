# The bottles as things (the world remake J72): a set of bottle recipes
# (shape, glass colour, label, drink), each a model standing on its own base
# with its label facing +z (props_lib.Props.bottle), and the back bar's
# bottles (where back_bar_tall.py wrote them down) as things: each a copy of
# the recipe of its shape, a little different from the next (its turn, its
# size). Copies of one model are drawn together, so 200 bottles cost about as
# much as 24.
#
#   blender -b --factory-startup --python tools/assets/bottles.py -- <texture dir> <out dir> <bottles.json> <things.json>
import json
import math

from mathutils import Euler
import os
import random
import sys

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402
import props_lib  # noqa: E402

tex_dir, out_dir, bottles_path, things_path = sys.argv[sys.argv.index("--") + 1:][:4]

# Recipes per shape: (glass colour, label, drink; "" = a clear spirit).
RECIPES = {
    "wine": [(3, 0, "red_wine"), (1, 9, "red_wine"), (8, 12, "white_wine")],
    "whiskey": [(0, 1, "whiskey"), (4, 13, "whiskey"), (9, 15, "whiskey")],
    "slim": [(2, 2, ""), (8, 3, ""), (11, 10, "green")],
    "liqueur": [(10, 8, "red"), (12, 11, "orange"), (6, 5, "coffee")],
    "square": [(8, 6, "whiskey"), (2, 7, "whiskey")],
    "stubby": [(5, 4, "whiskey"), (14, 14, "cola")],
    "cognac": [(4, 6, "whiskey"), (9, 1, "whiskey")],
    "champagne": [(1, 3, "white_wine"), (6, 12, "white_wine")],
    "tequila": [(8, 4, "white_wine"), (2, 15, "")],
    "magnum": [(1, 0, "red_wine")],
}


def model_name(kind, k):
    return f"bottle_{kind}_{k + 1}"


def own_label(P, label):
    """The bottle's label alone (its cell of the 4 x 4 label picture), so each
    model carries one label, not all sixteen."""
    import bpy
    import numpy as np
    r, c = divmod(label % 16, 4)
    mat = P.mat("labels")
    node = next(n for n in mat.node_tree.nodes if n.type == "TEX_IMAGE")
    big = node.image
    w, h = big.size
    px = np.empty(w * h * 4, dtype=np.float32)
    big.pixels.foreach_get(px)
    px = px.reshape(h, w, 4)
    cw, ch = w // 4, h // 4
    cell = px[r * ch:(r + 1) * ch, c * cw:(c + 1) * cw].copy()
    img = bpy.data.images.new("Bottle label", cw, ch)
    img.pixels.foreach_set(cell.ravel())
    node.image = img
    for o in bpy.context.scene.objects:
        if o.type != "MESH" or o.data.materials[0] != mat:
            continue
        for loop in o.data.uv_layers.active.data:
            u, v = loop.uv
            loop.uv = ((u - c / 4) * 4, (v - r / 4) * 4)


for kind, recipes in RECIPES.items():
    for k, (color, label, drink) in enumerate(recipes):
        common.reset_scene()
        P = props_lib.Props(tex_dir, seed=40 + k)
        P.bottle(0, 0, 0, kind, color=color, label=label, yaw=0.0, segments=12, fill=drink)
        own_label(P, label)
        common.finish_and_export(os.path.join(out_dir, model_name(kind, k) + ".glb"), uv_scale={})

rng = random.Random(72)
things = []
tilted = {}  # a recipe lying or leaning (the wine rack, the ice bucket): its own model, turned already
for n, b in enumerate(json.load(open(bottles_path))):
    recipes = RECIPES[b["kind"]]
    k = (b["color"] + b["label"]) % len(recipes)
    s = round(rng.uniform(0.97, 1.03), 3)
    tx, ty, tz = b["turn"]
    model = model_name(b["kind"], k)
    if round(tx) or round(tz):
        # A thing turns only about the up axis (the place file's "turn"), so
        # the tilt is built into a model of its own.
        model = f"{model}_tilt{round(tx)}_{round(tz)}"
        tilted[model] = (b["kind"], k, tx, tz)
    t = {"id": f"bottle-{n + 1}", "kind": "bottle", "at": [round(v, 4) for v in b["at"]],
         "turn": round(ty + rng.uniform(-12, 12), 2) if not (round(tx) or round(tz)) else round(ty, 2),
         "scale": s, "parts": {"model": {"file": model + ".glb", "material": "bottle-glass"}}}
    things.append(t)
for model, (kind, k, tx, tz) in tilted.items():
    color, label, drink = RECIPES[kind][k]
    common.reset_scene()
    P = props_lib.Props(tex_dir, seed=40 + k)
    P.bottle(0, 0, 0, kind, color=color, label=label, yaw=0.0, segments=12, fill=drink,
             tilt=Euler((math.radians(tx), 0, math.radians(tz)), "XYZ").to_matrix().to_4x4())
    own_label(P, label)
    common.finish_and_export(os.path.join(out_dir, model + ".glb"), uv_scale={})
with open(things_path, "w") as f:
    json.dump(things, f)
print("BOTTLE THINGS", len(things), "from", sum(len(r) for r in RECIPES.values()), "recipes")
