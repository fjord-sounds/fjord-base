# Getting started

## Clone the repositories

fjord-base and the plugins live side by side. Plugins find fjord-base at `../fjord-base`:

```bash
mkdir fjord-sounds && cd fjord-sounds
git clone https://github.com/fjord-sounds/fjord-base.git
git clone https://github.com/fjord-sounds/torden.git
git clone https://github.com/fjord-sounds/huldra.git
git clone https://github.com/fjord-sounds/fjord.git
```

**fjord-base must be cloned.** A plugin's `CMakeLists.txt` uses the sibling checkout when it exists. Without it CMake
downloads the release pinned in `FJORD_BASE_TAG` when it configures – good for CI and one-off builds, but you cannot
change fjord-base that way. With the sibling checkout a change in fjord-base shows up in every plugin at once, so build
and test all plugins before you tag a new fjord-base version ([build-and-release.md](build-and-release.md)).

JUCE 8 comes through fjord-base: `fjord-base/external/JUCE` (git-ignored) if you put a checkout there, otherwise CMake
downloads the version pinned in `FJORD_JUCE_TAG` (`cmake/FjordBase.cmake`).

## Requirements

- CMake 3.22 or newer, a C++20 compiler
- macOS: Command Line Tools (no Xcode IDE needed; the scripts use Unix Makefiles)
- Windows: Visual Studio build tools; Linux: ALSA, X11, FreeType, fontconfig and OpenGL development packages
  (the list is in `.github/workflows/plugin-ci.yml`)
- Optional: `cwebp` and Node.js with Playwright for the README images

## Build a plugin

```bash
cd torden
./build.sh                 # macOS: AU + VST3 + Standalone, installs them, runs auval
SKIP_AUVAL=1 ./build.sh    # without auval
```

On Windows and Linux use CMake directly:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFJORD_COPY_PLUGIN=OFF
cmake --build build --config Release -j8
```

`build.sh` is a thin wrapper around `fjord-base/tools/build-plugin.sh`. `REFRESH_AU=1` restarts the macOS plugin
registry after adding a new Audio Unit; it is off by default because it can trigger an audio-device prompt in some
hosts.

## Run the tests

```bash
cmake --build build --target TordenRenderTest -j8
./build/TordenRenderTest_artefacts/Release/TordenRenderTest build/renders      # must print ALL PASSED
```

More in [testing.md](testing.md).

## Build and install everything

```bash
cd fjord-base
tools/install-all.sh                 # every plugin next to fjord-base (fjord, torden, huldra, hamar …), then the sixteen effects
tools/install-all.sh torden huldra   # only these plugins (and the effects)
tools/install-all.sh --no-effects    # plugins only;  --effects-only: effects only
```

`SKIP_AUVAL=1` skips auval, `REFRESH_AU=1` restarts the macOS plugin registry first.

## Build the single-effect plugins

```bash
cd fjord-base
tools/build-effects.sh                 # all sixteen: "Fjord Reverb" …, installs them, runs auval
tools/build-effects.sh reverb delay    # only these
TEST=1 tools/build-effects.sh          # also runs FjordEffectsTest
```

## Start a new plugin

```bash
python3 fjord-base/tools/new-plugin.py Skerry Skry --tagline "SYNTH"
```

See [new-plugin.md](new-plugin.md).
