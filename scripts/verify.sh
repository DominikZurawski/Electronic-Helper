#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR=${1:-build/debug}

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$BUILD_DIR" --target ppe_gui
ctest --test-dir "$BUILD_DIR" --output-on-failure

printf "\nOK: build + tests finished successfully.\n"
