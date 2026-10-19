#!/usr/bin/env bash
# Installs the pinned Emscripten toolchain if missing. Runs automatically at
# the start of every Claude cloud session (.claude/settings.json), and is safe
# to run by hand. After it runs: source /opt/emsdk/emsdk_env.sh
set -euo pipefail

EMSDK_VERSION=6.0.12
EMSDK_DIR=/opt/emsdk

if [ ! -d "$EMSDK_DIR" ]; then
  git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR" >/dev/null 2>&1
fi
if [ ! -f "$EMSDK_DIR/.installed-$EMSDK_VERSION" ]; then
  "$EMSDK_DIR/emsdk" install "$EMSDK_VERSION" >/dev/null
  "$EMSDK_DIR/emsdk" activate "$EMSDK_VERSION" >/dev/null
  touch "$EMSDK_DIR/.installed-$EMSDK_VERSION"
fi
echo "Emscripten $EMSDK_VERSION ready. Run: source $EMSDK_DIR/emsdk_env.sh"

# astcenc (ARM, Apache 2.0): compresses model textures at build time
# (tools/assets/astc_pack.py, run by CMake). A build tool only.
ASTCENC_VERSION=5.2.0
if [ ! -x /opt/astcenc/bin/astcenc-avx2 ]; then
  tmp=$(mktemp -d)
  if curl -sSL -o "$tmp/astcenc.zip" "https://github.com/ARM-software/astc-encoder/releases/download/$ASTCENC_VERSION/astcenc-$ASTCENC_VERSION-linux-x64.zip" &&
     mkdir -p /opt/astcenc && unzip -o -q "$tmp/astcenc.zip" -d /opt/astcenc; then
    echo "astcenc $ASTCENC_VERSION installed."
  else
    echo "Note: astcenc download failed (builds will skip compressed textures)."
  fi
  rm -rf "$tmp"
fi

# The Cloudflare room's local dev server (wrangler dev, used by
# tools/room_test.mjs). Skipped if already installed; never fails the setup.
cd "$(dirname "$0")/.."
if [ -f cloud/package-lock.json ] && [ ! -x cloud/node_modules/.bin/wrangler ]; then
  npm ci --prefix cloud --no-audit --no-fund >/dev/null 2>&1 && echo "Cloud dev server (wrangler) installed." || echo "Note: npm ci --prefix cloud failed (room_test needs it)."
fi

# The room's physics module (cloud/sim/room_sim.wasm, not in git): the
# Worker and room_test need it. Never fails the setup.
if [ ! -f cloud/sim/room_sim.wasm ] && [ -f "$EMSDK_DIR/emsdk_env.sh" ]; then
  (source "$EMSDK_DIR/emsdk_env.sh" >/dev/null 2>&1 && bash cloud/sim/build.sh >/dev/null 2>&1) && echo "Room physics built (cloud/sim/room_sim.wasm)." || echo "Note: cloud/sim/build.sh failed (room_test needs it)."
fi
