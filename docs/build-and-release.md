# Build and release

## CMake

A plugin's `CMakeLists.txt` is short:

```cmake
cmake_minimum_required(VERSION 3.22)
project(Torden VERSION 0.2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# … macOS deployment target and architectures …

set(FJORD_BASE_TAG "v0.2.0")
set(FJORD_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../fjord-base" CACHE PATH "Local fjord-base checkout")
if(EXISTS "${FJORD_BASE_DIR}/cmake/FjordBase.cmake")
    add_subdirectory("${FJORD_BASE_DIR}" "${CMAKE_BINARY_DIR}/fjord-base")
else()
    include(FetchContent)
    FetchContent_Declare(fjord_base GIT_REPOSITORY https://github.com/fjord-sounds/fjord-base.git
                                    GIT_TAG ${FJORD_BASE_TAG} GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(fjord_base)
endif()

fjord_add_plugin(Torden PRODUCT_NAME "Torden" CODE Trdn BUNDLE_ID "com.fjord.torden"
                 VST3_CATEGORIES Instrument Drum PRESETS "presets/Factory/*.torden")
```

`fjord_add_plugin` creates the plugin target (VST3 + Standalone everywhere, AU on macOS, LV2 on Linux; manufacturer
`Fjrd`), embeds the presets, sets compile definitions (`FJORD_GIT_HASH`, `FJORD_BASE_VERSION`), warnings and LTO flags
and adds the `<Target>RenderTest` console tool. Options: `EFFECT`, `RESOURCES`, `INCLUDES`, `DEFINITIONS`, `LINK`,
`SOURCES`, `JUCE_ARGS` (e.g. `MICROPHONE_PERMISSION_ENABLED TRUE`), `RENDER_TEST`, `ICON`.

Cache options: `FJORD_COPY_PLUGIN` (install to the user plugin folders after building; OFF in CI), `FJORD_BASE_DIR`,
`FJORD_JUCE_DIR`, `FJORD_JUCE_TAG`. Plugin codes are four characters, unique among all Fjord plugins and effect plugins,
and never change (hosts store them).

## Scripts

| Script | What it does |
|---|---|
| `<plugin>/build.sh` | builds AU + VST3 + Standalone with Unix Makefiles, installs them, runs `auval`; `SKIP_AUVAL=1`, `REFRESH_AU=1` |
| `fjord-base/tools/install-all.sh [plugin …]` | builds and installs every plugin next to fjord-base and the effect plugins (`--no-effects`, `--effects-only`) |
| `fjord-base/tools/build-effects.sh [effect …]` | the same for the single-effect plugins; `TEST=1` also runs FjordEffectsTest |
| `fjord-base/tools/release/bump_version.py x.y.z` | sets the version and rolls the changelog (run from the repository) |
| `fjord-base/tools/screenshots/shoot.sh` | README images, see [testing.md](testing.md) |
| `fjord-base/tools/new-plugin.py` | creates a plugin from the template |

## Continuous integration

Plugins call reusable workflows from fjord-base; their own workflow files are a few lines:

```yaml
# .github/workflows/ci.yml
jobs:
  ci:
    uses: fjord-sounds/fjord-base/.github/workflows/plugin-ci.yml@main
    with: { target: Torden }
```

- **plugin-ci** builds the plugin and its RenderTest on macOS, Windows and Linux and requires `ALL PASSED`.
- **plugin-release** (on a `v*` tag) checks the tag against `project(<Name> VERSION …)`, builds Windows and Linux
  binaries, zips them with LICENSE and README, and publishes a GitHub release with the changelog section as notes.
  macOS binaries are not published: macOS users build from source.
- **base-update** (weekly) opens a pull request when fjord-base has a newer tag than `FJORD_BASE_TAG`.
- fjord-base itself has `ci` (builds the effect plugins and FjordEffectsTest), `release` (an effects bundle) and
  `juce-update` (weekly pull request for a newer JUCE; the pin lives in `cmake/FjordBase.cmake`).

CI configures with `FJORD_COPY_PLUGIN=OFF` and gets fjord-base through the pinned tag, so the tag must exist before a
plugin that pins it is pushed.

## Versions and releases

- A plugin's version is `project(<Name> VERSION x.y.z)`. It becomes the plugin version (VST3, AU, LV2), the About panel
  text and the test tool's version.
- fjord-base's version is the `VERSION` file; its tag is `v<VERSION>`.
- Release a plugin: update the changelog under "Unreleased", then
  `python3 ../fjord-base/tools/release/bump_version.py 0.2.0`, review the diff, commit, tag `v0.2.0`, push the tag.
- Change fjord-base: build **and test every plugin** against the local checkout, update its changelog, bump and tag
  fjord-base, then raise `FJORD_BASE_TAG` in the plugins (the weekly workflow proposes it).
- JUCE is pinned once, in fjord-base (`FJORD_JUCE_TAG`). After a JUCE update, build and test every plugin.
- Licence: AGPL-3.0 (JUCE is used under AGPLv3).
