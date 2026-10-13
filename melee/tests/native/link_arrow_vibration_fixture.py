"""Extract unchanged stock vibration bodies for a focused native test unit.

The complete Arrow source also links unrelated fighter, stage and render
systems. This fixture compiles the selected bodies directly from that source,
with no rewritten decisions or substitute vibration implementation.
"""
import argparse
from pathlib import Path
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("source", type=Path)
parser.add_argument("output", type=Path)
args = parser.parse_args()
source = args.source.read_text()


def definition(name, array=False):
    pattern = (rf"(?m)^f32 {name}\[\] = \{{" if array else
               rf"(?m)^(?:static bool inline|s32|void|bool) {name}\([^;]*?\)\s*\{{")
    match = re.search(pattern, source)
    if not match:
        raise AssertionError(f"Missing source definition: {name}")
    start, end, depth = match.start(), match.end(), 1
    while depth:
        if end == len(source):
            raise AssertionError(f"Unterminated source definition: {name}")
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    if array:
        if source[end] != ";":
            raise AssertionError(f"Missing array terminator: {name}")
        end += 1
    return source[start:end]


includes = "\n".join(line for line in source.splitlines()
                     if line.startswith("#include") and '"itlinkarrow.h"' not in line)
unit = includes + '\n#include <melee/it/kinds/itlinkarrow.h>\n\n'
unit += definition("it_803F6A84", array=True) + "\n\n"
for name in ("itLinkArrow_802A81C4", "it_802A8330",
             "itlinkarrow_inline_bool", "itLinkarrow_UnkMotion4_Anim"):
    unit += definition(name) + "\n\n"
args.output.write_text(unit)
