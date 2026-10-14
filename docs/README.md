# fjord-base documentation

fjord-base is the shared foundation of all fjord-sounds plugins: the user interface, the sixteen effects with their
presets, the preset system, the build helpers, the CI workflows and the project template. These pages describe how a
fjord-sounds plugin is built – what the code looks like, which files a repository has, how it is built and released,
and how it looks and behaves. Read them before you change a plugin or start a new one.

| Page | Contents |
|---|---|
| [getting-started.md](getting-started.md) | Clone the repositories, build a plugin, run the tests |
| [architecture.md](architecture.md) | How fjord-base and a plugin fit together, the audio path, the UI frame |
| [repo-layout.md](repo-layout.md) | Files and folders of fjord-base and of a plugin repository |
| [code-style.md](code-style.md) | Naming, formatting, comments, real-time rules |
| [build-and-release.md](build-and-release.md) | CMake, scripts, CI, versions, releases, JUCE updates |
| [design-system.md](design-system.md) | Look and feel: layout, type, colours per plugin, the logo dot, both themes |
| [ui-components.md](ui-components.md) | The shared UI classes and how to use them |
| [effects.md](effects.md) | The sixteen effects, every parameter, the chain, the single-effect plugins |
| [presets.md](presets.md) | Preset files, folders, categories, tags, loudness, writing presets |
| [testing.md](testing.md) | The RenderTest tool, what it checks, screenshots, other tools |
| [readme-guide.md](readme-guide.md) | The required structure of a plugin's README, changelog and repository files |
| [compatibility-rules.md](compatibility-rules.md) | What must never change, so old projects and presets keep working |
| [new-plugin.md](new-plugin.md) | Checklist for a new plugin, starting from the template |

fjord-base needs to be **cloned next to the plugin repositories** (`../fjord-base`). Without that checkout CMake
downloads the version pinned in the plugin; see [getting-started.md](getting-started.md).

## Principles

1. **One place for everything reusable.** If a second plugin would need the same code, it belongs in fjord-base.
   Plugins keep only what makes them themselves: their sound engine, their pages, their presets and their brand.
2. **Same experience everywhere.** Header, preset bar, browser, About panel, FX page, keyboard and theme switch behave
   the same in every plugin. A user who knows one plugin knows all of them.
3. **Every colour belongs to the plugin.** The base offers defaults; each plugin defines its full palette through its
   `Brand` (see [design-system.md](design-system.md)).
4. **Old projects keep working.** Parameter IDs never change, lists are append-only, presets store plain values
   ([compatibility-rules.md](compatibility-rules.md)).
5. **Everything is tested without a DAW.** Each plugin has a RenderTest tool that renders sounds, checks presets and
   takes the screenshots for the README ([testing.md](testing.md)).
6. **Open about AI.** The code, the interface, the tests and the presets are written by AI; every README says so.
