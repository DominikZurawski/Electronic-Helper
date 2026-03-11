#!/usr/bin/env bash
set -euo pipefail

# Prefer Wayland when available, fallback to X11. Safe for non-Wayland setups.
if [ -n "${WAYLAND_DISPLAY:-}" ]; then
  export QT_QPA_PLATFORM="wayland;xcb"
fi

exec "${1:-ppe_gui}" "${@:2}"
