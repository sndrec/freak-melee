"""Generate a real module and extract actual shared front-filter predicate slices.

The extracted slices retain both collision branches verbatim, but deliberately
do not claim the whole fighter/item collision traversal. Guard creation/reset
is a complete actual production body. No game is launched.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess


def braced(source, start):
    opening = source.index("{", start)
    end, depth = opening + 1, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def extract_source(root, output):
    path = root / "src/melee/ft/ftcoll.c"
    source = path.read_text(encoding="utf-8")
    slices = []
    for name, signature, needle in [
        ("actual_fighter_front_filter", "Fighter* this_fp, Fighter* victim_fp", "if (this_fp->x221B_b3)"),
        ("actual_item_front_filter", "Fighter* fp, Item* item", "if (fp->x221B_b3)"),
    ]:
        assert source.count(needle) == 1
        predicate = braced(source, source.index(needle))
        slices.append("static bool " + name + "(" + signature + ")\n{\n    bool var_r3 = true;\n" +
                      predicate + "\n    return var_r3;\n}\n")
    match = re.search(r"(?m)^void ftColl_8007B1B8\([^;{}]*\)\s*\{", source)
    assert match
    slices.append(braced(source, match.start()))
    (output / "script_guard_front_filter_original.inc").write_text(
        "\n\n".join(slices) + "\n", encoding="utf-8")
    (output / "script_guard_front_filter_source.json").write_text(json.dumps({
        "source": str(path), "sha256": hashlib.sha256(source.encode()).hexdigest(),
        "coverage": "Two actual front-filter predicate slices and complete shared guard-create/reset body",
        "excluded": "Full collision traversal, hitbox intersection, damage reaction and gameplay",
    }, indent=2) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--compiler-test", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(args.compiler_test.resolve()),
                    str((args.output / "script_guard_front_filter_generated.inc").resolve())], check=True)
    extract_source(args.root, args.output)


if __name__ == "__main__":
    main()
