#!/usr/bin/env bash
# The bar's world check (rule "The user tests", request 2026-10-26 #9): builds
# and runs tools/native/bar_world_test.cpp with g++ (a few seconds): the place
# and kinds files read cleanly, ids are unique, every kind has a live loop.
# piece.mjs runs it whenever a piece changes assets/world/.
set -euo pipefail
cd "$(dirname "$0")/.."
SCENE="bar/bar_scene.cpp bar/scene_names.cpp bar/world_files.cpp bar/materials.cpp looped/procedural.cpp looped/mesh_data.cpp looped/world/place_layout.cpp looped/world/loop.cpp looped/world/kind.cpp looped/world/world.cpp looped/world/catalog.cpp looped/json.cpp"
mkdir -p build/native
g++ -std=c++20 -O1 -I. tools/native/bar_world_test.cpp $SCENE -o build/native/bar_world_test
./build/native/bar_world_test | grep -E "FAIL|^ +|PASSED|FAILED" || true
./build/native/bar_world_test >/dev/null
