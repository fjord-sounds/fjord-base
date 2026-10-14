# Presets

There are three kinds of presets: **sound presets** of a plugin, **effect presets** (one effect) and **effect chains**
(all effects). All are plain XML, all store plain parameter values.

## Sound presets

```xml
<TordenPreset name="Deep Stone" category="Techno" author="Fjord" tags="Deep,Sub" macros="Punch|Length|Drive|Tone" fxOrder="saturation,eq,…">
  <Param id="body_level" value="0.8"/>
  …
  <Envelopes/> <Pattern/>        <!-- plugin extras, written by writeExtras -->
</TordenPreset>
```

- Root tag `<Name>Preset`, extension `.<name>`; one file per preset in `presets/Factory/<Category>/`. They are
  embedded into the plugin at build time (`PRESETS "presets/Factory/*.<ext>"`).
- Parameters a preset does not mention get their **default**, so presets stay valid when parameters are added.
- Never in a preset: the master gain (the user's). The preset level is the preset's own trim.
- User presets: `~/Music/Fjord/<Plugin>/Presets/<Category>/*.<ext>`, found next to the factory presets in the browser,
  with tags and favourites.
- Plugin extras (envelopes, step patterns, motion paths …): `applyExtras(xml, isInit)` and `writeExtras(xml)` in the
  plugin's `PresetManager`. The base handles parameters, name, category, tags, author, macros, FX order, undo and the
  host's program list.

`PresetManagerBase::Config` is `{ extension, rootTag, hasPatterns, patternLabel, userOwnedParameters }`. Virtual hooks:
`migrate`, `shouldLoadParameter`, `applyExtras`, `writeExtras`, `shouldLoadMacros`, `shouldLoadEffects`,
`normaliseFxOrder`, `collectPresets`, and `saveUserPreset`.

### Locks

A locked part of the sound keeps its values when another sound preset is loaded – an arp pattern, one oscillator, the
envelopes, an effect chain you want to try on other sounds. Every plugin lists its lock groups once
(`fjord::base::locks::LockSet`, `presets/Locks.h`): a **property** in the state (`osc1Lock` – saved with the project, the
name never changes), a menu label and the **parameter id prefixes** the group protects (`"sub_|noise_"`).
`PresetManagerBase::setLocks(set)` makes the manager skip locked parameters. Data that is not a parameter (envelopes,
patterns, key edits, MSEGs …) is protected in `applyExtras` with `isLockedProperty("…")`. The macro names stay when the
`macroLock` property is set.

`addEffectGroups()` adds the same effect locks to every plugin: the whole chain (`fxLock`, also the **Lock FX** switch of
the FX page; it keeps the order too) and one lock per effect (`fxLock_<effect>`).

The user sees the locks in the **padlock** in the preset bar (a closed padlock in the accent colour while something is
locked; its menu ends with "Unlock all" and "Options …") and as small padlocks in the title row of panels
(`LockButton`). See [ui-components.md](ui-components.md).

### Categories, tags, loudness

- The Brand lists the categories and tags in display order. Tags are filters: each should be useful (not on almost
  nothing, not on almost everything).
- Effect presets must be **audible**: `FjordEffectsTest --strength` measures how far every preset moves the sound (the
  level of processed minus dry relative to a broadband stereo test phrase, in dB). Subtle presets are fine, but most
  presets of an effect should sit above about -12 dB; the median per effect is printed.
- Presets are **loudness matched** within a plugin (the same target RMS and peak headroom), so switching presets does
  not jump in level. Use `<Plugin>RenderTest --loudness <json>` to measure.
- One sound idea per preset. No generated variants ("X Soft", "X Bright").

## Effect presets (`.fjordmod`)

Presets of one effect, by short parameter key (`fx_reverb_size` is `size`):

```xml
<FjordFxPresets effect="reverb">
  <Preset name="Small Room" category="Room"><Param id="size" value="0.25"/> … </Preset>
</FjordFxPresets>
```

- Factory: `fjord-base/presets/fx/<effect>.fjordmod` – at least **40 per effect**, embedded in every plugin and in the
  effect plugins. Missing keys get defaults.
- User: `~/Music/Fjord/FX Presets/<effect>/<name>.fjordmod`, shared by every Fjord plugin.
- Loading a module preset sets the effect's parameters and switches the effect on (in an effect plugin the switch is
  the bypass and stays on).
- The preset menu of the effect editor and the preset bar of the effect plugins use them.

## Effect chains (`.fjordfx`)

```xml
<FjordFxPreset name="Big Hall" category="Space" fxOrder="saturation,eq,compressor,…">
  <Param id="fx_reverb_on" value="1"/> …
</FjordFxPreset>
```

- Factory: `fjord-base/presets/chains/*.fjordfx` (35, categories Clean, Space, Echo, Modulation, Character, Lo-Fi),
  user: `~/Music/Fjord/FX Chains/`.
- Loading sets every `fx_*` parameter (missing ones get defaults) and the order. `fxPresetName` in the state shows the
  chain's name in the FX CHAIN bar; empty means "From sound preset".

## Writing good presets

- Start from the sound's purpose (a pad for a soft verse, a kick for a house track), name it for that.
- Keep the values musical: ranges used well, no extreme corners unless that is the point.
- Add ±3 % seeded variation to continuous values if presets are produced in bulk, so near-identical sounds do not
  share exact numbers; never to choices, switches, tuning, patterns, delay times or the level stage.
- Check loudness, silence at the tail, and that the preset does not rely on parameters that do not exist.
- Presets are written by AI like the rest of the project; do not describe them as hand-made.
