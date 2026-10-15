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
run room.py "$tex" $out/room.glb
run bar_counter.py "$tex" $out/bar_counter.glb
run back_bar.py "$tex" $out/back_bar.glb
run jukebox.py "$tex" $out/jukebox.glb
run stools.py "$tex" $out
run furniture.py "$tex" $out
run lighting.py "$tex" $out
run props.py "$tex" $out
run decor.py "$tex" $out
run glasses.py $out
$B tools/assets/preview.py -- "$scratch/previews" $out/*.glb 2>&1 | grep -cE "PREVIEW" | sed 's/$/ previews rendered/'
du -sh $out
