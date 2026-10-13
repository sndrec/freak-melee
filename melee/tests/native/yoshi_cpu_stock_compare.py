"""Compile exact CPU source excerpts and compare an isolated authored module."""
import argparse
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
ORIGINAL = "14f03cb46"
ATTACK = "src/melee/ft/ftcpuattack.c"
COMMON = "src/melee/ft/kinds/ftCommon/ftCo_0A01.c"


def function(text, name):
    match = re.search(r"^(?:static inline |static )?(?:void|bool) " + name +
                      r"\([^;]*?\)\s*\{", text, re.MULTILINE)
    if not match:
        raise AssertionError("Function missing: " + name)
    start = match.end() - 1
    depth = 1
    end = start + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[match.start():end] + "\n"


def case(text, start, next_case):
    start = text.index("    case Ft_Kind_Yoshi:", start)
    end = text.index("    case " + next_case + ":", start)
    return text[start:end]


def excerpts(batch, sdk):
    original = {path: subprocess.check_output(
        ["git", "show", ORIGINAL + ":" + path], cwd=ROOT, text=True)
        for path in (ATTACK, COMMON)}
    attack, common = original[ATTACK], original[COMMON]
    names = ("ftCo_CpuSetNeutralStick", "ftCo_CpuFinishWithNeutralY",
             "ftCo_CpuTapR", "ftCo_800A0508", "ftCo_CpuUseAvailableJump",
             "ftCo_800A3234")
    source = "// Exact original source at " + ORIGINAL + "; names isolated for comparison.\n"
    for name in names:
        source += function(common, name)
    for name in ("ftCo_CpuClearTargetAndFinish", "ftCo_800B9340", "ftCo_800B9020", "ftCo_800B98C8"):
        source += function(attack, name)
    for name in names + ("ftCo_CpuClearTargetAndFinish", "ftCo_800B9340", "ftCo_800B9020", "ftCo_800B98C8"):
        source = re.sub(r"\b" + name + r"\b", "original_" + name, source)
    source += function(common, "ftCo_800A2170") + function(common, "ftCo_800A2C08")
    source += "\nstatic void original_ranged(Fighter* fp) {\nstruct CpuFighter* cpu = &fp->cpu; Fighter* target; bool can_attack; switch(fp->kind) {\n"
    source += case(attack, attack.index("void ftCo_800B77E8"), "Ft_Kind_Luigi")
    source += "default: break; } }\n"
    source += "\nstatic bool original_threat(Fighter* fp, Fighter* arg1) {\nVec3 sp24, sp18; int sp14; u32 sp10; float temp_f1,temp_f2,temp_f3,temp_f4; struct CpuFighter* temp_r31=&fp->cpu; switch(arg1->kind) {\n"
    source += case(attack, attack.index("bool ftCo_800BB768"), "Ft_Kind_Samus")
    source += "default: break; } return false; }\n"
    (batch / "cpu_original.inc").write_text(source)
    current_attack = (sdk / ATTACK).read_text()
    current_common = (sdk / COMMON).read_text()
    source = "// Actual frozen production routing bodies, not replacements.\n"
    for name, text in (("ftCo_800B9020", current_attack),
                       ("ftCo_800A3234", current_common),
                       ("ftCo_CpuUseAvailableJump", current_common)):
        source += function(text, name).replace(name, "production_" + name)
    (batch / "cpu_production.inc").write_text(source)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--batch", type=Path, required=True)
    parser.add_argument("--snapshot", type=Path, required=True)
    args = parser.parse_args()
    batch, snapshot = args.batch.resolve(), args.snapshot.resolve()
    batch.mkdir(exist_ok=True, parents=True)
    sdk = snapshot / "sdk"
    excerpts(batch, sdk)
    executable = batch / "yoshi_cpu_stock_policy_test.exe"
    command = [str(snapshot / "compiler/clang.exe"), "-fms-runtime-lib=dll",
               "-std=gnu11", "-O2", "-DMELEE_NATIVE=1", "-D_USE_MATH_DEFINES",
               "-D_CRT_SECURE_NO_WARNINGS", "-D_DLL", "-D_MT",
               "-I" + str(sdk / "src"), "-I" + str(sdk / "extern/dolphin/include"),
               "-I" + str(batch), str(ROOT / "tests/native/yoshi_cpu_stock_policy_test.c"),
               str(sdk / "src/melee/ft/native_fighter_parameters.c"),
               str(sdk / "src/melee/ft/native_callback_lifetime.c"), "-o", str(executable)]
    setup = Path("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat")
    script = batch / "cpu-fixture-compile.cmd"
    script.write_text("@echo off\ncall " + subprocess.list2cmdline([str(setup)]) +
                      " >nul\n" + subprocess.list2cmdline(command) + "\n")
    with (batch / "cpu-fixture-build.log").open("wb") as log:
        result = subprocess.run(["cmd", "/c", str(script)], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        raise AssertionError((batch / "cpu-fixture-build.log").read_text())
    result = subprocess.run([str(executable), str(batch / "project/module.dll"),
                             str(batch / "project/fighter_parameters.fmp")],
                            cwd=ROOT, text=True, capture_output=True)
    (batch / "cpu-fixture-results.log").write_text(result.stdout + result.stderr)
    print(result.stdout + result.stderr, end="")
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
