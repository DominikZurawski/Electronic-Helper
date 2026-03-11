#!/usr/bin/env bash
set -euo pipefail

ICON_PNG="assets/icon.png"
ICONSET_DIR="assets/icon.iconset"
ICNS_OUT="assets/icon.icns"

if [ ! -f "$ICON_PNG" ]; then
  echo "Missing $ICON_PNG. Run scripts/generate_icons.py first."
  exit 1
fi

rm -rf "$ICONSET_DIR"
mkdir -p "$ICONSET_DIR"

# Create required sizes
sizes=(16 32 64 128 256 512 1024)
for s in "${sizes[@]}"; do
  sips -z $s $s "$ICON_PNG" --out "$ICONSET_DIR/icon_${s}x${s}.png" >/dev/null
  if [ $s -lt 1024 ]; then
    sips -z $((s*2)) $((s*2)) "$ICON_PNG" --out "$ICONSET_DIR/icon_${s}x${s}@2x.png" >/dev/null
  fi
done

iconutil -c icns "$ICONSET_DIR" -o "$ICNS_OUT"
rm -rf "$ICONSET_DIR"

echo "Generated: $ICNS_OUT"
