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
