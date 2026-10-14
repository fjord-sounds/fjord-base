# Compatibility rules

Hosts store parameters, plugin codes and state; users store presets and projects. These rules keep all of that working.

## Parameters

- **IDs never change.** The id string is what hosts, projects and presets store.
- New parameters go to the **end** of a table (a plugin's table, or an effect's block only when no plugin depends on the
  position – plugins store by ID, so appending inside the shared FX block is allowed). Modulation destinations are
  listed from the table, so their position matters: append only.
- **Choice lists are append-only.** The stored value is the index.
- Ranges may **grow**, never shrink. Defaults of existing parameters do not change; new parameters need neutral
  defaults (the setting that leaves the sound as it was), because presets without them load the default.
- Presets and state store **plain values**, not normalised ones.
- Booleans are two-step choices (Off / On) in the host.
- Lock groups, tag lists and other lists referenced by index are append-only too.

## The shared FX block

`FJORD_FX_PARAMS` is shared by all plugins. A change there changes every plugin:
- new effects are appended at the end of the table and of `fx::Module`; the clipper stays last in the chain;
- note lists (`FJORD_FX_NOTES`) and mode lists are append-only;
- effect plugin codes (`Fsat` …) and ids (`saturation` …) never change.

## State properties

`stateVersion`, `presetName`, `presetCategory`, `macroNames`, `fxOrder`, `fxPresetName`, `fxLock`, `uiScale`,
`darkTheme` and each plugin's own properties are stored in the state. Never rename them. When a change needs old state
converted, raise `kStateVersion` and add a step to the plugin's state loading
(`fjord_base/presets/Migration.h` has tables for renamed ids with old defaults and value conversion; the preset manager's
`migrate` hook applies them to presets).

Projects and presets written before the first public release of a plugin do not need migration. From the first release on,
every incompatible change needs one.

## Presets and files

- Preset formats (`.<name>`, `.fjordmod`, `.fjordfx`) only grow: new optional elements and attributes.
- Unknown parameters in a preset are ignored; missing ones get defaults.
- User folders (`~/Music/Fjord/<Plugin>/Presets`, `FX Presets`, `FX Chains`) never move without a one-time copy
  (`Brand::legacySettingsFolders` does that for settings).

## The audio thread

Never allocate or lock; hosts may send blocks bigger than announced; every sample rate works. See
[code-style.md](code-style.md).

## Versions

fjord-base follows semantic versions; a change that breaks the API for plugins is a new minor version while it is
0.x, a major one after 1.0. Plugins pin a tag; update the pin only after building and testing the plugin against it.
