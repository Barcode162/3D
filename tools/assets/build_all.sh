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
B="/opt/blender/blender -b --factory-startup --python"
out=assets/models
run() { $B "tools/assets/$1" -- "${@:2}" 2>&1 | grep -E "EXPORTED|Error|Traceback" || true; }
# The big bar upgrade (docs/BAR_REDESIGN.md): one model per zone, built in
# world space (except the piano, sim rig and rhythm corner, placed by the app).
run hall.py "$tex" $out/hall.glb
run bar_counter_l.py "$tex" $out/bar_counter_l.glb
run back_bar_tall.py "$tex" $out/back_bar_tall.glb
run piano.py "$tex" $out/piano.glb
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
run furniture.py "$tex" $out
run lighting.py "$tex" $out
run decor.py "$tex" $out
run glasses.py $out
(cd $out && rm -f booth.glb booth_table.glb poster_jazz.glb poster_cocktails.glb mirror_frame_brass.glb \
  mirror_frame_black.glb neon_sign.glb beer_taps.glb coaster.glb napkin_holder.glb menu_stand.glb)
$B tools/assets/preview.py -- "$scratch/previews" $out/*.glb 2>&1 | grep -cE "PREVIEW" | sed 's/$/ previews rendered/'
du -sh $out
