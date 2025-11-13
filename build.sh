#!/bin/bash
set -euo pipefail

# Build script for Omnix

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

echo "Building Omnix..."

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Running CMake..."
cmake "$PROJECT_ROOT"

echo "Building project..."
make

echo "Build successful!"
echo "Omnix.app created in $BUILD_DIR"
echo "To run: open $BUILD_DIR/Omnix.app"

# Optional signing + notarization
# Provide these environment variables to enable:
#   CERT       → e.g. "Developer ID Application: Your Name (TEAMID)"
#   BUNDLE_ID  → e.g. com.atech.Omnix (should match Info.plist)
#   APPLE_ID   → your Apple ID email
#   APP_PW     → app-specific password

if [[ -n "${CERT:-}" && -n "${BUNDLE_ID:-}" && -n "${APPLE_ID:-}" && -n "${APP_PW:-}" ]]; then
  echo "Signing and notarizing..."
  "$PROJECT_ROOT/scripts/sign_and_notarize.sh" "$CERT" "$BUNDLE_ID" "$APPLE_ID" "$APP_PW"
  echo "Signing + notarization complete. Distributable DMG at: $BUILD_DIR/Omnix.dmg"
else
  echo "Skipping signing/notarization (CERT/BUNDLE_ID/APPLE_ID/APP_PW not all set)."
  echo "To sign and notarize, run:"
  echo "CERT=\"Developer ID Application: Your Name (TEAMID)\" BUNDLE_ID=com.atech.Omnix APPLE_ID=you@example.com APP_PW='app-password' ./build.sh"
fi
