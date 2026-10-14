# Creating a new plugin

```bash
python3 fjord-base/tools/new-plugin.py <Name> <CODE> [--tagline "KICK DESIGNER"] \
        [--accent-light 0xffa8650a] [--accent-dark 0xfff2b544] [--dest <folder>]
```

`<Name>` is the product name (letters and digits, capitalised), `<CODE>` a four-character plugin code that is unique among
all Fjord plugins and effect plugins (the script rejects the reserved ones) and that never changes. The script creates
`<dest>/<name>/` (default: next to fjord-base) from `templates/plugin`: CMake, parameter table with the shared FX block, a
small polyphonic synth as a starting point, processor, editor with a Sound page and the FX page, a preset, RenderTest,
README, changelog, repository files and workflows. It builds and passes its test as generated.

## Checklist

1. **Generate and build**: `cd <name> && git init && ./build.sh`, run the RenderTest (`ALL PASSED`).
2. **Brand** (`PluginProcessor.cpp`): name, logo, tagline, accents, gradient, glow, flash, categories, tags – every
   colour is the plugin's own ([design-system.md](design-system.md)).
3. **Parameters** (`Parameters.h`): replace the example parameters with the plugin's; keep `master_gain`,
   `preset_level` and the `FJORD_FX_PARAMS` block with its `static_assert`. IDs are final once released.
4. **Engine** (`Source/dsp/`): replace the example synth. Respect the real-time rules ([code-style.md](code-style.md)).
   Decide where the preset level sits and give the chain the note or hit positions (`Context::triggers`).
5. **Pages** (`Source/ui/`, `PluginEditor.cpp`): build the pages from the shared controls
   ([ui-components.md](ui-components.md)); keep the header, tabs, FX page and keyboard behaviour as they are. Add the
   tab names to `ViewSetup::tabs` and pass `noteCounter` / `lastVelocity` for the logo dot and meter flash.
6. **Locks** (`Source/presets/Locks.h`): list what the user can keep when switching presets (every plugin has the
   padlock): one group per part with its parameter prefixes, `addEffectGroups()` for the shared effects, a `MenuLayout`,
   `setLocks` in the PresetManager, `LockMenu::attach` in the editor, small `LockButton`s in the panels; protect extras in
   `applyExtras` with `isLockedProperty`. The template shows it.
7. **PresetManager**: derive from `PresetManagerBase`; add `applyExtras` / `writeExtras` for extra data; set the
   extension and root tag. Write factory presets ([presets.md](presets.md)), measure loudness (`--loudness`).
8. **RenderTest**: add tests for the engine's special cases; make `--screenshot` cover every page.
9. **Screenshots and README**: fill `shots.conf`, add a `hero.mjs` (copy one from another plugin and adapt the artwork
   and key facts), run `tools/screenshots/shoot.sh`, fill the README following [readme-guide.md](readme-guide.md).
10. **Icon**: `resources/icon.png` (the RenderTest `--icon` mode draws the family style).
11. **CI**: the workflow files call fjord-base's reusable workflows – set `target`. Make sure the fjord-base tag in
    `FJORD_BASE_TAG` exists.
12. **auval and pluginval** pass; test in a DAW.
13. **Check the names and codes once more** (product name, bundle id, plugin code, VST3 categories) – hosts store them.

## Rules of thumb

- If the code could be useful to a second plugin, put it in fjord-base, not in the plugin.
- Do not copy controls, theme, header, fonts or effects into the plugin.
- Do not give the plugin its own FX page, its own preset browser or its own About panel.
- Keep the plugin's UI recognisable as part of the family: same frame, same typography, same behaviour, its own accent.
