# Repository layout

## fjord-base

```
fjord-base/
  CMakeLists.txt          builds the interface library and, standing alone, the effect plugins
  VERSION                 version of fjord-base (the tag is v<VERSION>)
  cmake/
    FjordBase.cmake       fjord_setup_juce, fjord_add_base_library, fjord_add_plugin
    bootstrap.cmake       the snippet plugins copy into their CMakeLists.txt
  source/fjord_base/
    core/                 Dsp.h – smoothers, filters, delay lines, pitch shifter, helpers
    params/               Params.h/.cpp – parameter table machinery
    fx/                   the sixteen effects, chain, FX page, graphs, effect and chain presets
    ui/                   Brand, Theme, fonts, look and feel, controls, header, preset bar and browser, PluginView
    presets/              PresetManagerBase, Migration.h
    plugin/               PluginHelpers.h – peak meter, block splitter, transport, debug report
  effects/                the single-effect plugins ("Fjord Reverb" …) and FjordEffectsTest
  presets/fx/             factory presets per effect (<effect>.fjordmod)
  presets/chains/         factory effect chains (*.fjordfx)
  resources/fonts/        Manrope (SIL OFL) with its licence
  templates/plugin/       the skeleton for a new plugin (see new-plugin.md)
  tools/                  build-plugin.sh, build-effects.sh, install-all.sh, new-plugin.py,
                          release/bump_version.py, screenshots/shoot.sh
  docs/                   this documentation
  .github/workflows/      reusable workflows for plugins (plugin-ci, plugin-release, base-update) and base's own
                          ci, release, juce-update
  README.md  CHANGELOG.md  CONTRIBUTING.md  SECURITY.md  CODE_OF_CONDUCT.md  THIRD_PARTY_NOTICES.md  LICENSE
```

## A plugin repository

Every plugin has the same shape (the template generates it):

```
<plugin>/
  CMakeLists.txt          project(<Name> VERSION x.y.z), bootstrap snippet, one fjord_add_plugin(...) call
  build.sh                wrapper around fjord-base/tools/build-plugin.sh
  Source/
    Parameters.h/.cpp     the X-macro table of every parameter (+ the shared FX block)
    PluginProcessor.h/.cpp  audio path, state, programs, debug report, the Brand
    PluginEditor.h/.cpp   <Name>View (a PluginView) and <Name>Editor (a ScaledEditor)
    dsp/                  the sound engine
    ui/                   Theme.h (thin wrapper over base + the plugin's colour helpers), one header per page
    presets/              PresetManager (derived from PresetManagerBase)
    (more folders for what the plugin needs: seq/, state/, …)
  presets/Factory/<Category>/*.<ext>   factory presets, embedded into the plugin at build time
  resources/              icon.png and further resources
  tests/RenderTest.cpp    offline test and tool binary
  tools/screenshots/      shoot.sh (wrapper), shots.conf, hero.mjs, package.json
  docs/screenshots/       README images (WebP, light and dark)
  .github/                workflows (ci, release, base-update – small callers of fjord-base), issue and PR templates,
                          dependabot.yml
  README.md  CHANGELOG.md  CONTRIBUTING.md  SECURITY.md  CODE_OF_CONDUCT.md  THIRD_PARTY_NOTICES.md  LICENSE
  .gitignore
```

What does **not** belong in a plugin repository any more: copies of the controls, theme, header, preset bar or browser,
fonts, the effect code, per-plugin bump scripts, JUCE update workflows. All of that lives in fjord-base.

## .gitignore

```
build/                        build output
external/JUCE                 local JUCE checkout (fjord-base only)
node_modules/  __pycache__/   tool dependencies
.DS_Store  .idea/  .vscode/  *.swp
```
