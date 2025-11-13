#!/bin/bash
set -euo pipefail

# Local Installer (.pkg) builder (no Apple Developer account required)
# - Builds Omnix if needed
# - Stages payload to install into /Applications
# - Generates a Distribution with Welcome + Read Me pages
# - Produces an unsigned .pkg (users may need to right-click → Open the pkg first time)

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
APP_ROOT="$BUILD_DIR/Omnix.app"
PKGROOT="$BUILD_DIR/pkgroot"
PKG_COMPONENT="$BUILD_DIR/Omnix.component.pkg"
PKG_DISTRIBUTION="$BUILD_DIR/Omnix_Distribution.xml"
PKG_OUTPUT="$BUILD_DIR/Omnix-Installer.pkg"
RES_DIR="$PROJECT_ROOT/scripts/pkg_resources"

echo "[1/6] Ensuring build exists..."
if [ ! -d "$APP_ROOT" ]; then
  echo "App not found at $APP_ROOT. Building..."
  "$PROJECT_ROOT/build.sh"
fi

echo "[2/6] Preparing payload at $PKGROOT ..."
rm -rf "$PKGROOT"
mkdir -p "$PKGROOT"
cp -R "$APP_ROOT" "$PKGROOT/"

echo "[3/6] Creating component package..."
APP_VERSION="1.0"
IDENTIFIER="com.example.omnix"
pkgbuild \
  --root "$PKGROOT" \
  --identifier "$IDENTIFIER" \
  --version "$APP_VERSION" \
  --install-location "/Applications" \
  "$PKG_COMPONENT"

echo "[4/6] Ensuring installer resources (Welcome / Read Me)..."
mkdir -p "$RES_DIR"
if [ ! -f "$RES_DIR/welcome.html" ]; then
  cat > "$RES_DIR/welcome.html" <<'HTML'
<!doctype html>
<html><head><meta charset="utf-8"><title>Welcome to Omnix</title>
<style>body{font:14px -apple-system,Helvetica,Arial,sans-serif;line-height:1.5;margin:24px;color:#222}</style>
</head><body>
<h1>Welcome to Omnix</h1>
<p>This installer will place Omnix in your Applications folder.</p>
<p>Because this build is not notarized (no Apple Developer account), the first app launch may require a one-time approval using the GUI only. Full details are on the next screen.</p>
</body></html>
HTML
fi

if [ ! -f "$RES_DIR/readme.html" ]; then
  cat > "$RES_DIR/readme.html" <<'HTML'
<!doctype html>
<html><head><meta charset="utf-8"><title>Read Me - Omnix</title>
<style>body{font:14px -apple-system,Helvetica,Arial,sans-serif;line-height:1.5;margin:24px;color:#222} code{background:#f3f3f3;padding:2px 4px;border-radius:4px}</style>
</head><body>
<h2>After Installation</h2>
<ol>
  <li>Open Finder → Applications</li>
  <li>Locate <strong>Omnix.app</strong></li>
  <li>Right-click (or control-click) → <strong>Open</strong> → <strong>Open</strong> (one-time)</li>
</ol>
<p>Alternatively, System Settings → Privacy &amp; Security → scroll to Security → click <strong>Open Anyway</strong> for Omnix.</p>
<p>After this one-time approval, you can double‑click Omnix normally.</p>
</body></html>
HTML
fi

echo "[5/6] Generating Distribution.xml..."
cat > "$PKG_DISTRIBUTION" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
  <title>Omnix</title>
  <domains enable_localSystem="true" enable_currentUserHome="true" enable_anywhere="false"/>
  <welcome file="welcome.html"/>
  <readme file="readme.html"/>
  <options customize="never" require-scripts="false"/>
  <choices-outline>
    <line choice="default">
      <line choice="OmnixChoice"/>
    </line>
  </choices-outline>
  <choice id="default" title="Default">
    <pkg-ref id="${IDENTIFIER}"/>
  </choice>
  <choice id="OmnixChoice" visible="false">
    <pkg-ref id="${IDENTIFIER}"/>
  </choice>
  <pkg-ref id="${IDENTIFIER}">Omnix.component.pkg</pkg-ref>
</installer-gui-script>
XML

echo "[6/6] Building final installer package..."
productbuild \
  --distribution "$PKG_DISTRIBUTION" \
  --resources "$RES_DIR" \
  --package-path "$BUILD_DIR" \
  "$PKG_OUTPUT"

echo "Done. Installer created at: $PKG_OUTPUT"
echo "Note: This .pkg is UNSIGNED. Recipients may need to right-click → Open the .pkg the first time."


