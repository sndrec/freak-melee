"""Extract unchanged stock/HSD bodies and trap every unused current Host slot."""
import argparse
import hashlib
from pathlib import Path
import re

p = argparse.ArgumentParser(description=__doc__)
p.add_argument("source_root", type=Path)
p.add_argument("output", type=Path)
a = p.parse_args()

def function(source, name):
    match = re.search(rf"(?m)^void {name}\([^;]*?\)\s*\{{", source)
    if not match:
        raise ValueError(f"Missing production body: {name}")
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]

unit = "// Extracted production bodies; no replacement algorithm.\n"
for path, names in (
    ("melee/ef/eflib.c", ("efLib_Cb_ftMr_SpecialLw", "efLib_Cb_ftLg_SpecialLw")),
    ("sysdolphin/baselib/jobj.c", ("HSD_JObjSetMtxDirtySub", "HSD_JObjSetFlags",
        "HSD_JObjClearFlags", "HSD_JObjSetFlagsAll", "HSD_JObjClearFlagsAll")),
):
    raw = (a.source_root / path).read_bytes()
    unit += f"// {path} SHA256 {hashlib.sha256(raw).hexdigest()}\n"
    for name in names:
        unit += function(raw.decode("utf-8"), name) + "\n"

host = (a.source_root / "melee/ft/native_fighter_module.h").read_text(encoding="utf-8")
host = host.split("typedef struct NativeFighterHost {", 1)[1].split("} NativeFighterHost;", 1)[0]
host = re.sub(r"//[^\n]*", "", host)
slots = re.findall(r"\(\*(\w+)\)\s*\(", host)
slots += re.findall(r"\bHSD_GObj(?:Event|Predicate)\s+(\w+)\s*;", host)
if len(slots) != len(set(slots)):
    raise ValueError("Duplicate Host field extraction")
# A new data member must be classified rather than treated as a function pointer.
remainder = re.sub(r"[^;]*\(\*\w+\)\s*\([^;]*?;", "", host)
remainder = re.sub(r"\bHSD_GObj(?:Event|Predicate)\s+\w+\s*;", "", remainder)
if re.sub(r"\s+", "", remainder) != "constMotionState*common_motion_states;":
    raise ValueError(f"Unclassified Host fields: {remainder.strip()}")
unit += "\nstatic void stock_fixture_host_traps(NativeFighterHost* host) {\n"
for field in slots:
    unit += f"    host->{field} = (__typeof__(host->{field})) unexpected_host_call;\n"
unit += "}\n"
a.output.write_text(unit, encoding="utf-8", newline="\n")
