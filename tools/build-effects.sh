#!/usr/bin/env bash
# Builds the single-effect plugins ("Fjord Reverb" ...), installs them and validates the AUs.
#   tools/build-effects.sh                 all effects
#   tools/build-effects.sh reverb delay    only these (ids as in effects/CMakeLists.txt)
#   SKIP_AUVAL=1 / REFRESH_AU=1 as in build-plugin.sh;  TEST=1 also runs FjordEffectsTest
set -euo pipefail
cd "$(dirname "$0")/.."
ONLY=$(IFS=,; echo "$*")
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DFJORD_EFFECTS_ONLY="$ONLY"
JOBS="$(sysctl -n hw.ncpu 2>/dev/null || nproc)"
TARGETS=$(grep -oE '"[a-z]+\|Fjord [^|]+\|F[a-z]{3}"' effects/CMakeLists.txt | tr -d '"')
if [[ "${REFRESH_AU:-0}" == "1" ]]; then killall -9 AudioComponentRegistrar 2>/dev/null || true; fi
while IFS='|' read -r id name code; do
  if [[ -n "$ONLY" && ",$ONLY," != *",$id,"* ]]; then continue; fi
  target=$(echo "$name" | tr -d ' -')
  cmake --build build --target "${target}_All" -j"$JOBS"
  if [[ "$(uname)" == "Darwin" && "${SKIP_AUVAL:-0}" != "1" ]]; then
    auval -v aufx "$code" Fjrd | tail -n 2
  fi
done <<< "$TARGETS"
if [[ "${TEST:-0}" == "1" ]]; then
  cmake --build build --target FjordEffectsTest -j"$JOBS"
  ./build/effects/FjordEffectsTest_artefacts/Release/FjordEffectsTest
fi
