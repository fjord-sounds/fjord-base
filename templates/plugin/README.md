<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/screenshots/hero-dark.webp">
  <img alt="@Name@ – @TAGLINE_LOWER@" src="docs/screenshots/hero-light.webp">
</picture>

# @Name@

<!-- One paragraph: what it is, for which DAWs (VST3, Audio Unit, LV2, standalone), the 2–3 things that make it
     special, how many factory presets. Then where the name comes from and that it is part of the Fjord family. -->
@Name@ is a … for every common DAW – VST3, Audio Unit, LV2 and a standalone app. It is part of the
[Fjord](https://github.com/fjord-sounds) family and shares its Scandinavian, minimal interface (light and dark) and
its sixteen effects.

[![CI](https://github.com/fjord-sounds/@name@/actions/workflows/ci.yml/badge.svg)](https://github.com/fjord-sounds/@name@/actions/workflows/ci.yml)
[![License: AGPL v3](https://img.shields.io/badge/license-AGPL--3.0-blue.svg)](LICENSE)

> **Made with AI.** @Name@ – the plugin code, the user interface, the tests and all factory presets – was written
> entirely by AI with Claude Opus, directed by prompts. Nothing is hand-programmed or hand-tuned.
> The code is open so you can read it, test it and judge for yourself; bug reports and pull requests are welcome.

<p>
  <a href="#a-tour">Tour</a> ·
  <a href="#sound-architecture">Sound architecture</a> ·
  <a href="#effects">Effects</a> ·
  <a href="#presets">Presets</a> ·
  <a href="#building">Building</a> ·
  <a href="#tests">Tests</a>
</p>

## Lightweight

- **Small**: the VST3 is about … MB, including all factory presets and the font.
- **Self-contained**: no sample library, no installer data, no account, no network access.
- **Calm on the audio thread**: no allocations or locks while rendering.

## Works in your DAW

| Format | Platforms | Hosts (examples) |
|---|---|---|
| **VST3** | macOS, Windows, Linux | Ableton Live, Bitwig Studio, Cubase, FL Studio, Studio One, Reaper, Ardour |
| **AU** | macOS | Logic Pro, GarageBand, MainStage |
| **LV2** | Linux | Ardour, Carla, Qtractor |
| **Standalone** | macOS, Windows, Linux | no DAW needed |

- **Tempo sync**: tempo-synced effects follow the host tempo and song position.
- **Any block size and sample rate**: oversized host blocks are split internally; tested at 44.1 to 96 kHz.
- **Presets in the host**: factory presets show up as programs.
- **Automation**: every knob is a host parameter.

## A tour

### Sound

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/screenshots/sound-dark.webp">
  <img alt="Sound page" src="docs/screenshots/sound-light.webp">
</picture>

<!-- One section per page: screenshot (light + dark), then what you can do there. -->

### FX

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/screenshots/fx-dark.webp">
  <img alt="FX page" src="docs/screenshots/fx-light.webp">
</picture>

Fifteen effects as tiles in two rows – drag them into any order; the clipper is always last. Every tile shows its
effect live. The selected effect opens below with a large graph (the EQ points can be dragged, wheel = Q), its controls
and its own presets. **FX CHAIN** loads a whole effect chain; **Lock FX** keeps it while you browse sounds.

## Sound architecture

| Part | What it does |
|---|---|
| **…** | … |
| **Output** | Preset level → effect chain → clipper → master. |

## Effects

@Name@ has the sixteen effects of the Fjord family, each with at least 40 presets of its own, and 35 effect chains:
Saturation, Lo-Fi, Transient, Compressor, Squash, EQ, Filter, Motion (chorus, ensemble, phaser, flanger, doubler,
vibrato), Delay, Reverb, Width, Tremolo, Frequency Shifter, Exciter, Harmonizer and the Clipper. Every effect is also
available as a plugin of its own ("Fjord Reverb" …). Controls and details:
[fjord-base/docs/effects.md](https://github.com/fjord-sounds/fjord-base/blob/main/docs/effects.md).

## Presets

… factory presets in … categories. User presets are saved to `~/Music/Fjord/@Name@/Presets/<Category>/*.@name@`
(plain XML) and appear in the browser with tags and favourites. Effect presets and chains are shared by all Fjord
plugins (`~/Music/Fjord/FX Presets/`, `~/Music/Fjord/FX Chains/`).

The factory presets live in [`presets/Factory/`](presets/Factory) and are embedded in the plugin at build time.

## Building

Requirements: CMake 3.22+ and a C++20 compiler. @Name@ is built on
[fjord-base](https://github.com/fjord-sounds/fjord-base), the shared code of all Fjord plugins. Clone it next to
this repository – otherwise CMake downloads the version pinned in `CMakeLists.txt` (`FJORD_BASE_TAG`). JUCE 8 comes
through fjord-base.

```bash
git clone https://github.com/fjord-sounds/fjord-base.git
git clone https://github.com/fjord-sounds/@name@.git
cd @name@
./build.sh                                     # macOS: AU + VST3 + Standalone, installs them, runs auval
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFJORD_COPY_PLUGIN=OFF && cmake --build build -j8   # elsewhere
```

## Tests

`@Name@RenderTest` renders @Name@ without a DAW:

```bash
cmake --build build --target @Name@RenderTest -j8
./build/@Name@RenderTest_artefacts/Release/@Name@RenderTest build/renders          # must print ALL PASSED
./build/@Name@RenderTest_artefacts/Release/@Name@RenderTest build/renders --sr 96000
```

It checks the sound and every factory preset for NaN and clipping, silence after the release, odd host block sizes
and that the project state brings back every parameter.

More modes: `--screenshot <sound|fx|browser|about> <preset|init> <light|dark> <out.png> [fx index]`,
`--loudness <json>`, `--param-info <tsv>`. README images: `tools/screenshots/shoot.sh` (needs cwebp; a `hero.mjs`
for the product image needs Playwright – `npm install` in `tools/screenshots`).

## Licences

@Name@ is licensed under the [AGPL-3.0](LICENSE). It uses JUCE (AGPLv3) and the Manrope font (SIL Open Font
License) – see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
