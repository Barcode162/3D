#!/usr/bin/env bash
# Builds the room's physics (cloud/sim/room_sim.cpp + the bar's C++) into
# cloud/sim/room_sim.wasm for the Cloudflare Worker. Needs emsdk_env.sh sourced.
# Run by CI before the room tests and the deploy, and by tools/setup.sh.
set -euo pipefail
cd "$(dirname "$0")/../.."
em++ -std=c++20 -O2 -I. -fno-exceptions -DWORLD_FILES_GIVEN_ONLY \
  cloud/sim/room_sim.cpp looped/physics.cpp looped/mesh_data.cpp bar/props.cpp bar/bar_scene.cpp \
  bar/scene_names.cpp bar/world_files.cpp bar/materials.cpp looped/procedural.cpp looped/json.cpp \
  looped/world/loop.cpp looped/world/kind.cpp looped/world/world.cpp \
  --no-entry -sSTANDALONE_WASM=1 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=16MB -sSTACK_SIZE=1MB \
  -sEXPORTED_FUNCTIONS=_sim_buffer,_sim_dbuffer,_sim_text,_sim_file,_sim_thing_event,_sim_thing_apply,_sim_thing_at,_sim_thing_net,_sim_init,_sim_step,_sim_get,_sim_hold,_sim_release,_sim_place,_sim_awake \
  -o cloud/sim/room_sim.wasm
ls -l cloud/sim/room_sim.wasm
