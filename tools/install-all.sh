#!/usr/bin/env bash
# Builds and installs every Fjord plugin next to fjord-base (every sibling folder with a build.sh: fjord, torden, huldra, hamar …)
# and the single-effect plugins ("Fjord Reverb" …).
#
#   tools/install-all.sh                  all plugins, then all effects
#   tools/install-all.sh torden huldra    only these plugins (and the effects, unless --no-effects)
#   tools/install-all.sh --no-effects     plugins only
#   tools/install-all.sh --effects-only   only the effect plugins
#   SKIP_AUVAL=1  no auval runs     REFRESH_AU=1  restart the macOS plugin registry first (see build-plugin.sh)
#
# Installs AU + VST3 (+ the standalone apps are built, not installed) into the user plugin folders; stops at the first failure.
set -euo pipefail
BASE="$(cd "$(dirname "$0")/.." && pwd)"
ROOT="$(dirname "$BASE")"

plugins=()
with_plugins=1
with_effects=1
for arg in "$@"; do
  case "$arg" in
    --no-effects)   with_effects=0 ;;
    --effects-only) with_plugins=0 ;;
    -h|--help)      sed -n '2,12p' "$0"; exit 0 ;;
    *)              plugins+=("$arg"); ;;
  esac
done

if [[ $with_plugins == 1 ]]; then
  if [[ ${#plugins[@]} -eq 0 ]]; then
    for dir in "$ROOT"/*/; do
      name="$(basename "$dir")"
      [[ "$name" != "fjord-base" && -x "$dir/build.sh" ]] && plugins+=("$name")
    done
  fi
  for name in "${plugins[@]}"; do
    [[ -x "$ROOT/$name/build.sh" ]] || { echo "No plugin '$name' next to fjord-base (needs $ROOT/$name/build.sh)"; exit 1; }
    echo "==> $name"
    # Hand REFRESH_AU only to the first build: one registry restart is enough
    (cd "$ROOT/$name" && ./build.sh)
    export REFRESH_AU=0
  done
fi

if [[ $with_effects == 1 ]]; then
  echo "==> effects"
  "$BASE/tools/build-effects.sh"
fi
echo "Done."
