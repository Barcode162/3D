// The C++ files the native light tools build from (tools/light.mjs,
// tools/bake.mjs): the bar's scene and the light code, no GPU.
const WORLD = 'looped/world/loop.cpp looped/world/kind.cpp looped/world/world.cpp looped/world/catalog.cpp looped/json.cpp';
const SCENE = `bar/bar_scene.cpp bar/scene_names.cpp bar/world_files.cpp bar/materials.cpp looped/procedural.cpp looped/mesh_data.cpp looped/world/place_layout.cpp ${WORLD}`;
export const LIGHT_SOURCES = `looped/bake/light_paths.cpp looped/radiosity.cpp looped/bvh.cpp looped/scene_geometry.cpp looped/gltf.cpp ${SCENE}`;
