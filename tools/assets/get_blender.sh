#!/usr/bin/env bash
# Installs Blender (headless use only, to build assets) into /opt/blender.
# Not part of setup.sh: it's ~350 MB and only needed to rebuild models.
set -euo pipefail
if [ -x /opt/blender/blender ]; then /opt/blender/blender --version | head -1; exit 0; fi
mkdir -p /opt/blender
curl -fsSL https://download.blender.org/release/Blender4.5/blender-4.5.14-linux-x64.tar.xz \
  | tar -xJ --strip-components=1 -C /opt/blender
/opt/blender/blender --version | head -1
