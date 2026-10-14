#!/usr/bin/env bash
# Builds a Fjord plugin (AU + VST3 + Standalone on macOS) without the Xcode IDE, installs it and validates the AU.
# Called by the plugin's own build.sh:  "$BASE/tools/build-plugin.sh" <plugin dir> <Target> <aumu|aufx> <code> [Release|Debug]
#   SKIP_AUVAL=1   no auval run
#   REFRESH_AU=1   restart the macOS plugin registry first (only after adding a new AU – it can trigger an
#                  audio-device prompt in Logic, so it is off by default)
set -euo pipefail
DIR="$1"; TARGET="$2"; AU_TYPE="$3"; CODE="$4"; CONFIG="${5:-Release}"
cd "$DIR"
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="$CONFIG"
cmake --build build --config "$CONFIG" --target "${TARGET}_All" -j"$(sysctl -n hw.ncpu 2>/dev/null || nproc)"
if [[ "${REFRESH_AU:-0}" == "1" ]]; then
  killall -9 AudioComponentRegistrar 2>/dev/null || true
fi
if [[ "$(uname)" == "Darwin" && "${SKIP_AUVAL:-0}" != "1" ]]; then
  auval -v "$AU_TYPE" "$CODE" Fjrd | tail -n 5
fi
echo "Standalone: build/${TARGET}_artefacts/$CONFIG/Standalone/${TARGET}.app"
