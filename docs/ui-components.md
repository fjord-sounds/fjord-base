# UI components

Everything below lives in `fjord_base/ui/` (namespace `fjord::base::ui`) unless noted. Plugins pull it in with
`using namespace fjord::base::ui;` in their `ui/Theme.h`.

## Frame

| Class | Purpose |
|---|---|
| `PluginView` | The window frame: header, tabs, pages, preset bar, browser, save dialog, About. Plugin derives from it, passes a `ViewSetup` and implements `layoutContent(area)`; optional `tickContent`, `layoutTabRow`, `pageChanged`, `browserVisibilityChanged`. `setPages({...})`, `showPage(i)`, `showBrowser`, `showAbout`, `showSaveDialog`, `tickAnimations` (for screenshots). |
| `ViewSetup` | `state`, `presets`, `undo`, `masterParamId`, `masterLabel`, `tabs`, `tabWidth`, `peaks`, `noteCounter`, `lastVelocity`, `debugReport`, `panic`, `rememberPage`, `meterWidth`. |
| `ScaledEditor<View, Processor>` | The `AudioProcessorEditor`: scales the fixed-size view 75–150 %, keeps the aspect ratio, stores `uiScale`. |
| `Page` | Base class of pages (`tick()` for animation). |
| `PresetBar` | Previous/next, name, ··· menu (options) or padlock menu (locks), save. `MenuStyle::Dots` or `Padlock`; `addOptions`, `onMenu`, `watch()`. |
| `PresetBrowser`, `SaveDialog`, `TagFilterBar`, `PresetGrid` | The preset browser with categories, tags, favourites and search, and the save dialog; content comes from the Brand and the preset manager. |
| `LockMenu::attach(bar, apvts, lockSet, layout)` | The padlock menu in the preset bar: closed padlock while something is locked, tooltip with the locked parts, the menu described by a `MenuLayout`, "Unlock all" and "Options …". |
| `MenuLayout` | Describes the lock menu: `header`, `item(group)`, `separator`, `submenu(name, groups, "All …")`, `all(label, groups)` and `effects(lockSet)` for the shared effect locks. |
| `LevelMeter`, `AboutPanel`, `CopyButton` (Header.h) | Meter with flash, About panel (logo, tagline, version, build, copy debug info), copy button. |

## Controls (Controls.h)

| Class | Purpose |
|---|---|
| `Knob (state, paramId, label)` | Rotary knob attached to a parameter: drag, wheel, double-click resets, text entry, tooltip with the formatted value. |
| `Segmented (state, paramId, glyphFn?, labels?)` | Row of choices for a choice parameter; optional glyphs (`waveGlyph`, `lfoGlyph`). |
| `Toggle`, `Choice` | Switch and dropdown for bool / choice parameters. |
| `Section (title)` | Panel with an all-caps title; `content()` is the area below the title (relative to the section), `placeTitleLock` puts a lock at the title's right. |
| `Labelled`, `labelled(c, title)` | A small label above a selector. |
| `ControlRow`, `layoutGrid`, `layoutRow` | Layout helpers: relative widths, rows of knobs and selectors. |
| `LockButton(state, lockSet, group, showLabel)` | A small padlock switch for one lock group, usually in a panel's title row (`Section::placeTitleLock`). |
| `PillButton` | Small pill-shaped button. |
| `drawPanel`, `drawPadlock`, `strokeGlow`, `starPath`, `freqToX`/`xToFreq`, `styleTextEditor` | Drawing and layout helpers. |
| `Keyboard` (Keyboard.h) | The on-screen keyboard, styled by the palette. |
| Icons.h | `drawGithub`, `undoArrow`, `drawThemeSwitch`, `drawChevron`, `drawSave`. |

## Window size

The window scales as a whole between 75 and 150 % (`ScaledEditor`): drag the corner, or choose **Window size** in the options
menu of the preset bar (75, 100, 125, 150 %) – useful in hosts that hide the resize handle. The size is remembered per
project (`uiScale`) and globally.

## Theme and fonts

`Theme::get()` is the singleton: `palette()`, `isDark()`/`setDark`, `scale`, flags (`reduceMotion`), persistence
(`setPersistent(false)` in tests so they never write settings). `gradientColours(dark)`, `gradientAt(dark, x)` and
`drawGlow` come from the Brand. `font(height, weight)`, `logoFont`, `drawSpacedText` are in Fonts.h.

## FX page (fx/FxPage.h)

`fx::FxPage` takes a `Host` struct: the state, the undo manager, the `Meters`, getter and setter for the order and
optional `ChainPresets*` (enables the FX CHAIN bar and Lock FX). `selectEffect(i)` for screenshots. The effect plugins
use its parts directly (`GraphPanel`, `ModuleEditor`).

## Writing a page

```cpp
class SoundPage : public Page
{
public:
    explicit SoundPage (juce::AudioProcessorValueTreeState& state)
        : cutoff (state, "filter_cutoff", "Cutoff") { addAndMakeVisible (filter); addAndMakeVisible (cutoff); }
    void resized() override { /* place children; Section::content() is section-relative */ }
private:
    Section filter { "Filter" };
    Knob cutoff;
};
```

Add the page to `setPages({...})` in the view's constructor and a tab name to `ViewSetup::tabs`. The template contains a
working example.
