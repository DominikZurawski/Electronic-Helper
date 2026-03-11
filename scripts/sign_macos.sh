#!/usr/bin/env bash
set -euo pipefail

# TODO: Configure macOS code signing and notarization in CI.
# Requirements:
# - Apple Developer ID certificate in keychain
# - APPLE_SIGN_IDENTITY in CI secrets (e.g. "Developer ID Application: ...")
# - Notarization: APPLE_ID, APPLE_TEAM_ID, APPLE_APP_SPECIFIC_PASSWORD

if [ $# -lt 1 ]; then
  echo "Usage: ./scripts/sign_macos.sh <path-to-app-or-dmg>"
  exit 1
fi

TARGET="$1"

if [ -z "${APPLE_SIGN_IDENTITY:-}" ]; then
  echo "Signing disabled: APPLE_SIGN_IDENTITY not set."
  exit 0
fi

codesign --force --options runtime --sign "$APPLE_SIGN_IDENTITY" --deep "$TARGET"

# Optional notarization (disabled by default)
if [ -n "${APPLE_ID:-}" ] && [ -n "${APPLE_TEAM_ID:-}" ] && [ -n "${APPLE_APP_SPECIFIC_PASSWORD:-}" ]; then
  xcrun notarytool submit "$TARGET" --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" --password "$APPLE_APP_SPECIFIC_PASSWORD" --wait
  xcrun stapler staple "$TARGET"
fi
