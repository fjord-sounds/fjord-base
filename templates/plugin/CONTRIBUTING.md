# Contributing to @Name@

Thanks for your interest! Ideas that make @Name@ better are very welcome.

By contributing you agree that your work is licensed under the [AGPL-3.0](LICENSE), like the rest of the project.

## Build

@Name@ is built on [fjord-base](https://github.com/fjord-sounds/fjord-base) (shared UI, effects, build helpers).
Clone it next to this repository; without it CMake downloads the pinned version (`FJORD_BASE_TAG`).

```bash
git clone https://github.com/fjord-sounds/fjord-base.git
git clone https://github.com/fjord-sounds/@name@.git && cd @name@
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFJORD_COPY_PLUGIN=OFF
cmake --build build -j8
cmake --build build --target @Name@RenderTest -j8
./build/@Name@RenderTest_artefacts/Release/@Name@RenderTest build/renders      # must print ALL PASSED
```

`./build.sh` is the macOS shortcut that also installs the plugins and runs `auval`. Changes to the effects, the
header, the preset browser or the controls belong in fjord-base – see its
[docs](https://github.com/fjord-sounds/fjord-base/tree/main/docs).

## Rules that keep old projects and presets working

- All parameters are listed once in `Source/Parameters.h`. IDs never change; new parameters go to the **end** of the
  list and need a neutral default.
- Choice lists are append-only. Presets and the project state store plain values.
- If a change needs old project state to be converted, raise `kStateVersion` and convert it in `setStateInformation`
  (`fjord_base/presets/Migration.h` helps).
- The audio thread never allocates or locks.

## Presets

Factory presets live in `presets/Factory/<Category>/*.@name@` (one idea each) and are loudness matched to each other.

## Pull requests

- Keep them focused. Say what changes and why.
- UI changes: attach screenshots in the light **and** the dark theme (`@Name@RenderTest --screenshot …`).
- Update the README and [CHANGELOG.md](CHANGELOG.md) (section "Unreleased"), and refresh the README images
  (`tools/screenshots/shoot.sh`) when the UI or the presets change.
- Everything in the repository is English.
