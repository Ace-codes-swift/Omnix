#!/bin/bash
set -euo pipefail

# Usage:
#   ./scripts/sign_and_notarize.sh "Developer ID Application: Your Name (TEAMID)" com.your.bundle.id apple_id@example.com 'app-specific-password'
#
# Prereqs:
# - Xcode command line tools installed
# - Developer ID Certificate installed in login keychain
# - App-specific password created for your Apple ID
# - TEAMID matches your Apple Developer Team ID

if [ $# -lt 4 ]; then
  echo "Usage: $0 \"Developer ID Application: Your Name (TEAMID)\" com.your.bundle.id apple_id@example.com 'app-specific-password'"
  exit 1
fi

CERT="$1"
BUNDLE_ID="$2"
APPLE_ID="$3"
APP_PW="$4"

APP_ROOT="build/Omnix.app"
BIN_PATH="$APP_ROOT/Contents/MacOS/Omnix"
FRAMEWORKS_DIR="$APP_ROOT/Contents/Frameworks"
ENTITLEMENTS="scripts/entitlements.plist"

if [ ! -d "$APP_ROOT" ]; then
  echo "App bundle not found at $APP_ROOT. Build first."
  exit 1
fi

echo "[1/6] Preparing bundle..."
mkdir -p "$FRAMEWORKS_DIR"

# Strip debug symbols for release
strip -x "$BIN_PATH" || true

echo "[2/6] Codesigning app (hardened runtime)..."
codesign --force --deep --options runtime \
  --entitlements "$ENTITLEMENTS" \
  --sign "$CERT" \
  "$APP_ROOT"

echo "Verifying signature..."
codesign --verify --deep --strict --verbose=2 "$APP_ROOT"

echo "[3/6] Creating DMG..."
DMG_PATH="build/Omnix.dmg"
rm -f "$DMG_PATH"
hdiutil create -volname "Omnix" -srcfolder "$APP_ROOT" -ov -format UDZO "$DMG_PATH"

echo "[4/6] Notarizing DMG... (this can take several minutes)"
xcrun notarytool submit "$DMG_PATH" \
  --apple-id "$APPLE_ID" \
  --password "$APP_PW" \
  --team-id "$(echo "$CERT" | sed -n 's/.*(\(.*\)).*/\1/p')" \
  --wait

echo "[5/6] Stapling ticket..."
xcrun stapler staple "$DMG_PATH"

echo "[6/6] Done. Distributable DMG: $DMG_PATH"


