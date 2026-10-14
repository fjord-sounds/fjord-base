# Code style

fjord-sounds code is C++20 on JUCE 8. New code should read like the code around it.

## Language and text

- Everything in a repository is **English**: code, comments, UI text, scripts, documentation, commit messages.
- UI labels are short and sentence-like in the code ("Pre-delay"); the UI draws section titles and knob labels in
  capitals with spacing. Do not capitalise in the string.

## Naming and formatting

- Types `PascalCase`, functions and variables `camelCase`, constants `kPascalCase`, parameter ids `snake_case`
  strings, parameter enum names `PascalCase` with a short prefix per block (`FxVerbMix`).
- Four spaces, no tabs. JUCE style: a space before the opening parenthesis of calls (`foo (x)`), braces on their own
  line for types and functions, lambdas inline.
- Header-only where it makes sense (UI components, effects); `.cpp` only for what needs it. Use `inline`.
- Namespaces: `fjord::base::…` in fjord-base, the plugin's own name in the plugin. No `using namespace` in headers
  outside a plugin's `ui/Theme.h` wrapper.

## Comments

- Explain **why** and the shape of the idea, not what the line does. A short header comment at the top of a file or
  class says what it is.
- Keep comments in step with the code. No commented-out code, no TODO without an owner.
- Tables and formats that other code or users depend on (preset XML, state properties) are described where they are
  read and written.

## Real-time rules

- The audio thread never allocates, locks, logs or touches files. Allocate in `prepare` / constructors; build static
  tables before the first block (call the accessor once in the constructor).
- Parameters are read once per block (`Reader::read`) and smoothed where they could click.
- Hosts may send blocks larger than announced: process through `BlockSplitter`.
- Handle every sample rate (44.1–192 kHz) and mono / stereo input where the plugin allows it.
- Denormals: `juce::ScopedNoDenormals` at the top of `processBlock`.
- Anything shared between the UI and audio thread is atomic or a lock-free store.

## UI code

- Components paint with `Theme::get().palette()` and the brand gradient – never fixed colours.
- Fonts come from `font()`; text with spacing from `drawSpacedText`.
- Animations check `Theme::get().reduceMotion()`.
- Pages implement `tick()` for animation; the view calls it from one timer.
- Pages are laid out in `resized()` from the bounds they get; the plugin's `layoutContent` decides the bounds.

## Parameters

See [compatibility-rules.md](compatibility-rules.md). In short: one table per plugin, IDs never change, new ones go at
the end, choice lists are append-only, defaults are neutral.

## Tests and docs with every change

Build, run the RenderTest (`ALL PASSED`), look at screenshots of the affected pages in **both themes**, update the
README, the CHANGELOG ("Unreleased") and these docs where behaviour changed.
