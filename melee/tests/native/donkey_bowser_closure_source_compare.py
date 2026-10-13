"""Bounded console batch: actual Donkey/Bowser templates, original C, owned rig maps.

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

from donkey_bowser_closure_source_fixture import extract, SOURCE_UNITS

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
    for kind, physical in ((3, 75), (5, 76)):
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
    parser.add_argument("--donkey-project", type=Path, default=ROOT / ".local/donkey-owned-final-heavy/project")
    parser.add_argument("--bowser-project", type=Path, default=ROOT / ".local/bowser-capture-final/project")
    parser.add_argument("--candidate", action="store_true",
                        help="Allow fixture-only independence before template promotion")
    parser.add_argument("--bundle", action="store_true", help="Bundle and verify candidates in the fresh private store")
    parser.add_argument("--iso", type=Path, default=ROOT.parent / "Super Smash Bros. Melee (USA) (v1.02).iso")
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
    families = [("donkey", "ftDonkey/ftdonkey_clone.c", args.donkey_project.resolve()),
                ("koopa", "ftKoopa/ftkoopa_clone.c", args.bowser_project.resolve())]
    inputs = [tool, clang, args.common_dat.resolve()]
    portrait = tool.parent / "ui/workshop_draft_portrait.png"
    if args.bundle:
        inputs += [portrait, args.iso.resolve()]
    inputs += list((sdk / "src").rglob("*.h"))
    inputs += list((sdk / "extern/dolphin/include").rglob("*.h"))
    inputs += [sdk / "src/melee/ft/native_fighter_parameters.c", sdk / "src/melee/ft/native_callback_lifetime.c"]
    inputs += [sdk / "src" / path for path in SOURCE_UNITS]
    for path in ("donkey_bowser_closure_source_fixture.py", "donkey_bowser_closure_source_compare.py", "donkey_bowser_closure_source_test.c"):
        inputs.append(ROOT / "tests/native" / path)
    for family, base, project in families:
        inputs += [ROOT / "examples" / (family + "_wip.fms"), ROOT / "src/melee/ft/kinds" / base]
        inputs.append(sdk / "src/melee/ft/kinds" / ("ftDonkey/ftdonkey.c" if family == "donkey" else "ftKoopa/ftkoopa.c"))
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
        code = dict(line.split("=",1) for line in (copied / "project.txt").read_text().splitlines() if "=" in line)["code"]
        run([tool, "script-c", behavior, code, generated])
        run(flags + ["-shared", generated, "-o", module])
        generated_files.append(generated)
        module_args.extend((module, copied / "fighter_parameters.fmp"))
    fixture = out / "donkey_bowser_closure_source_fixture.h"
    extract(sdk / "src", fixture)
    maps = rigs(args.common_dat.resolve())
    with fixture.open("a", encoding="utf-8", newline="\n") as output:
        output.write("static const NativeFighterBone closure_expected_bones[2][52] = {\n")
        for rig in maps:
            assert len(rig["bindings"]) in (48,52)
            output.write("    {" + ",".join("{%d,%d}" % pair for pair in rig["bindings"]) + "},\n")
        output.write("};\n")
    with fixture.open("a", encoding="utf-8", newline="\n") as output:
        output.write("static const uint32_t closure_stock_attribute_words[2][512] = {\n")
        for family, _, _ in families:
            words = [0] * 512
            template = (ROOT / "examples" / (family + "_wip.fms")).read_text()
            fmp = (out / family / "fighter_parameters.fmp").read_text()
            values = {name: (kind, value) for kind, name, value in re.findall(r"(?m)^(int|float|bool) (\w+) = ([^\n]+)", fmp)}
            for kind, name, offset in re.findall(r"(?m)^attribute (int|float|bool) (\w+) (\d+)", template):
                actual, value = values[name]; assert actual == kind
                offset = int(offset); assert offset % 4 == 0
                words[offset//4] = struct.unpack("<I", struct.pack("<f", float(value)))[0] if kind == "float" else int(value) & 0xffffffff
            words[0], words[1] = (341,351) if family == "donkey" else (words[0],words[1])
            output.write("    {" + ",".join("UINT32_C(%d)" % word for word in words) + "},\n")
        output.write("};\n")
    test = out / "donkey_bowser_closure_source_test.exe"
    run(flags + ["-I" + str(out), ROOT / "tests/native/donkey_bowser_closure_source_test.c",
        *[ROOT / "src/melee/ft/kinds" / base for _, base, _ in families],
        sdk / "src/melee/ft/native_fighter_parameters.c", sdk / "src/melee/ft/native_callback_lifetime.c", "-o", test])
    result = run([test, *module_args])
    match = re.search(r"(\d+) source comparisons, 0 failures", result)
    if not match:
        raise RuntimeError("Missing comparison result")
    packages = {}
    if args.bundle:
        for (family, _, _), module in zip(families, module_args[::2]):
            copied = out / family
            shutil.copyfile(module, copied / "module.dll")
            result = run([tool, "--root", out / "store", "bundle-project", copied, portrait, args.iso.resolve()])
            package = re.search(r"not installed\): ([0-9a-f]{64})", result)
            if not package:
                raise RuntimeError("Missing bundled package ID")
            package = package[1]
            verification = run([tool, "--root", out / "store", "verify-package", package, args.iso.resolve()])
            (copied / "package.txt").write_text(package + "\n", encoding="utf-8", newline="\n")
            packages[family] = {"id": package, "output": result, "verification": verification, "module_sha256": sha(module)}
        packages["package_files"] = {str(p.relative_to(out)): sha(p) for p in (out / "store").rglob("*") if p.is_file()}
    for path, expected in hashes.items():
        if sha(Path(path)) != expected:
            raise RuntimeError(f"Input changed during source batch: {path}")
    evidence = {"comparisons": int(match[1]), "failures": 0, "output": result, "rigs": maps,
        "mode": "candidate" if args.candidate else "shipped independent",
        "scope": "actual full-template independent modules, all46 Donkey/23 Koopa motion rows and52/48 retail bone maps; original lifecycle/common item inline decisions; exact CPU spacing/cargo/charge/ranged/threat policies; heavy damage/throw/walk and common weight/victim-pose/finish callers; cargo and punch-release collision boundaries; module ownership rejection guards",
        "limits": ["No gameplay, renderer/audio, complete CPU planner, special branch replay, costume scheduling or rollback",
                   "World/collision, animation/item presentation and capture initialization are recorded Host boundaries; real plink generation validation is linked",
                   "Cargo transitions compare actual common transition math with controlled shared collision-probe outcomes; no full terrain collision solver is run",
                   "Authored missing-victim cargo finish safely falls through; original cargo initializer requires a nonnull victim",
                   "Retail undefined private-union initialization is not asserted as reproducible gameplay behavior"],
        "packages": packages,
        "input_sha256": hashes, "generated_sha256": {p.name: sha(p) for p in generated_files},
        "extracted_fixture_sha256": sha(fixture),
        "owned_source_sha256": {str(p.relative_to(out)): sha(p) for family, _, _ in families
            for p in (out / family / "behavior.fms", out / family / "fighter_parameters.fmp")}}
    (out / "acceptance.json").write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
