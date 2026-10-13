"""Normal Peach import and two actual-source suites against one composed module."""
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        while block := f.read(1024 * 1024):
            h.update(block)
    return h.hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ("root", "baseline", "output"):
        p.add_argument("--" + name, type=Path, required=True)
    p.add_argument("--iso", type=Path, default=HERE.parents[3] /
                   "Super Smash Bros. Melee (USA) (v1.02).iso")
    a = p.parse_args()
    root, base, out = a.root.resolve(), a.baseline.resolve(), a.output.resolve()
    sdk = base / "sdk"
    if out.exists():
        raise ValueError("Use a fresh output directory")
    out.mkdir(parents=True)
    commands = []

    def run(args, log):
        args = [str(x) for x in args]
        commands.append(args)
        result = subprocess.run(args, cwd=root, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        log.write_text(result.stdout, encoding="utf-8")
        if result.returncode:
            raise RuntimeError(result.stdout)
        return result.stdout

    clang, tool = base / "compiler/clang.exe", base / "bin/native_content_tool.exe"
    vcvars = Path("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat")

    def compile(args, directory, name):
        batch = directory / (name + ".cmd")
        batch.write_text('@echo off\ncall "' + str(vcvars) + '" >nul\n' +
                         subprocess.list2cmdline([str(x) for x in args]) + "\n")
        run(["cmd", "/c", batch], directory / (name + ".log"))

    common = [clang, "-std=gnu11", "-O2", "-fms-runtime-lib=dll", "-DMELEE_NATIVE=1",
              "-D_DLL", "-D_MT", "-D_USE_MATH_DEFINES", "-D_CRT_SECURE_NO_WARNINGS",
              "-I" + str(sdk / "src"), "-I" + str(sdk / "extern/dolphin/include")]
    components = json.loads((HERE / "components.json").read_text())
    for path, expected in components["source_inputs"].items():
        if digest(root / path) != expected:
            raise ValueError("Accepted component changed: " + path)
    project = out / "project"
    run([tool, "clone-stock", a.iso.resolve(), "Peach", project,
         "Peach combined source composition"], out / "clone.log")
    run([tool, "import-parameters", project, HERE / "candidate.fms", out / "owned.fms"],
        out / "import.log")
    shutil.copyfile(project / "fighter_parameters.fmp", out / "fighter_parameters.fmp")
    run([tool, "script-c", out / "owned.fms", "peach_full_composition", out / "generated.inc"],
        out / "generate.log")
    compile(common + ["-shared", "-x", "c", out / "generated.inc", "-o", out / "module.dll"],
            out, "compile_module")
    results = {}
    for name in ("pluck", "counter"):
        case = out / name
        case.mkdir()
        shutil.copyfile(out / "generated.inc", case / "generated.inc")
        run([sys.executable, HERE / (name + "_fixture.py"), "--root", sdk, "--output", case,
             "--generated", case / "generated.inc", "--parameters", out / "fighter_parameters.fmp",
             "--dat", project / "fighter.dat"], case / "extract.log")
        units = ["src/melee/ft/native_callback_lifetime.c", "src/melee/it/native_item_owner.c"]
        if name == "counter":
            units.append("src/melee/ft/native_fighter_events.c")
        compile(common + ["-I" + str(case), HERE / (name + "_test.c"),
                          *[sdk / unit for unit in units], "-o", case / "test.exe"],
                case, "compile_test")
        result = run([case / "test.exe"], case / "run.log")
        match = re.search(r"(\d+) original/generated comparisons, (\d+) checks", result)
        if not match:
            raise ValueError("Missing suite result: " + name)
        results[name] = {"comparisons": int(match[1]), "checks": int(match[2]),
                         "safety_cases": int(re.search(r"(\d+) authored safety cases", result)[1]),
                         "census": json.loads((case / "census.json").read_text())}
        print(result.strip())
    inputs = {path for path in sdk.rglob("*") if path.is_file()}
    inputs.update(path for path in (base / "compiler").rglob("*") if path.is_file())
    inputs.update(path for path in HERE.iterdir() if path.is_file())
    inputs.update(root / path for path in components["source_inputs"])
    inputs.update(path for path in project.iterdir() if path.is_file())
    inputs.update([a.iso.resolve(), base / "freeze.json", tool])
    receipt = {
        "status": "PASS unregistered inherited composition; unfinished routes remain inherited",
        "snapshot": base.name, "normal_project_pipeline": True, "suites": results,
        "comparisons": sum(x["comparisons"] for x in results.values()),
        "checks": sum(x["checks"] for x in results.values()),
        "safety_cases": sum(x["safety_cases"] for x in results.values()),
        "commands": commands,
        "limits": [
            "This composes accepted components; it does not add Float, Side-B or full Parasol travel.",
            "Source-kind Peach and inherited unfinished phase pointers remain explicit.",
            "No installation, activation, gameplay or independence acceptance.",
            "Existing animation/probes/allocation/AB54/render/debug controls remain disclosed in suite headers.",
            "Two original-body suites execute the same final generated module; no handwritten script surrogate.",
            "Prior setup/Turnip/cleanup/producer receipts are immutable reused evidence, not rerun caller coverage.",
            "Counter raw debug-owner source invalidation remains a controlled inner boundary.",
        ],
        "inputs": {str(path): digest(path) for path in sorted(inputs)},
    }
    archive = out / "source_snapshot"
    archive.mkdir()
    for path in HERE.iterdir():
        if path.is_file():
            shutil.copyfile(path, archive / path.name)
    receipt["outputs"] = {str(path): digest(path) for path in sorted(out.rglob("*")) if path.is_file()}
    (out / "result.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print("Receipt:", out / "result.json")


if __name__ == "__main__":
    main()
