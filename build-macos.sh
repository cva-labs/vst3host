#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$ROOT/build-macos"

cmake -S "$ROOT" -B "$BUILD" -G Xcode \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build "$BUILD" --config Release --parallel 4

APP="$BUILD/VST3PlayerHost_artefacts/Release/VST3 Player Host.app"
codesign --force --deep --sign - "$APP"
ditto -c -k --sequesterRsrc --keepParent "$APP" "$ROOT/VST3-Player-Host-macOS-Universal.zip"
echo "Build complete: $ROOT/VST3-Player-Host-macOS-Universal.zip"
