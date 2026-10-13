"""Build/run the direct original-C common-policy fixture with a frozen SDK."""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--batch", type=Path, required=True)
    args = parser.parse_args()
    batch = args.batch.resolve()
    executable = batch / "yoshi_common_policy_test.exe"
    sdk = batch / "sdk"
    command = [str(batch / "compiler/clang.exe"), "-fms-runtime-lib=dll",
               "-std=gnu11", "-O2", "-DMELEE_NATIVE=1",
               "-D_USE_MATH_DEFINES", "-D_CRT_SECURE_NO_WARNINGS",
               "-D_DLL", "-D_MT", "-I" + str(sdk / "src"),
               "-I" + str(sdk / "extern/dolphin/include"),
               str(ROOT / "tests/native/yoshi_common_policy_test.c"),
               str(sdk / "src/melee/ft/native_fighter_parameters.c"),
               str(sdk / "src/melee/ft/native_capture.c"),
               str(sdk / "src/melee/ft/kinds/ftCommon/ftCo_CaptureWait.c"),
               "-o", str(executable)]
    setup = Path("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat")
    script = batch / "common-fixture-compile.cmd"
    script.write_text("@echo off\ncall " + subprocess.list2cmdline([str(setup)]) +
                      " >nul\n" + subprocess.list2cmdline(command) + "\n")
    with (batch / "common-fixture-build.log").open("wb") as log:
        result = subprocess.run(["cmd", "/c", str(script)], cwd=ROOT,
                                stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        raise AssertionError((batch / "common-fixture-build.log").read_text())
    result = subprocess.run([str(executable), str(batch / "project/module.dll"),
                             str(batch / "project/fighter_parameters.fmp")],
                            cwd=ROOT, text=True, capture_output=True)
    (batch / "common-fixture-results.log").write_text(result.stdout + result.stderr)
    print(result.stdout + result.stderr, end="")
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
