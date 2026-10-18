#!/bin/sh
# Rebuild models and embed them as base64 JS (binary fetch is blocked in the hosted preview).
# Needs Poly Haven CC0 sources in /tmp/ph (see nature.py / pack_textures.py).
# usage: BLENDER=/path/to/blender sh blender/embed.sh
cd "$(dirname "$0")"
B=${BLENDER:-blender}
$B -b -P car.py -- /tmp/car && $B -b -P props.py -- /tmp/props
{ printf 'export const CAR_GLB = "'; base64 -w0 /tmp/car.glb; printf '";\nexport const PROPS_GLB = "'; base64 -w0 /tmp/props.glb; printf '";\n'; } > ../assets.js
$B -b -P nature.py -- cards && python3 cards_post.py && $B -b -P nature.py -- build
{ printf 'export const NATURE_GLB = "'; base64 -w0 /tmp/nature.glb; printf '";\n'; } > ../nature_glb.js
python3 pack_textures.py
