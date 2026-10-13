"""Extract the unchanged fighter queries used by the stock Bow callbacks.

The Bow test links all of itlinkbow.c. Its fighter queries are compiled directly
from their source; unrelated neutral-special fighter callbacks are not linked.
"""
import argparse
from pathlib import Path
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("source", type=Path)
parser.add_argument("output", type=Path)
args = parser.parse_args()
source = args.source.read_text()
unit = """#include <melee/ft/kinds/ftLink/ftlinkspecialn.h>
#include <melee/ft/kinds/ftLink/ftlink.h>
#include <melee/ft/kinds/ftLink/types.h>
#include <melee/ft/kinds/ftLink/forward.h>
#include <melee/ft/kinds/ftCommon/inlines.h>
#include <melee/ft/fighter.h>
"""
for name in ("ftLk_SpecialN_GetIndex", "ftLk_SpecialN_UnsetFv14",
             "ftLk_SpecialN_IsActiveAnd2071b6", "ftLk_Init_BoomerangExists"):
    if name == "ftLk_Init_BoomerangExists":
        source = args.source.with_name("ftlink.c").read_text()
    match = re.search(rf"(?m)^(?:ftLk_SpecialNIndex|void|bool) {name}\([^;]*?\)\s*\{{", source)
    if match is None:
        raise AssertionError(f"Missing stock function {name}")
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    unit += source[match.start():end] + "\n\n"
args.output.write_text(unit, encoding="utf-8", newline="\n")
