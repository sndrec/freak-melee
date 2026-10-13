"""Compare actual generated Fox/Falco CPU callbacks with frozen original C.

No game or global build is launched. Projects must already have used normal
import-parameters/script-build with the matching snapshot tool and SDK.
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
    match = re.search(r"^(?:static )?(?:inline )?(?:void|bool|f32) " + name +
                      r"\([^;]*?\)\s*\{", text, re.MULTILINE)
    if not match:
        raise AssertionError("Missing original function: " + name)
    return declaration(text, match.group(0)) + "\n"


def select_native(text, native):
    # Select native/retail branches before brace extraction, preserving all
    # other preprocessor conditions and the original policy statements.
    stack = [(True, None)]
    output = []
    for line in text.splitlines(keepends=True):
        directive = line.strip()
        active = stack[-1][0]
        if directive == "#ifdef MELEE_NATIVE":
            stack.append((active and native, native))
        elif directive.startswith(("#if ", "#ifdef ", "#ifndef ")):
            if active:
                output.append(line)
            stack.append((active, None))
        elif directive == "#else" and len(stack) > 1:
            selected = stack[-1][1]
            if selected is None:
                if active:
                    output.append(line)
            else:
                stack[-1] = (stack[-2][0] and not selected, not selected)
        elif directive == "#endif" and len(stack) > 1:
            selected = stack.pop()[1]
            if selected is None and stack[-1][0]:
                output.append(line)
        elif active:
            output.append(line)
    assert len(stack) == 1
    return "".join(output)



def prepare(sdk, reference, project, out, family):
    hashes = {}

    def read(path):
        data = path.read_bytes()
        hashes[str(path)] = hashlib.sha256(data).hexdigest()
        return data.decode()

    original = select_native(read(reference / "src/melee/ft/ftcpuattack.c"), False)
    current = select_native(read(sdk / "src/melee/ft/ftcpuattack.c"), True)
    command = read(reference / "src/melee/ft/ftcmdscript.c")
    random = read(reference / "src/sysdolphin/baselib/random.c")
    helpers = ""
    for name in ("ftCo_CpuClearTargetAndFinish", "ftCo_CpuSetNeutralStick",
                 "ftCo_CpuFlickLstickX", "ftCo_CpuPressAWithLstickX",
                 "ftCo_800B9F90", "ftCo_800BA080", "ftCo_800BA080_dontinline",
                 "ftCo_800BA160", "ftCo_800BA224", "inline0", "inline1", "inline2",
                 "ftCo_CpuIsRollOrAirDodge", "ftCo_CpuIsSpotDodge",
                 "ftCo_CpuFireBlaster", "ftCo_CpuRetapR"):
        helpers += function(original, name)
    helpers += declaration(original, "typedef struct ftCo_CollData") + " ftCo_CollData;\n"
    for name in ("ftCo_CpuRangedAttackUnsafe", "ftCo_CpuAddAttackCandidate"):
        helpers += function(original, name)
    for name in ("ftCo_800B463C", "ftCo_800B46B8", "ftCo_800B4778", "ftCo_800B4880"):
        helpers += function(command, name)
    helpers += "u32 seed = 1; u32* seed_ptr = &seed;\n" + function(random, "HSD_Randf").replace(
        "f32 HSD_Randf(", "static f32 original_randf(")
    (out / "fox_cpu_original_helpers.inc").write_text(helpers)
    # Preprocessing removes native hook blocks only for the original baseline.
    # Both complete capture bodies retain their common throw/pummel tail.
    bodies = ""
    for name, replacement in (("ftCo_800B683C", "stock_capture_step"),
                              ("ftCo_800BA9A0", "stock_defense_step")):
        bodies += function(original, name).replace(name + "(", replacement + "(", 1)

    for name, replacement in (("ftCo_800B683C", "owned_capture_step"),
                              ("ftCo_800BA9A0", "owned_defense_step")):
        bodies += function(current, name).replace(name + "(", replacement + "(", 1)
    ranged = original[original.index("void ftCo_800B77E8"):]
    start = ranged.index("    case Ft_Kind_Fox:")
    end = ranged.index("    case Ft_Kind_Captain:", start)
    bodies += "static void stock_ranged(Fighter* fp) { Fighter* target; int can_attack;\nswitch(fp->kind) {\n" + ranged[start:end] + "default: break; } }\n"
    recovery = read(reference / "src/melee/ft/kinds/ftCommon/ftCo_0A01.c")
    recovery = recovery[recovery.index("void ftCo_800A96B8"):]
    start = recovery.index("    case Ft_Kind_Fox:")
    end = recovery.index("    case Ft_Kind_Yoshi:", start)
    bodies += "static void stock_recovery(Fighter* fp) { switch(fp->kind) {\n" + recovery[start:end] + "default: break; } }\n"
    (out / "fox_cpu_steps.inc").write_text(bodies)

    stem = "Fox" if family == "fox" else "Falco"
    fighter_source = read(sdk / f"src/melee/ft/kinds/ft{stem}/ft{family}.c")
    table_name = "ftFx_Init_MotionStateTable" if family == "fox" else "ftFc_Init_MotionStateTable"
    table = declaration(fighter_source, "MotionState " + table_name) + ";\n"
    clone = read(sdk / f"src/melee/ft/kinds/ft{stem}/ft{family}_clone.c")
    base = "\n".join(declaration(clone, marker) + ";" for marker in (
        f"static const NativeFighterBone ft{stem}Clone_Bones",
        f"static const NativeFighterArticleAsset ft{stem}Clone_ArticleAssets",
        f"static const Fighter_DemoStrings ft{stem}Clone_DemoMotionNames",
        f"const NativeFighterDefinition ft{stem}Clone_Definition"))
    callbacks = sorted(set(re.findall(r"\bft(?:Fx_[A-Za-z0-9_]+|Fc_[A-Za-z0-9_]+|Camera_UpdateCameraBox)\b", table + base)))
    callbacks = [name for name in callbacks if name != table_name and
                 not any(token in name for token in ("_MS_", "_MF_", "_SM_"))]
    (out / "fox_cpu_base.inc").write_text(
        "\n".join("#define " + name + " stock_slot" for name in callbacks) + "\n" + table + base +
        "\n" + "\n".join("#undef " + name for name in callbacks) +
        f"\n#define TEST_BASE ft{stem}Clone_Definition\n")
    generated = read(project / "behavior.generated.c")
    fields = sorted(set(re.findall(r"host->(\w+) == 0", generated)))
    (out / "fox_cpu_required_host.inc").write_text("\n".join(
        "host." + field + " = (__typeof__(host." + field + "))unused_host;"
        for field in fields) + "\n")
    (out / "fox_cpu_generated.inc").write_text(generated)
    for path in (project / "behavior.fms", project / "fighter_parameters.fmp", project / "module.dll",
                 ROOT / "examples" / (family.lower() + "_wip.fms"), sdk.parent / "freeze.json",
                 ROOT / "tests/native/fox_falco_cpu_stock_test.c", Path(__file__)):
        if path.exists():
            hashes[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
    (out / "source_manifest.json").write_text(json.dumps(hashes, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--reference", type=Path, required=True, help="Frozen SDK before CPU hook changes")
    parser.add_argument("--project", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--family", choices=("fox", "falco"), required=True)
    parser.add_argument("--prepare-only", action="store_true")
    args = parser.parse_args()
    snapshot, reference, project, out = (p.resolve() for p in (
        args.snapshot, args.reference, args.project, args.out_dir))
    out.mkdir(parents=True, exist_ok=True)
    sdk = snapshot / "sdk"
    prepare(sdk, reference, project, out, args.family)
    if args.prepare_only:
        return 0
    executable = out / "fox_falco_cpu_stock_test.exe"
    command = [str(snapshot / "compiler/clang.exe"), "-fms-runtime-lib=dll", "-std=gnu11", "-O2",
               "-ffp-contract=off", "-DMELEE_NATIVE=1", "-D_USE_MATH_DEFINES",
               "-D_CRT_SECURE_NO_WARNINGS", "-D_DLL", "-D_MT",
               "-I" + str(sdk / "src"), "-I" + str(sdk / "extern/dolphin/include"),
               "-I" + str(out), str(ROOT / "tests/native/fox_falco_cpu_stock_test.c"),
               str(sdk / "src/melee/ft/native_fighter_parameters.c"), "-o", str(executable)]
    setup = Path("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat")
    script = out / "compile.cmd"
    script.write_text("@echo off\ncall " + subprocess.list2cmdline([str(setup), "-vcvars_ver=14.44"]) +
                      " >nul\n" + subprocess.list2cmdline(command) + "\n")
    with (out / "build.log").open("wb") as log:
        result = subprocess.run(["cmd", "/c", str(script)], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        raise AssertionError((out / "build.log").read_text())
    result = subprocess.run([str(executable), str(project / "fighter_parameters.fmp"), args.family],
                            cwd=ROOT, text=True, capture_output=True)
    (out / "results.log").write_text(result.stdout + result.stderr)
    print(result.stdout + result.stderr, end="")
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
