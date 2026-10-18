#!/usr/bin/env bash
# Builds the room's physics (cloud/sim/room_sim.cpp + the bar's C++) into
# cloud/sim/room_sim.wasm for the Cloudflare Worker. Needs emsdk_env.sh sourced.
# Run by CI before the room tests and the deploy, and by tools/setup.sh.
set -euo pipefail
cd "$(dirname "$0")/../.."
em++ -std=c++20 -O2 -I. -fno-exceptions \
  cloud/sim/room_sim.cpp engine/physics.cpp engine/mesh_data.cpp app/props.cpp app/bar_scene.cpp \
  app/materials.cpp engine/procedural.cpp \
  --no-entry -sSTANDALONE_WASM=1 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=16MB -sSTACK_SIZE=1MB \
  -sEXPORTED_FUNCTIONS=_sim_buffer,_sim_init,_sim_step,_sim_get,_sim_hold,_sim_release,_sim_place,_sim_awake \
  -o cloud/sim/room_sim.wasm
ls -l cloud/sim/room_sim.wasm
