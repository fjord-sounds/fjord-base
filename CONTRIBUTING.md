# Contributing to fjord-base

Thanks for your interest! fjord-base is the shared foundation of several plugins, so changes here reach all of them –
keep them focused and careful.

By contributing you agree that your work is licensed under the [AGPL-3.0](LICENSE), like the rest of the project.

## Build

```bash
git clone https://github.com/fjord-sounds/fjord-base.git && cd fjord-base
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFJORD_COPY_PLUGIN=OFF
cmake --build build -j8
./build/effects/FjordEffectsTest_artefacts/Release/FjordEffectsTest      # must print ALL PASSED
```

JUCE 8 is downloaded by CMake on the first configure (or use a checkout in `external/JUCE`).

To see how a change affects the plugins, clone them next to fjord-base ([docs/getting-started.md](docs/getting-started.md))
and build and test each: Fjord, Torden, Huldra. A change in fjord-base is only done when all of them still pass.

## Rules

- Read [docs/](docs/README.md): architecture, code style, design system and the
  [compatibility rules](docs/compatibility-rules.md). Parameter IDs never change, lists are append-only, ranges only grow.
- Nothing plugin-specific goes into fjord-base, and no colour is fixed in the base: colours come from the plugin's `Brand`.
- The audio thread never allocates or locks.
- UI changes: attach screenshots in the light **and** the dark theme (`FjordEffectsTest --screenshot …`).
- New effect: append it to `fx::Module` and `FJORD_FX_PARAMS`, add its graph, at least 40 presets, an entry in
  `effects/CMakeLists.txt` with a unique plugin code, and document it in `docs/effects.md`.
- Update the README, [CHANGELOG.md](CHANGELOG.md) (section "Unreleased") and the docs when behaviour changes.
- Everything in the repository is English.

## Pull requests

Say what changes and why, which plugins you built and tested, and attach the screenshots for UI changes.
