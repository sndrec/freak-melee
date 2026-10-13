"""Bounded console batch: actual Marth/Roy templates, original C, owned rig maps.

Only fresh output is written. Shipped independent templates are required unless
--candidate explicitly requests a fixture-only independence declaration.
No project registration, gameplay, global build or input mutation occurs.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys

from marth_roy_closure_fixture import extract

ROOT = Path(__file__).resolve().parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rigs(dat):
    raw = dat.read_bytes()
    size, relocations, public, external = struct.unpack_from(">4I", raw, 4)
    table = 32 + size + 4 * relocations
    strings = table + 8 * (public + external)
    roots = {}
    for i in range(public):
        value, name = struct.unpack_from(">2I", raw, table + 8 * i)
        roots[raw[strings + name:].split(b"\0", 1)[0].decode()] = value
    load = roots["ftLoadCommonData"]
    def word(offset):
        return struct.unpack_from(">I", raw, 32 + offset)[0]
    result = []
    for kind, physical in ((18, 90), (26, 92)):
        parts = word(word(load + 16) + kind * 4)
        mapping = word(parts + 4)
        count = word(parts + 8)
        entries = list(enumerate(raw[32 + mapping:32 + mapping + 54]))
        omissions = word(word(load + 20) + kind * 4)
        omission_count = word(omissions + 4) if omissions else 0
        if count != physical or omission_count:
            raise ValueError("Unexpected stock rig contract; audit before changing metadata")
        result.append({"kind": kind, "physical_joints": physical, "runtime_parts": count,
            "omissions": [], "bindings": [(part, joint) for part, joint in entries if joint != 255]})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("sdk", "tool", "clang", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--common-dat", type=Path, default=ROOT / ".local/mewtwo_common_stock.dat")
    parser.add_argument("--marth-project", type=Path, default=ROOT / ".local/sword-owned-cpu-parity/Marth")
    parser.add_argument("--roy-project", type=Path, default=ROOT / ".local/sword-owned-cpu-parity/Roy")
    parser.add_argument("--candidate", action="store_true",
                        help="Allow fixture-only independence before template promotion")
    args = parser.parse_args()
    sdk, tool, clang, out = (getattr(args, name).resolve() for name in ("sdk", "tool", "clang", "output"))
    if out.exists():
        raise ValueError("Use a fresh output directory")
    out.mkdir(parents=True)
    def run(command):
        result = subprocess.run([str(x) for x in command], capture_output=True, text=True)
        with (out / "commands.log").open("a", encoding="utf-8", newline="\n") as log:
            log.write(subprocess.list2cmdline([str(x) for x in command]) + "\n" + result.stdout + result.stderr)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        return result.stdout
    families = [("marth", "ftMars/ftmars_clone.c", args.marth_project.resolve()),
                ("roy", "ftEmblem/ftemblem_clone.c", args.roy_project.resolve())]
    inputs = [tool, clang, args.common_dat.resolve()]
    inputs += list((sdk / "src").rglob("*.h"))
    inputs += list((sdk / "extern/dolphin/include").rglob("*.h"))
    inputs += [sdk / "src/melee/ft/native_fighter_parameters.c"]
    for path in ("marth_roy_closure_fixture.py", "marth_roy_closure_source_compare.py", "marth_roy_closure_stock_test.c"):
        inputs.append(ROOT / "tests/native" / path)
    for family, base, project in families:
        inputs += [ROOT / "examples" / (family + "_wip.fms"), ROOT / "src/melee/ft/kinds" / base]
        inputs.append(sdk / "src/melee/ft/kinds" / ("ftMars/ftmars.c" if family == "marth" else "ftEmblem/ftemblem.c"))
        inputs += [p for p in project.rglob("*") if p.is_file() and p.suffix.lower() not in (".c", ".dll", ".lib", ".exe")]
    hashes = {str(path): sha(path) for path in inputs}
    flags = [clang, "-std=gnu11", "-O2", "-DMELEE_NATIVE=1", "-D_CRT_SECURE_NO_WARNINGS",
             "-D_USE_MATH_DEFINES", "-fno-strict-float-cast-overflow", "-ffp-contract=off", "-fuse-ld=lld",
             "-I" + str(sdk / "src"), "-I" + str(sdk / "extern/dolphin/include")]
    module_args = []
    generated_files = []
    for family, _, project in families:
        copied = out / family
        shutil.copytree(project, copied, ignore=shutil.ignore_patterns(
            "*.c", "*.dll", "*.lib", "*.exe", "behavior.fms", "fighter_parameters.fmp"))
        behavior = copied / "behavior.fms"
        run([tool, "import-parameters", copied, ROOT / "examples" / (family + "_wip.fms"), behavior])
        owned = behavior.read_text(encoding="utf-8")
        independent = re.search(r"(?m)^\s*behavior independent\s*$", owned)
        if not independent:
            if not args.candidate or re.search(r"(?m)^fighter:", owned):
                raise ValueError("Shipped independent template required; --candidate permits only an unpromoted template without a fighter declaration")
            behavior.write_text("# Explicit fixture-only independent candidate.\nfighter:\n    behavior independent\n\n" + owned,
                                encoding="utf-8", newline="\n")
        generated, module = out / (family + ".generated.c"), out / (family + ".dll")
        run([tool, "script-c", behavior, "closure_" + family, generated])
        run(flags + ["-shared", generated, "-o", module])
        generated_files.append(generated)
        module_args.extend((module, copied / "fighter_parameters.fmp"))
    fixture = out / "marth_roy_closure_fixture.h"
    extract(sdk / "src", fixture)
    maps = rigs(args.common_dat.resolve())
    with fixture.open("a", encoding="utf-8", newline="\n") as output:
        output.write("static const NativeFighterBone closure_expected_bones[2][52] = {\n")
        for rig in maps:
            assert len(rig["bindings"]) == 52
            output.write("    {" + ",".join("{%d,%d}" % pair for pair in rig["bindings"]) + "},\n")
        output.write("};\n")
    test = out / "marth_roy_closure_stock_test.exe"
    run(flags + ["-I" + str(out), ROOT / "tests/native/marth_roy_closure_stock_test.c",
        *[ROOT / "src/melee/ft/kinds" / base for _, base, _ in families],
        sdk / "src/melee/ft/native_fighter_parameters.c", "-o", test])
    result = run([test, *module_args])
    match = re.search(r"(\d+) source comparisons, 0 failures", result)
    if not match:
        raise RuntimeError("Missing comparison result")
    for path, expected in hashes.items():
        if sha(Path(path)) != expected:
            raise RuntimeError(f"Input changed during source batch: {path}")
    evidence = {"comparisons": int(match[1]), "failures": 0, "output": result, "rigs": maps,
        "mode": "candidate" if args.candidate else "shipped independent",
        "scope": "full-template independent module opens; all32 motion rows replace original callbacks; original lifecycle C and common item/knockback inline decisions; known common policies",
        "limits": ["No gameplay, rendering, CPU planner matrix, special branch replay or costume animation scheduling",
                   "Presentation and lifetime Host boundaries are explicit fixtures; actual loader/lifetime acceptance remains separate",
                   "Charge entry resets ordinary state; retail undefined initial charge is not a reproducibility contract"],
        "input_sha256": hashes, "generated_sha256": {p.name: sha(p) for p in generated_files},
        "extracted_fixture_sha256": sha(fixture),
        "owned_source_sha256": {str(p.relative_to(out)): sha(p) for family, _, _ in families
            for p in (out / family / "behavior.fms", out / family / "fighter_parameters.fmp")}}
    (out / "acceptance.json").write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
