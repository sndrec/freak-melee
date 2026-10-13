"""Prepare/compile Purin's generated callbacks against pre-batch stock bodies.

Requires a normally imported/script-built Purin project and matching current
SDK/compiler snapshot. Reference bodies come from the immutable baseline15 SDK.
This source fixture launches no game. Engine I/O boundaries are fixture mocks.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def declaration(text, marker):
    start = text.index(marker)
    opening = text.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


def function(text, name):
    match = re.search(r"^(?:static inline |static )?(?:void|bool|float|s32) " +
                      re.escape(name) + r"\([^;]*?\)\s*\{", text, re.MULTILINE)
    if not match:
        raise ValueError("Missing production body: " + name)
    return declaration(text, match.group(0)) + "\n"


def prepare(reference, sdk, project, out):
    hashes = {}

    def read(root, relative):
        path = root / relative
        data = path.read_bytes()
        hashes[str(path)] = hashlib.sha256(data).hexdigest()
        return data.decode().replace("\r\n", "\n")

    f1 = read(reference, "src/melee/ft/kinds/ftCommon/ftCo_JumpAerialF1.c")
    aerial = read(reference, "src/melee/ft/kinds/ftCommon/ftCo_JumpAerial.c")
    shield = read(reference, "src/melee/ft/kinds/ftCommon/ftCo_ShieldBreakFly.c")
    bodies = function(aerial, "ft_SetVec") + function(aerial, "ft_800CB6EC")
    bodies += function(aerial, "ftCo_JumpAerial_Begin")
    bodies += function(aerial, "ftCo_JumpAerial_IASA")
    bodies += function(aerial, "ft_did_jump")
    bodies += function(read(reference, "src/melee/ft/kinds/ftCommon/ftCo_0D72.c"),
                       "ftCo_800D72A0")
    for name in ("ftCo_800D730C", "ftCo_800D74A4", "ftCo_JumpAerialF1_Anim",
                 "ftCo_JumpAerialF1_IASA", "ftCo_JumpAerialF1_Phys",
                 "ftCo_JumpAerialF1_Coll"):
        bodies += function(f1, name)
    bodies += function(shield, "ftCo_80098B20")
    bodies += function(read(reference, "src/melee/ft/ft_081B.c"), "ft_80082F28")
    bodies = bodies.replace("ftCo_800D74A4_inline(fp)", "fp->x2D0")
    renamed = ("ftCo_JumpAerial_Begin", "ftCo_JumpAerial_IASA", "ftCo_800D74A4",
               "ftCo_JumpAerialF1_Anim", "ftCo_JumpAerialF1_IASA",
               "ftCo_JumpAerialF1_Phys", "ftCo_JumpAerialF1_Coll", "ftCo_80098B20")
    for name in renamed:
        bodies = re.sub(r"\b" + name + r"\b", "reference_" + name, bodies)
    # Generic input must call current entry; compare it separately against the
    # reference entry, retaining all unchanged input-gating source statements.
    bodies = bodies.replace("reference_ftCo_800D74A4(gobj);", "fixture_jump_entry(gobj);")
    (out / "purin_jump_original.inc").write_text(bodies)
    actual = ""
    for path, names in {
        "src/melee/ft/kinds/ftCommon/ftCo_JumpAerialF1.c": ["ftCo_800D74A4"],
        "src/melee/ft/kinds/ftCommon/ftCo_ShieldBreakFly.c": ["ftCo_80098B20"],
        "src/melee/ft/kinds/ftCommon/ftCo_JumpAerial.c": ["ftCo_JumpAerial_Begin", "ftCo_JumpAerial_IASA"],
        "src/melee/ft/kinds/ftCommon/ftCo_FallAerial.c": ["ftCo_FallAerial_Enter"],
    }.items():
        text = read(sdk, path)
        for name in names:
            actual += function(text, name)
    actual = actual.replace("ftCo_800D74A4_inline(fp)", "fp->x2D0")
    (out / "purin_jump_current.inc").write_text(actual)
    table = declaration(read(reference, "src/melee/ft/kinds/ftPurin/ftpurin.c"),
                        "MotionState ftPr_Init_MotionStateTable") + ";\n"
    clone = read(sdk, "src/melee/ft/kinds/ftPurin/ftpurin_clone.c")
    base = "\n".join(declaration(clone, marker) + ";" for marker in (
        "static const NativeFighterBone ftPurinClone_Bones",
        "static const Fighter_DemoStrings ftPurinClone_DemoMotionNames",
        "const NativeFighterDefinition ftPurinClone_Definition"))
    symbols = sorted(set(re.findall(r"\bft(?:Pr_[A-Za-z0-9_]+|Co_JumpAerialF1_[A-Za-z0-9_]+|Camera_UpdateCameraBox)\b", table + base)) -
                     {"ftPr_Init_MotionStateTable", "ftPr_MS_SelfCount"})
    symbols = [name for name in symbols if "_MS_" not in name and "_MF_" not in name and "_SM_" not in name
               and name != "ftPr_SpecialAirNCharged"]
    (out / "purin_jump_base.inc").write_text(
        "\n".join("#define " + name + " stock_slot" for name in symbols) +
        "\n" + table + base + "\n" + "\n".join("#undef " + name for name in symbols) + "\n")
    generated = read(ROOT, str(project / "behavior.generated.c"))
    (out / "purin_jump_generated.inc").write_text(generated)
    fields = sorted(set(re.findall(r"host->(\w+) == 0", generated)))
    (out / "purin_jump_host.inc").write_text(
        "\n".join("    host." + field + " = (__typeof__(host." + field + "))unused_host;"
                  for field in fields if field != "fighter_parameters") + "\n")
    for path in (Path(__file__), ROOT / "tests/native/purin_jump_stock_test.c",
                 project / "behavior.fms", project / "fighter_parameters.fmp",
                 project / "module.dll"):
        hashes[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
    (out / "source_manifest.json").write_text(json.dumps(hashes, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--reference", type=Path, required=True,
                        help="Immutable baseline15 SDK, before this batch")
    parser.add_argument("--project", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--prepare-only", action="store_true")
    parser.add_argument("--compile-only", action="store_true",
                        help="Compile the comparison fixture without running it")
    args = parser.parse_args()
    snapshot, reference, project, out = (p.resolve() for p in (
        args.snapshot, args.reference, args.project, args.out_dir))
    sdk = snapshot / "sdk"
    out.mkdir(parents=True, exist_ok=True)
    prepare(reference, sdk, project, out)
    if args.prepare_only:
        return 0
    exe = out / "purin_jump_stock_test.exe"
    command = [str(snapshot / "compiler/clang.exe"), "-fms-runtime-lib=dll",
               "-std=gnu11", "-O2", "-ffp-contract=off", "-DMELEE_NATIVE=1",
               "-D_USE_MATH_DEFINES", "-D_CRT_SECURE_NO_WARNINGS", "-D_DLL", "-D_MT",
               "-I" + str(sdk / "src"), "-I" + str(sdk / "extern/dolphin/include"),
               "-I" + str(out), str(ROOT / "tests/native/purin_jump_stock_test.c"),
               str(sdk / "src/melee/ft/native_fighter_parameters.c"),
               str(sdk / "src/melee/ft/native_fighter_jump.c"), "-o", str(exe)]
    setup = Path("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat")
    cmd = out / "compile.cmd"
    cmd.write_text("@echo off\ncall " + subprocess.list2cmdline([str(setup), "-vcvars_ver=14.44"]) +
                   " >nul\n" + subprocess.list2cmdline(command) + "\n")
    with (out / "build.log").open("wb") as log:
        result = subprocess.run(["cmd", "/c", str(cmd)], cwd=ROOT,
                                stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        raise AssertionError((out / "build.log").read_text())
    if args.compile_only:
        print("Compiled source comparison fixture: " + str(exe))
        return 0
    result = subprocess.run([str(exe), str(project / "fighter_parameters.fmp")],
                            cwd=ROOT, text=True, capture_output=True)
    (out / "results.log").write_text(result.stdout + result.stderr)
    print(result.stdout + result.stderr, end="")
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
