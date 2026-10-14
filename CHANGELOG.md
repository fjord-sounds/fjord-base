# Changelog

All notable changes to fjord-base. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
versions follow [Semantic Versioning](https://semver.org/).

## [Unreleased]

## [0.2.0] – 2026-10-14
- **Locks** for every plugin: `locks::LockSet`, the padlock menu (`LockMenu`, `MenuLayout`), `LockButton`, and
  `PresetManagerBase::setLocks`; the effect locks (whole chain and one per effect) come with the base. The template and the
  documentation include them.
- Banner image for the README (`tools/screenshots/banner.mjs`).
- `tools/install-all.sh` builds and installs every plugin next to fjord-base and all effect plugins in one go.
- **Window size** in the options menu of the preset bar (75, 100, 125, 150 %).
- Effect presets are stronger: the Exciter adds more harmonics, and weak presets of Exciter, Harmonizer, Squash, Transient,
  Compressor, EQ, Motion, Delay, Reverb, Width, Saturation and Tremolo were raised so that nearly every preset is clearly
  audible. `FjordEffectsTest --strength` reports how far each preset moves the sound.

## [0.1.0] – first version
- The shared UI of the Fjord plugins: frame with header, logo dot, meter, tabs, preset bar, browser, save dialog, About
  panel, undo/redo, theme, scaling; knobs, selectors, sections, keyboard, Manrope; every colour comes from the plugin's
  `Brand`.
- Sixteen effects with one parameter block (`fx_<effect>_<param>`): Saturation, Lo-Fi, Transient, Compressor, Squash, EQ,
  Filter, Motion, Delay, Reverb, Width, Tremolo, Frequency Shifter, Exciter, Harmonizer and the Clipper; reorderable chain.
- The FX page: live tiles in two rows, interactive graphs, per-effect preset menu, effect chains, Lock FX.
- More than 700 effect presets (at least 40 per effect) and 35 effect chains.
- Single-effect plugins "Fjord <Effect>" for VST3, Audio Unit, LV2 and standalone.
- The preset manager, parameter table machinery, DSP helpers, plugin helpers (peak meter, block splitter, transport, debug
  report).
- CMake helpers (`fjord_add_plugin`), build scripts, reusable CI and release workflows, a project template and the
  documentation of how fjord-sounds plugins are built.
