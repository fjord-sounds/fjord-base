#!/usr/bin/env bash
# README images of a fjord-sounds plugin: every page in the light and the dark theme, rendered offscreen by the
# plugin's RenderTest tool and compressed to WebP (cwebp), then the plugin's own product image (hero.mjs, Playwright).
#
#   ../fjord-base/tools/screenshots/shoot.sh [plugin dir]     (the plugin's tools/screenshots/shoot.sh calls this)
#
# The plugin describes its shots in tools/screenshots/shots.conf (bash):
#   TOOL=build/TordenRenderTest_artefacts/Release/TordenRenderTest
#   SHOTS=( "id|page|preset|fx index|notes" ... )     fx index and notes (e.g. 48,55,60) may be empty
# Output: docs/screenshots/<id>-<light|dark>.webp; raw PNGs in build/screenshots/raw (git-ignored).
# Requires a built RenderTest tool, cwebp (brew install webp) and, for hero.mjs, npm install in tools/screenshots.
set -euo pipefail
cd "${1:-.}"
CONF=tools/screenshots/shots.conf
[[ -f "$CONF" ]] || { echo "$CONF not found (run from the plugin folder)"; exit 1; }
# shellcheck source=/dev/null
source "$CONF"
[[ -x "$TOOL" ]] || { echo "$TOOL not found – build it first (cmake --build build --target $(basename "$TOOL"))"; exit 1; }
OUT=docs/screenshots
RAW=build/screenshots/raw
mkdir -p "$OUT" "$RAW"

for shot in "${SHOTS[@]}"; do
  IFS='|' read -r id page preset fx notes <<< "$shot"
  for theme in light dark; do
    if [[ -n "$notes" ]]; then
      "$TOOL" --screenshot "$page" "$preset" "$theme" "$RAW/$id-$theme.png" "${fx:--1}" "$notes"
    else
      "$TOOL" --screenshot "$page" "$preset" "$theme" "$RAW/$id-$theme.png" ${fx:+"$fx"}
    fi
    cwebp -quiet -q 86 -resize 1650 0 "$RAW/$id-$theme.png" -o "$OUT/$id-$theme.webp"
  done
  echo "shot  $id"
done

if [[ -f tools/screenshots/hero.mjs ]]; then
  node tools/screenshots/hero.mjs
fi
