#!/usr/bin/env python3
"""Sets the version of a fjord-sounds repository and prepares its changelog.

Usage (from the repository, e.g. torden/):  python3 ../fjord-base/tools/release/bump_version.py 0.2.0

The single source of truth is `project(<Name> VERSION x.y.z)` in the plugin's CMakeLists.txt (plugin version for
VST3, AU, LV2, the About panel and the test tool), or the VERSION file in fjord-base itself.

What it does: sets the version and turns "## [Unreleased]" in CHANGELOG.md into the new release section (with
today's date) with a fresh empty "Unreleased" above it. Then commit and tag:
    git commit -am "Release 0.2.0" && git tag -a v0.2.0 -m "<Name> 0.2.0" && git push --follow-tags
The release workflow builds the binaries when the tag arrives.
"""
import datetime
import re
import sys
from pathlib import Path

if len(sys.argv) not in (2, 3) or not re.fullmatch(r"\d+\.\d+\.\d+", sys.argv[1]):
    sys.exit(__doc__)
version = sys.argv[1]
root = Path(sys.argv[2] if len(sys.argv) == 3 else ".").resolve()

cmake = root / "CMakeLists.txt"
text = cmake.read_text() if cmake.exists() else ""
match = re.search(r"^project\((\w+) VERSION \d+\.\d+\.\d+", text, re.M)
if (root / "VERSION").exists() and (match is None or match.group(1) == "FjordBase"):
    name = "fjord-base"
    (root / "VERSION").write_text(version + "\n")
else:
    assert match, f"project(<Name> VERSION ...) not found in {cmake}"
    name = match.group(1)
    cmake.write_text(re.sub(r"^(project\(\w+ VERSION )\d+\.\d+\.\d+", rf"\g<1>{version}", text, count=1, flags=re.M))

log = root / "CHANGELOG.md"
text = log.read_text()
assert "## [Unreleased]" in text, "CHANGELOG.md has no [Unreleased] section"
assert f"## [{version}]" not in text, f"CHANGELOG.md already has {version}"
text = text.replace("## [Unreleased]", f"## [Unreleased]\n\n## [{version}] – {datetime.date.today().isoformat()}", 1)
log.write_text(text)
print(f"{name} {version} set. Review the diff, then commit and tag v{version}.")
