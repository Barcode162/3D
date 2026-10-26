# The glasses on the tables as things (the world remake J72.2): one model per
# drink recipe (the glass, its drink, ice), standing on its own base, and a
# thing for each glass the lounge, the bay table and the bar top wrote down
# ($LOOPED_DRINKS, props_lib.save_records), where it stood. Bottles written
# down there use their bottle recipe (bottles.py).
#
#   LOOPED_DRINKS=<drinks.jsonl> blender ... (lounge.py, decor_walls.py, bar_counter_l.py)
#   blender -b --factory-startup --python tools/assets/drinks.py -- <texture dir> <out dir> <drinks.jsonl> <things.json>
import json
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402
import props_lib  # noqa: E402

tex_dir, out_dir, rows_path, things_path = sys.argv[sys.argv.index("--") + 1:][:4]
rows = [json.loads(line) for line in open(rows_path) if line.strip()]


def model_of(r):
    return "drink_" + r["kind"] + ("_" + r["drink"] if r["drink"] else "") + ("_ice" if r["ice"] else "")


made = set()
things = []
for n, r in enumerate(rows):
    if r["type"] == "bottle":
        things.append({"id": f"table_bottle-{n + 1}", "kind": "bottle", "at": [round(v, 4) for v in r["at"]],
                       "turn": round(r["turn"][1], 2),
                       "parts": {"model": {"file": f"bottle_{r['kind']}_1.glb", "material": "bottle-glass"}}})
        continue
    name = model_of(r)
    if name not in made:
        made.add(name)
        common.reset_scene()
        P = props_lib.Props(tex_dir, seed=72)
        P.glass(0, 0, 0, r["kind"], r["drink"] or None, ice=r["ice"])
        common.finish_and_export(os.path.join(out_dir, name + ".glb"), uv_scale={})
    things.append({"id": f"drink-{n + 1}", "kind": "glass", "at": [round(v, 4) for v in r["at"]],
                   "turn": round(r["turn"], 2), "parts": {"model": {"file": name + ".glb", "material": "bottle-glass"}}})
with open(things_path, "w") as f:
    json.dump(things, f)
print("DRINK THINGS", len(things), "from", len(made), "drink models")
