#!/usr/bin/env bash
# Rebuilds every model in assets/models/ in one go: installs Blender if
# needed, downloads the CC0 textures, runs each script, then renders a
# preview picture of every model (Cycles) to <scratch dir>/previews/.
# Usage: bash tools/assets/build_all.sh <scratch dir>
set -euo pipefail
cd "$(dirname "$0")/../.."
scratch=${1:?usage: build_all.sh <scratch dir>}
tex="$scratch/textures"
bash tools/assets/get_blender.sh
bash tools/assets/fetch_textures.sh "$tex"
python3 tools/assets/labels.py "$tex"  # bottle labels (needs Pillow)
# Poly Haven models (CC0, the remake's kit, J35): fetched, slimmed, written.
ph() { bash tools/assets/polyhaven_fetch.sh "$1" "$scratch/polyhaven" 1k >/dev/null
       $B tools/assets/polyhaven.py -- "$scratch/polyhaven/$1" "$out/$2" "${@:3}" 2>&1 | grep -E "EXPORTED|Error|Traceback" || true; }
B="/opt/blender/blender -b --factory-startup --python"
out=assets/models
run() { $B "tools/assets/$1" -- "${@:2}" 2>&1 | grep -E "EXPORTED|Error|Traceback" || true; }
# The big bar upgrade (docs/archive/BAR_REDESIGN.md): one model per zone, built in
# world space (except the piano, sim rig and rhythm corner, placed by the app).
run hall.py "$tex" $out/hall.glb
run bar_counter_l.py "$tex" $out/bar_counter_l.glb
run back_bar_tall.py "$tex" $out/back_bar_tall.glb
run piano.py "$tex" $out/piano.glb
run stage.py "$tex" $out/stage.glb
run big_mirror.py "$tex" $out
run construction.py "$tex" $out/construction.glb
run sim_rig.py "$tex" $out/sim_rig.glb
run rhythm.py "$tex" $out/rhythm.glb
run darts_zone.py "$tex" $out/darts_zone.glb
run lounge.py "$tex" $out
run decor_walls.py "$tex" $out/decor_walls.glb
# Older scripts that still provide models in use (they also write a few
# unused ones; delete those again: booth, posters, mirror frames, ...).
run jukebox.py "$tex" $out/jukebox.glb
run stools.py "$tex" $out
# The bar stools (J35.2): seat at 0.76 m (Poly Haven's is a counter stool,
# 0.69 m), its back away from the counter (+z).
ph bar_chair_round_01 bar_stool.glb --tris 8000 --height 0.825 --turn 180
# The pendant lamps over the counter (J35.5): 80% size, hanging from their
# top (lighting.py's own pendant is no longer used).
ph hanging_industrial_lamp pendant_lamp.glb --tris 8000 --height 1.07 --hang
run furniture.py "$tex" $out
run lighting.py "$tex" $out
run decor.py "$tex" $out
run glasses.py $out
(cd $out && rm -f booth.glb booth_table.glb poster_jazz.glb poster_cocktails.glb mirror_frame_brass.glb \
  mirror_frame_black.glb neon_sign.glb beer_taps.glb coaster.glb napkin_holder.glb menu_stand.glb)
$B tools/assets/preview.py -- "$scratch/previews" $out/*.glb 2>&1 | grep -cE "PREVIEW" | sed 's/$/ previews rendered/'
du -sh $out
