"""Extract actual catalog, deferred scheduling and update bodies for source tests."""
import argparse
from pathlib import Path
import re

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("source_root", type=Path)
p.add_argument("output", type=Path)
a = p.parse_args()
text = "// Actual production bodies; allocation and stage boundaries are test stubs.\n"
for path, names in (
    ("melee/ef/efsync.c", ("efSync_ModelRecipe",)),
    ("melee/ef/efasync.c", ("efAsync_QueueProcessDeferred", "efAsync_QueueFlush")),
    ("melee/ef/eflib.c", ("efLib_Update", "efLib_Destroy", "efLib_RemoveLast")),
):
    source = (a.source_root / path).read_text()
    for name in names:
        match = re.search(r"(?m)^[^\n;{}]*\b" + name + r"\([^;]*?\)\s*\{", source)
        if match is None:
            raise ValueError(f"Missing production function {path}:{name}")
        end, depth = match.end(), 1
        while depth:
            depth += (source[end] == "{") - (source[end] == "}")
            end += 1
        text += source[match.start():end] + "\n"
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_text(text, newline="\n")
