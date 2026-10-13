"""Extract stock callback tables/events and classify current Host function slots."""
import argparse
import hashlib
from pathlib import Path
import re


def body(text, name, return_type="void"):
    match = re.search(rf"(?m)^(?:static )?{return_type} {name}\([^;]*?\)\s*\{{", text)
    if not match:
        raise ValueError(f"Missing source body: {name}")
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[match.start():end]


def extract(source_root, output):
    unit = "// Actual production tables/bodies; unused motion callbacks trap.\n"
    callbacks = set()
    tables = []
    for family, prefix in (("ftMars", "ftMs"), ("ftEmblem", "ftFe")):
        path = source_root / f"melee/ft/kinds/{family}/{family.lower()}.c"
        raw = path.read_bytes()
        text = raw.decode("utf-8")
        unit += f"// {path.name} SHA256 {hashlib.sha256(raw).hexdigest()}\n"
        match = re.search(rf"MotionState {prefix}_Init_MotionStateTable\[.*?\n\}};", text, re.S)
        if not match:
            raise ValueError("Missing original motion table")
        tables.append(match[0])
        callbacks.update(re.findall(r"\b(ft\w+_(?:Anim|IASA|Phys|Coll)|ftCamera_UpdateCameraBox)\b", match[0]))
        for event in ("OnDeath", "OnItemPickup", "OnItemDrop", "OnItemVisible",
                      "OnItemInvisible", "OnKnockbackEnter", "OnKnockbackExit"):
            unit += body(text, f"{prefix}_Init_{event}") + "\n"
        if family == "ftMars":
            unit += body(text, "ftMs_SpecialN_80136730") + "\n"
    callbacks.update(("ftMs_SpecialN_Enter", "ftMs_SpecialAirN_Enter", "ftMs_SpecialS_Enter",
        "ftMs_SpecialAirS_Enter", "ftMs_SpecialHi_Enter", "ftMs_SpecialAirHi_Enter",
        "ftMs_SpecialLw_Enter", "ftMs_SpecialAirLw_Enter"))
    for callback in sorted(callbacks):
        unit += f"void {callback}(HSD_GObj* object) {{ (void)object; unexpected_host_call(); }}\n"
    unit += "\n".join(tables) + "\n"
    host = (source_root / "melee/ft/native_fighter_module.h").read_text(encoding="utf-8")
    host = host.split("typedef struct NativeFighterHost {", 1)[1].split("} NativeFighterHost;", 1)[0]
    host = re.sub(r"//[^\n]*", "", host)
    slots = re.findall(r"\(\*(\w+)\)\s*\(", host)
    slots += re.findall(r"\bHSD_GObj(?:Event|Predicate)\s+(\w+)\s*;", host)
    remainder = re.sub(r"[^;]*\(\*\w+\)\s*\([^;]*?;", "", host)
    remainder = re.sub(r"\bHSD_GObj(?:Event|Predicate)\s+\w+\s*;", "", remainder)
    if len(slots) != len(set(slots)) or re.sub(r"\s+", "", remainder) != "constMotionState*common_motion_states;":
        raise ValueError("Unclassified/duplicate Host member")
    unit += "static void closure_host_traps(NativeFighterHost* host) {\n"
    unit += "".join(f"    host->{slot} = (__typeof__(host->{slot}))unexpected_host_call;\n" for slot in slots)
    unit += "}\n"
    output.write_text(unit, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source_root", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    extract(args.source_root, args.output)
