#!/bin/bash

set -euo pipefail

# Determine project root (directory where this script lives)
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENGINE_DIR="$ROOT_DIR/Engine"
ENGINE_BUILD_DIR="$ENGINE_DIR/lib"
ENGINE_INCLUDE_DIR="$ENGINE_DIR/include"
ENGINELIB_OUT_DIR="$ROOT_DIR/src/EngineLib"

echo "Building Engine static library with CMake..."
cmake -S "$ENGINE_DIR" -B "$ENGINE_BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$ENGINE_BUILD_DIR" --config Release

echo "Ensuring output directory exists at $ENGINELIB_OUT_DIR..."
mkdir -p "$ENGINELIB_OUT_DIR"

echo "Copying public headers from $ENGINE_INCLUDE_DIR to $ENGINELIB_OUT_DIR (overwriting existing files)..."
cp -a "$ENGINE_INCLUDE_DIR/." "$ENGINELIB_OUT_DIR/"

LIB_SRC="$ENGINE_BUILD_DIR/libEngine.a"
if [[ ! -f "$LIB_SRC" ]]; then
  echo "Error: Expected library not found at $LIB_SRC"
  exit 1
fi

echo "Copying built library from $LIB_SRC to $ENGINELIB_OUT_DIR (overwriting existing file if present)..."
cp -f "$LIB_SRC" "$ENGINELIB_OUT_DIR/"

echo "Engine library and headers have been updated in src/EngineLib."


