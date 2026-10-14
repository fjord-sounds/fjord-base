# Design system

The fjord-sounds look is calm, with lots of air: hairlines instead of boxes, one accent colour, all-caps spaced labels,
Manrope everywhere, and a northern-lights glow behind the header. It works in a light theme (icy white) and a dark
theme (polar night); both must always look right.

The family look comes from **layout, typography and behaviour – not from fixed colours**. Every plugin chooses its
own colours.

## Colours belong to the plugin

A plugin defines `fjord::base::brand()` once (in `PluginProcessor.cpp`). It returns a `Brand` with:

| Field | Meaning |
|---|---|
| `name`, `logo`, `tagline` | product name, lower-case logo text, tagline for the About panel |
| `githubUrl`, `settingsFolder`, `userPresetFolder` | links and folders (`Fjord/<Plugin>`) |
| `light`, `dark` | the **complete palette** per theme: background, panel, line, text, textDim, accent, accentSoft, track, keyWhite, keyBlack |
| `gradientLight`, `gradientDark` | three-colour gradient for meters, banners and visuals (aurora, storm, veil …) |
| `glow`, `glowDark`, `glowLight` | the soft glow behind the header and its strength |
| `flash` | colour of the meter flash on every note or hit (transparent = none) |
| `categories`, `tags`, `categoryLabels` | preset browser content in display order |

`familyLight(accent)` and `familyDark(accent)` return the family palettes (icy white, polar night) for an accent colour.
They are **defaults to start from** – a plugin may change any single colour or replace the palette completely. Nothing
in fjord-base uses a fixed colour: components read `Theme::get().palette()`, `gradientColours(dark)`, `gradientAt` and
`drawGlow`, which come from the Brand. Plugin-specific colour helpers (Torden's storm, Huldra's veil, Fjord's aurora,
Hamar's brass and wood, layer, engine and category colours) live in the plugin's `ui/Theme.h`.

Current accents: Fjord aurora green (dark `0xff58d1a4`, light `0xff0e7566`), Torden thunderstorm amber
(`0xfff2b544` / `0xffa8650a`), Huldra rose with a lilac and ice-blue veil, Hamar ivory and brass (`0xffe6c88a` /
`0xff8a6526`, gradient ivory → brass → walnut). Each single-effect plugin has an accent of
its own (see `effects/source/EffectProcessor.cpp`).

## Typography

- **Manrope** (Regular, Medium, SemiBold), embedded unmodified; `font (height, weight)` and `logoFont`.
- Section titles and knob labels are **all caps with letter spacing** (`drawSpacedText`, spacing ≈ 0.16–0.25 em),
  9.5–11 px. Values and preset names are normal case.
- The logo is the plugin name in lower case plus a full stop in the accent colour.

## Layout

- Windows are drawn at a fixed size and scaled as a whole (75–150 %, remembered in `uiScale`). Instruments use roughly
  1100 × 740 (Fjord, Torden) up to 1200 × 800 (Huldra, Hamar); effect plugins 1040 × 600.
- Header 64 px, tab row 40 px, margin 16 px. Panels have a 6–8 px radius, a fine `line` outline and the `panel` fill.
- Knobs sit on a grid; selectors (segmented, dropdown, toggle) have a small all-caps label above and a fixed height.
- Less is more: no gradients on controls, no shadows, no icons where text does the job.

## The header and the logo dot

Every plugin has the same header: logo with a **dot that flashes with every note or hit** (brightness = velocity), the
level meter with an optional flash in `brand().flash`, the preset bar, undo/redo, GitHub, theme switch and the master
knob. A click on the logo opens the About panel; a double-click stops all sound. Tabs sit below; plugin specific
controls (Huldra's play-mode chips, Hamar's model menu and quality) go into the tab row.

## The FX page

The same page in every plugin (`fx::FxPage`): the **FX CHAIN** bar (preset chains, Lock FX) on top, two rows of eight
tiles (fifteen effects and the clipper, drag to reorder, every tile live), then the big graph of the selected effect on
the left and its controls, preset menu and on switch on the right. Do not build a separate FX page.

## Locks

Every plugin lets the user keep parts of a sound while browsing presets: the padlock in the preset bar opens the lock menu,
and the main panels carry a small padlock in their title row. Locked parts are shown in the accent colour. The list of
lockable parts is the plugin's own; the effect locks are shared ([presets.md](presets.md)).

## Motion and accessibility

- Animation is calm and meaningful (levels, signal, decay). Everything that moves checks `Theme::reduceMotion()`.
- Tooltips on header buttons and controls with non-obvious function.
- Contrast: text on the panel colour must stay readable in both themes; check screenshots of both.

## Checking a design change

Render the affected pages with `<Plugin>RenderTest --screenshot …` in `light` and `dark`, look at them, and refresh the
README images (`tools/screenshots/shoot.sh`). Screenshots never show personal data – the About page shows local system
information and is never used for README images.
