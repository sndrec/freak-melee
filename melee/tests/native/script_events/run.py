"""Build real event signature/default units and archive a source-backed census."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
NO_WINDOW = getattr(subprocess, "CREATE_NO_WINDOW", 0)
UNITS = ["native_script_source.c", "native_script_expression.c",
         "native_script_statement.c", "native_script_program.c",
         "native_script_bytecode.c", "native_script_value.c", "native_script_vm.c",
         "native_script_events.c"]
EVIDENCE = ["tools/native_fighter_script.c", "tools/native_fighter_script_code.c",
            "tools/native_fighter_script_primitives.c", "tools/native_fighter_item_script.c",
            "tools/native_fighter_item_statements.c", "tools/native_fighter_script_model_effect.c",
            "src/melee/ft/native_fighter_runtime.h", "src/melee/ft/types.h",
            "src/melee/ft/forward.h", "src/melee/it/types.h", "src/melee/it/kinds/types.h",
            "src/melee/it/itgroundcoll.h", "src/melee/ef/forward.h",
            "src/sysdolphin/baselib/forward.h"]


def source_checks():
    """Check actual slot/type provenance, without claiming callback body parity."""
    catalog = (ROOT/"src/melee/ft/native_script_events.c").read_text()
    # Named engine slots must exist in the actual declaration headers. Stage
    # response hooks are function arguments, not fabricated Item struct fields.
    headers = {"NativeFighterDefinition": "src/melee/ft/native_fighter_runtime.h",
               "NativeFighterArticleLogic": "src/melee/ft/native_fighter_runtime.h",
               "NativeFighterProjectile": "src/melee/ft/native_fighter_runtime.h",
               "NativeFighterArticleSignal": "src/melee/ft/native_fighter_runtime.h",
               "NativeFighterModelEffect": "src/melee/ft/native_fighter_runtime.h",
               "NativeItemDefinition": "src/melee/ft/native_fighter_runtime.h",
               "Fighter": "src/melee/ft/types.h", "MotionState": "src/melee/ft/types.h",
               "Item": "src/melee/it/types.h", "ItemLogicTable": "src/melee/it/kinds/types.h",
               "ItemStateTable": "src/melee/it/kinds/types.h"}
    checked = 0
    for slot in re.findall(r'\.native_slot="([^"]+)"', catalog):
        if "callback argument" in slot:
            for function in re.findall(r"it_[0-9A-Fa-f]+", slot):
                assert function in (ROOT/"src/melee/it/itgroundcoll.h").read_text(), function
        else:
            owner, member = slot.split(".")
            assert re.search(r"\b" + re.escape(member) + r"\b", (ROOT/headers[owner]).read_text()), slot
        checked += 1
    assert checked == 188
    items = (ROOT/"src/melee/it/kinds/types.h").read_text()
    assert re.search(r"HSD_GObjPredicate\s+animated", items)
    assert re.search(r"HSD_GObjEvent\s+physics_updated", items)
    assert re.search(r"HSD_GObjPredicate\s+collided", items)
    fighter = (ROOT/"src/melee/ft/types.h").read_text()
    assert all(re.search(r"HSD_GObjEvent\s+" + phase, fighter)
               for phase in ("anim_cb", "input_cb", "phys_cb", "coll_cb"))
    return checked


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--compiler", default="clang")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    executable = args.output/"test.exe"
    command = [args.compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-O2", "-DMELEE_NATIVE=1", "-D_CRT_SECURE_NO_WARNINGS", "-I", str(ROOT/"extern/dolphin/include"), "-I", str(ROOT/"src"), str(HERE/"test.c")]
    command += [str(ROOT/"src/melee/ft"/unit) for unit in UNITS]
    command += ["-o", str(executable)]
    build = subprocess.run(command, capture_output=True, text=True, creationflags=NO_WINDOW)
    (args.output/"build.log").write_text(build.stdout+build.stderr)
    build.check_returncode()
    unit = subprocess.run([str(executable), "unit", str(HERE/"probe.fms")], capture_output=True, text=True, creationflags=NO_WINDOW)
    (args.output/"unit.log").write_text(unit.stdout+unit.stderr)
    unit.check_returncode()
    receipt = json.loads(unit.stdout)
    checked_slots = source_checks()
    sources = sorted((ROOT/"examples").glob("*_full.fms")) + sorted((ROOT/"examples/rook_workshop").rglob("*.fms"))
    census = subprocess.run([str(executable), "census"]+list(map(str, sources)), capture_output=True, text=True, creationflags=NO_WINDOW)
    census.check_returncode()
    manifest = json.loads(census.stdout)
    resolved = sum(item["resolved"] for item in manifest)
    unresolved = sum(item["unresolved"] for item in manifest)
    gaps = [(Path(item["path"]).name, event["name"]) for item in manifest
            for event in item["callbacks"] if not event["resolved"]]
    assert len(manifest) == 16 and resolved == 1861 and unresolved == 2
    assert set(gaps) == {("link_full.fms", "trail"), ("young_link_full.fms", "trail")}
    receipt.update({"scope": "event signatures, payload types and explicit audited callback defaults",
                    "files": len(manifest), "resolved_signatures": resolved, "unresolved_signatures": unresolved,
                    "native_slots_source_checked": checked_slots, "whole_program_acceptance": False,
                    "engine_scheduler_migration": False, "compiler": command})
    paths = sources + [HERE/"test.c", HERE/"probe.fms"] + [ROOT/path for path in EVIDENCE]
    paths += [ROOT/"src/melee/ft"/unit for unit in UNITS]
    paths += list((ROOT/"src/melee/ft").glob("native_script_*.h"))
    receipt["source_sha256"] = {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    (args.output/"manifest.json").write_text(json.dumps(manifest, indent=2)+"\n")
    (args.output/"receipt.json").write_text(json.dumps(receipt, indent=2)+"\n")
    print(json.dumps({key: value for key, value in receipt.items() if key not in ("compiler", "source_sha256")}, indent=2))


if __name__ == "__main__":
    main()
