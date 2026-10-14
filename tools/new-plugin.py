#!/usr/bin/env python3
"""Creates a new fjord-sounds plugin repository from templates/plugin.

Usage (from anywhere):
  python3 fjord-base/tools/new-plugin.py <Name> <CODE> [--tagline "KICK DESIGNER"] [--accent-light 0xffa8650a]
                                         [--accent-dark 0xfff2b544] [--dest <folder>]

  Name      product name, e.g. Skerry (the repository is the lower-case name, next to fjord-base)
  CODE      four-character plugin code, unique among all Fjord plugins, e.g. Skry (stored by hosts, never change it)

Creates <dest>/<name>/ (default: next to fjord-base) with CMake, parameters, processor, editor, one page, the FX page,
a preset, a RenderTest, README, CI workflows and the repository files. Then:
  cd <name> && git init && ./build.sh
"""
import argparse
import re
import shutil
import sys
from pathlib import Path

BASE = Path(__file__).resolve().parents[1]
TEMPLATE = BASE / "templates" / "plugin"
RESERVED_CODES = {"Trdn", "Hldr", "Hmar", "Fjrd", "Fsat", "Flfi", "Ftrs", "Fcmp", "Fsqh", "Fequ", "Fflt", "Fmot", "Fdly", "Frvb",
                  "Fwdt", "Ftrm", "Fshf", "Fexc", "Fhrm", "Fclp"}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("name")
    ap.add_argument("code")
    ap.add_argument("--tagline", default="INSTRUMENT")
    ap.add_argument("--accent-light", default="0xff0e7566")
    ap.add_argument("--accent-dark", default="0xff58d1a4")
    ap.add_argument("--dest", default=str(BASE.parent))
    a = ap.parse_args()

    if not re.fullmatch(r"[A-Z][A-Za-z0-9]*", a.name):
        sys.exit("Name: letters and digits, starting with a capital (e.g. Skerry)")
    if not re.fullmatch(r"[A-Za-z0-9]{4}", a.code):
        sys.exit("CODE: exactly four letters or digits")
    if a.code in RESERVED_CODES:
        sys.exit(f"CODE {a.code} is already used by another Fjord plugin")
    out = Path(a.dest) / a.name.lower()
    if out.exists():
        sys.exit(f"{out} already exists")

    version = (BASE / "VERSION").read_text().strip()
    subst = {"@Name@": a.name, "@name@": a.name.lower(), "@NAME@": a.name.upper(), "@CODE@": a.code,
             "@TAGLINE@": a.tagline, "@TAGLINE_LOWER@": a.tagline.lower(), "@ACCENT_LIGHT@": a.accent_light,
             "@ACCENT_DARK@": a.accent_dark, "@BASE_TAG@": "v" + version}

    def convert(text):
        for k, v in subst.items():
            text = text.replace(k, v)
        return text

    for src in sorted(TEMPLATE.rglob("*")):
        rel = src.relative_to(TEMPLATE)
        if src.name == ".DS_Store" or any(p in ("node_modules",) for p in rel.parts):
            continue
        target = out / convert(str(rel))
        if target.name == "gitignore":
            target = target.with_name(".gitignore")
        if src.is_dir():
            target.mkdir(parents=True, exist_ok=True)
            continue
        target.parent.mkdir(parents=True, exist_ok=True)
        try:
            target.write_text(convert(src.read_text()))   # text files get the placeholders replaced
        except UnicodeDecodeError:
            shutil.copy2(src, target)                      # binary files (images, fonts) are copied
        if src.stat().st_mode & 0o111:
            target.chmod(target.stat().st_mode | 0o111)
    print(f"Created {out}\nNext: cd {out} && git init && ./build.sh   (tests: cmake --build build --target {a.name}RenderTest)")
    print("Then: write the sound engine, the pages, the presets, the README, and add it to your workspace notes.")


if __name__ == "__main__":
    main()
