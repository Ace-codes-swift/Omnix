#!/bin/bash
set -euo pipefail

# Local packaging script (no Apple Developer account required)
# - Builds Omnix if needed
# - Ad-hoc signs the app (no notarization)
# - Creates a DMG that includes a Read Me with GUI-only open instructions

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
APP_ROOT="$BUILD_DIR/Omnix.app"
STAGING_DIR="$BUILD_DIR/Omnix_dmg_staging"
DMG_PATH="$BUILD_DIR/Omnix-Local.dmg"

echo "[1/4] Ensuring build exists..."
if [ ! -d "$APP_ROOT" ]; then
  echo "App not found at $APP_ROOT. Building..."
  "$PROJECT_ROOT/build.sh"
fi

echo "[2/4] Ad-hoc signing app for better compatibility (no account required)..."
# Ad-hoc sign the entire bundle. This is optional but can reduce launch issues on some systems.
# We intentionally DO NOT use hardened runtime or entitlements here.
if command -v codesign >/dev/null 2>&1; then
  codesign --force --deep --sign - "$APP_ROOT" || true
  # Best-effort verification (non-fatal)
  codesign --verify --deep --verbose=2 "$APP_ROOT" || true
else
  echo "codesign not found; skipping ad-hoc signing."
fi

echo "[3/4] Preparing DMG contents..."
rm -rf "$STAGING_DIR"
mkdir -p "$STAGING_DIR"
cp -R "$APP_ROOT" "$STAGING_DIR/"

cat > "$STAGING_DIR/Read Me - How to Open Omnix.txt" <<'EOF'
Omnix
=====

Thanks for downloading Omnix!

This build is distributed without Apple notarization, so the first launch needs a one-time approval using macOS' GUI only (no Terminal required).

Install (optional but recommended):
1) Open this DMG
2) Drag Omnix.app into your Applications folder

First launch (one-time):
• Right-click (or control-click) Omnix.app → Open → Open
  - If you only see “Open Anyway” in System Settings, try this:
    System Settings → Privacy & Security → scroll to Security section → “Open Anyway” for Omnix

After this one-time approval, you can open Omnix normally by double-clicking it.

Notes:
• This build is ad-hoc signed (no developer certificate). That’s expected.
• If macOS still blocks the app, ensure you performed the right-click → Open step once.

Enjoy!
EOF

echo "[4/4] Creating DMG..."
rm -f "$DMG_PATH"
hdiutil create -volname "Omnix" -srcfolder "$STAGING_DIR" -ov -format UDZO "$DMG_PATH"

echo "Done. DMG created at: $DMG_PATH"
echo "Share this DMG. Recipients can follow the Read Me for a GUI-only first launch."


