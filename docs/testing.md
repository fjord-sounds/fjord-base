# Testing

There are no automated unit tests in the usual sense; the **RenderTest** is the test suite and the tool belt of every
plugin. It runs offline (no DAW, no audio device) and must print `ALL PASSED`.

```bash
cmake --build build --target TordenRenderTest -j8
./build/TordenRenderTest_artefacts/Release/TordenRenderTest build/renders            # all tests
./build/TordenRenderTest_artefacts/Release/TordenRenderTest build/renders --sr 96000 # another sample rate
```

## What the tests check

- the sound engine: every important mode, finite output, level limits, silence after the release, no DC
- every **factory preset**: renders, finite, peak ≤ 1, not silent
- retriggering and voice handling, sequencers and clocks where a plugin has them
- odd host block sizes (1, 37, 512, 4096 samples) give valid output
- **state round trip**: save and restore the project state – every parameter, extras and the effect order come back
- presets: loading, saving and undo bring back parameters and extras
- effects: `FjordEffectsTest` renders every effect and every effect preset at 48 and 96 kHz, checks finite output,
  peak, silence, and that every effect has at least 40 presets

Tests never write user settings (`Theme::get().setPersistent(false)`) and ignore user presets
(`PresetManager::ignoreUserPresets`, `ChainPresets::ignoreUserPresets`).

## Tool modes

| Mode | What it does |
|---|---|
| `--screenshot <page> <preset\|init> <light\|dark> <out.png> [fx index] [notes]` | Draws the real UI offscreen to a PNG (write it outside the repository, e.g. a scratch folder) |
| `--param-info <tsv>` | id, kind, range and default of every parameter (for preset tooling) |
| `--params <file>` | list of all parameter ids |
| `--loudness <json>` | RMS and peak per factory preset (measured without the preset trim) |
| `--strength [json]` (FjordEffectsTest) | how far every effect preset moves the sound, in dB relative to the dry phrase; prints weakest / median / strongest per effect |
| `--single <preset> <out.wav>` | renders one sound to a WAV |
| `--icon <png>` | draws the app icon |

Modes differ a little per plugin; each plugin's README lists its own.

## Screenshots

`tools/screenshots/shoot.sh` in a plugin renders every shot listed in `tools/screenshots/shots.conf` in both themes,
compresses them to WebP (`docs/screenshots/<id>-<light|dark>.webp`) and then runs the plugin's `hero.mjs` (product
image, Playwright). `shots.conf`:

```bash
TOOL=build/TordenRenderTest_artefacts/Release/TordenRenderTest
SHOTS=(
  "kick|kick|Thunder Hammer|"           # id | page | preset | fx index | notes
  "fx|fx|Warehouse Rumble|5"
)
```

The banner of fjord-base's README (the three plugins in perspective) is composed by `fjord-base/tools/screenshots/banner.mjs`
from the page shots of Fjord, Torden and Huldra (`node banner.mjs`, needs `npm install` in that folder); refresh it when the UI
of one of them changes noticeably.

Look at the screenshots before you report a UI change, in **both** themes. They never show personal data: no About page,
no user presets.

## Plugin validation

- macOS: `auval -v aumu <Code> Fjrd` (instruments) or `auval -v aufx <Code> Fjrd` (effects); `build.sh` runs it.
- [pluginval](https://github.com/Tracktion/pluginval) at strictness 10 on the VST3 is recommended before a release.

## CI

`plugin-ci` builds the plugin and its RenderTest on macOS, Windows and Linux and requires `ALL PASSED`
([build-and-release.md](build-and-release.md)).
