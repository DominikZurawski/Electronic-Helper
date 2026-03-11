#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR=${1:-build/release}

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --target ppe ppe_gui

INSTALL_DIR="$BUILD_DIR/install"
cmake --install "$BUILD_DIR" --prefix "$INSTALL_DIR"

if command -v linuxdeployqt >/dev/null 2>&1; then
  linuxdeployqt "$INSTALL_DIR/bin/ppe_gui" -appimage
else
  echo "linuxdeployqt not found. Install it to build an AppImage."
  echo "Fallback: use the TGZ/ZIP from CPack."
fi

cpack --config "$BUILD_DIR/CPackConfig.cmake"
