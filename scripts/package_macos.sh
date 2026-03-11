#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR=${1:-build/release}

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --target ppe ppe_gui

INSTALL_DIR="$BUILD_DIR/install"
cmake --install "$BUILD_DIR" --prefix "$INSTALL_DIR"

if command -v macdeployqt >/dev/null 2>&1; then
  macdeployqt "$INSTALL_DIR/bin/ppe_gui.app"
else
  echo "macdeployqt not found. Install Qt6 and ensure it is in PATH."
fi

cpack --config "$BUILD_DIR/CPackConfig.cmake"
