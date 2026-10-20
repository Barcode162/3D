#!/usr/bin/env bash
# Runs the native tests twice: built with g++ (fast) and with em++ run in
# node (the same compiler as the game, so wasm math is checked too).
# Usage: bash tools/native/run.sh   (needs emsdk_env.sh sourced for em++)
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p build/native
RACE="bar/projects/racing/world.cpp bar/projects/racing/track.cpp bar/projects/racing/mesh.cpp bar/projects/racing/cars.cpp looped/vehicle.cpp looped/drive/car_spec.cpp looped/drive/tire.cpp looped/drive/vehicle_sim.cpp looped/json.cpp"
declare -A TESTS=(
  [vehicle_test]="tools/native/vehicle_test.cpp looped/vehicle.cpp"
  [drive_lab]="tools/native/drive_lab.cpp looped/drive/car_spec.cpp looped/drive/tire.cpp looped/drive/vehicle_sim.cpp looped/json.cpp"
  [race_test]="tools/native/race_test.cpp $RACE"
  [steering_test]="tools/native/steering_test.cpp looped/steering.cpp $RACE"
  [json_test]="tools/native/json_test.cpp looped/json.cpp"
  [math_test]="tools/native/math_test.cpp"
  [quality_test]="tools/native/quality_test.cpp"
  [entity_test]="tools/native/entity_test.cpp bar/entities.cpp bar/bar_scene.cpp bar/materials.cpp looped/procedural.cpp looped/mesh_data.cpp"
  [proto_test]="tools/native/proto_test.cpp looped/netproto.cpp looped/json.cpp"
  [chess_test]="tools/native/chess_test.cpp bar/projects/chess/chess_rules.cpp"
  [activity_test]="tools/native/activity_test.cpp bar/activity.cpp"
  [interaction_test]="tools/native/interaction_test.cpp bar/interaction.cpp"
  [walk_test]="tools/native/walk_test.cpp bar/bar_scene.cpp bar/materials.cpp looped/procedural.cpp looped/mesh_data.cpp"
  [physics_test]="tools/native/physics_test.cpp looped/physics.cpp looped/mesh_data.cpp bar/props.cpp bar/bar_scene.cpp bar/materials.cpp looped/procedural.cpp"
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
g++ -std=c++20 -O2 -I. tools/native/synth_bench.cpp looped/synth.cpp -o build/native/synth_bench
build/native/synth_bench
if command -v em++ >/dev/null; then
  em++ -std=c++20 -O2 -I. tools/native/synth_bench.cpp looped/synth.cpp -sENVIRONMENT=node -o build/native/synth_bench.js
  node build/native/synth_bench.js --print
fi
echo "NATIVE TESTS PASSED"
