#!/usr/bin/env bash
# Runs the native tests twice: built with g++ (fast) and with em++ run in
# node (the same compiler as the game, so wasm math is checked too).
# Usage: bash tools/native/run.sh   (needs emsdk_env.sh sourced for em++)
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p build/native
RACE="app/race/world.cpp app/race/track.cpp app/race/mesh.cpp engine/vehicle.cpp"
declare -A TESTS=(
  [vehicle_test]="tools/native/vehicle_test.cpp engine/vehicle.cpp"
  [race_test]="tools/native/race_test.cpp $RACE"
  [json_test]="tools/native/json_test.cpp engine/json.cpp"
  [math_test]="tools/native/math_test.cpp"
  [proto_test]="tools/native/proto_test.cpp engine/netproto.cpp engine/json.cpp"
  [walk_test]="tools/native/walk_test.cpp app/bar_scene.cpp app/materials.cpp engine/procedural.cpp engine/mesh_data.cpp"
  [physics_test]="tools/native/physics_test.cpp engine/physics.cpp engine/mesh_data.cpp app/props.cpp app/bar_scene.cpp app/materials.cpp engine/procedural.cpp"
)
for t in "${!TESTS[@]}"; do
  echo "== $t (g++)"
  g++ -std=c++20 -O2 -I. ${TESTS[$t]} -o build/native/$t
  build/native/$t
  if command -v em++ >/dev/null; then
    echo "== $t (em++ + node)"
    em++ -std=c++20 -O2 -I. ${TESTS[$t]} -sENVIRONMENT=node -sNODERAWFS=1 -sALLOW_MEMORY_GROWTH=1 -o build/native/$t.js
    node build/native/$t.js
  else
    echo "(em++ not found: skipped the wasm run)"
  fi
done
echo "== synth_bench (g++, limits) and (em++ + node, print only)"
g++ -std=c++20 -O2 -I. tools/native/synth_bench.cpp engine/synth.cpp -o build/native/synth_bench
build/native/synth_bench
if command -v em++ >/dev/null; then
  em++ -std=c++20 -O2 -I. tools/native/synth_bench.cpp engine/synth.cpp -sENVIRONMENT=node -o build/native/synth_bench.js
  node build/native/synth_bench.js --print
fi
echo "NATIVE TESTS PASSED"
