# README and repository files

Every plugin's README follows the same order, so visitors find the same things in the same place.

## README structure

1. **Product image** – the hero picture (`docs/screenshots/hero-dark.webp` / `hero-light.webp`, a `<picture>` that
   follows the reader's theme).
2. **`# Name`** and one paragraph: what it is, the formats (VST3, Audio Unit, LV2, standalone), the two or three things
   that make it special, how many factory presets, where the name comes from, that it belongs to the Fjord family.
   Say that every sound is synthesized (or sampled) as it is.
3. **Badges** (CI, licence) and the **"Made with AI"** note: the plugin code, the user interface, the tests and all
   factory presets were written by AI (Claude), directed by prompts; the code is open so everyone can read, test and
   judge it; bug reports and pull requests are welcome. Never claim the work is hand-made.
4. **Link bar** to the main sections.
5. **Lightweight** – size, self-contained (no samples, account or network), calm audio thread.
6. **Works in your DAW** – the format table, tempo sync, block sizes and sample rates, presets as programs, automation,
   plugin-specific hosting features.
7. **A tour** – one `###` per page: screenshot (light and dark via `<picture>`) and a short description. The FX page text
   is shared in spirit: fifteen tiles in two rows, drag to reorder, FX CHAIN, Lock FX.
8. **Sound architecture** – a table of the parts of the engine and the output path.
9. **Effects** – the sixteen effects of the family, 40+ presets each, 35 chains, also available as plugins; link to
   `fjord-base/docs/effects.md`.
10. **Presets** – counts, categories, user folders, loudness, where the files live.
11. **Building** – requirements, **clone fjord-base next to the repository**, `./build.sh` and the CMake commands.
12. **Tests** – the RenderTest commands, what is checked, other modes, how the README images are made.
13. **Licences** – AGPL-3.0, JUCE, Manrope; link to THIRD_PARTY_NOTICES.md.

Style: plain, factual and calm; sentences instead of marketing; numbers must match the code (preset counts, effect
counts) – check them when you change something. English only.

## Other files

| File | Notes |
|---|---|
| `CHANGELOG.md` | Keep a Changelog format, section **Unreleased** on top, one bullet per user-visible change; `bump_version.py` turns it into a release |
| `CONTRIBUTING.md` | build steps (including fjord-base), the compatibility rules for that plugin, preset rules, pull request checklist |
| `SECURITY.md` | private vulnerability reporting, supported versions |
| `CODE_OF_CONDUCT.md` | same text in every repository |
| `THIRD_PARTY_NOTICES.md` | fjord-base, JUCE and its SDKs, Manrope, any DSP library (e.g. Signalsmith) with licences |
| `.github/ISSUE_TEMPLATE/*`, `pull_request_template.md` | bug report asks for the debug info from the About panel; the PR checklist asks for ALL PASSED, light + dark screenshots, unchanged parameter IDs, README and changelog |
| `LICENSE` | AGPL-3.0 |

Public files describe the project as it is. They do not contain personal notes or testing diaries, and screenshots never
show personal data.

## When you change something

Update, in the same change: README (feature text, tour, counts), CHANGELOG ("Unreleased"), the screenshots in both themes
(`tools/screenshots/shoot.sh`) and these docs if a rule or behaviour changed. A task is not finished before the
documentation matches the code.
