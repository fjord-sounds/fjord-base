# Architecture

## Two layers

```
fjord-base                         plugin (e.g. torden)
├─ core/    DSP helpers            ├─ Parameters      – the plugin's own parameters + the shared FX block
├─ params/  parameter tables       ├─ PluginProcessor – audio path, state, programs, Brand
├─ fx/      16 effects, chain,     ├─ dsp/            – the sound engine
│           FX page, presets       ├─ ui/             – the plugin's pages
├─ ui/      frame, controls, theme ├─ presets/        – PresetManager (extras)
├─ presets/ PresetManagerBase      ├─ presets/Factory – factory presets
├─ plugin/  helpers                └─ tests/          – RenderTest
└─ effects/ single-effect plugins
```

fjord-base is an **INTERFACE library** (`fjord_base`): its `.cpp` files compile inside every plugin target, so JUCE
modules are built once per target. Binary data (Manrope, effect presets and chains) is a separate library
(`FjordBaseData`). `fjord_add_plugin()` in `cmake/FjordBase.cmake` wires everything: formats, plugin codes, binary data,
warnings, the test target.

Namespaces: base code is `fjord::base::…` (`fjord::base::fx`, `…::ui`, `…::params`, `…::presets`, `…::plugin`,
`…::dsp`); each plugin keeps its own (`torden`, `huldra`, `fjord`).

## Parameters

Every plugin lists its parameters once, in an X-macro table in `Source/Parameters.h` (id, name, kind, range, default,
curve, format, choices). The table produces the enum, the specs, the host parameters (`addToLayout`) and the plain
values the audio thread reads once per block (`ValuesT`, `ReaderT`). The effects are one block of that table:
`FJORD_FX_PARAMS(X)` – the plugin pastes it in, `fxValues(values)` gives the chain a view of it.

## Audio path

```
MIDI / UI / sequencer → sound engine → preset level → effect chain (user order) → clipper → master → meter
```

`BlockSplitter` splits host blocks bigger than announced; `Transport::read` gives tempo and position; `PeakMeter`
feeds the header meter. The chain gets a `Context` with the note or hit positions of the block. Instruments that apply
a per-preset level do it **before** the effects or – like Fjord, whose arp and pump work before the chain – directly
before the master, so that the loudness of a preset is the same as the preset author heard it.

The master gain belongs to the user: presets never load it (`userOwnedParameters`), the preset level is the preset's
own trim.

## State and presets

The plugin state is the APVTS state plus properties outside the parameters (`stateVersion`, `presetName`,
`presetCategory`, `macroNames`, `fxOrder`, `fxPresetName`, `fxLock`, `uiScale`, `darkTheme`, plugin specific ones).
`PresetManagerBase` reads and writes preset XML, handles factory and user presets, categories, tags, favourites,
macros, Lock FX and undo. A plugin derives from it and adds only its extras (`applyExtras`, `writeExtras`, …).
Details in [presets.md](presets.md).

## The UI frame

`PluginView` is the frame of every plugin window, drawn at a fixed size and scaled as a whole by `ScaledEditor`
(75–150 %):

```
logo. ▁▂▃▅ meter      ‹  CATEGORY · Preset name  ›  ··· ⤓        ↶ ↷  github  ☀  (master)
TAB  TAB  TAB  TAB                                          (plugin extras in the tab row)
──────────────────────────────────────────────────────────────────────────────────────────
pages …                                                         (plugin content)
footer / keyboard …
```

The plugin supplies a `ViewSetup` (state, preset manager, undo manager, tab names, meter and note callbacks, debug
report, panic) and implements `layoutContent`. Pages derive from `ui::Page`. The FX page is `fx::FxPage` and is used as
is. See [ui-components.md](ui-components.md).

## Effects as plugins

`effects/` contains one generic processor and editor. The same source builds sixteen plugins; the macro
`FJORD_EFFECT_MODULE` picks the effect. Each effect gets its own `Brand` (accent colour, name, logo) so that the
frame, meter and About panel work unchanged. See [effects.md](effects.md).
