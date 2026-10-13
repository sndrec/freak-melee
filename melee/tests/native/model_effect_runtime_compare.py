"""Batch drawn stock/clone regressions after authored Tornado/Cyclone updates.

All projects and installed test packages live in the supplied output directory.
Games are hidden and muted through run_fighter_parity.py. Fighter traces do not
measure rendered model joint parity; model_effect_stock_source_compare.py covers the
actual generated visual decisions against the original source separately.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

from fighter_script_bundle_roundtrip import bundled_id, portrait

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--iso", type=Path,
                        default=ROOT.parent / "Super Smash Bros. Melee (USA) (v1.02).iso")
    args = parser.parse_args()
    snapshot, output, iso = (p.resolve() for p in
                             (args.snapshot, args.output, args.iso))
    output.mkdir(parents=True, exist_ok=False)
    tool = snapshot / "bin/native_content_tool.exe"
    image, store = output / "portrait.png", output / "store"
    portrait(image)
    rows = []

    def run(arguments, log):
        result = subprocess.run([str(v) for v in arguments], cwd=ROOT,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                text=True)
        log.write_text(result.stdout, encoding="utf-8")
        if result.returncode:
            raise RuntimeError(f"Exit {result.returncode}; inspect {log}")
        return result.stdout

    cases = (("Mario", "mario_full.fms", "drmario-tornado-repeat-inputs.fmr", 350),
             ("Dr. Mario", "drmario_full.fms", "drmario-tornado-repeat-inputs.fmr", 350),
             ("Luigi", "luigi_wip.fms", "drmario-special-down-ground-inputs.fmr", 357))
    try:
        for fighter, template, recording, motion in cases:
            case = output / template.removesuffix(".fms")
            case.mkdir()
            project = case / "project"
            source = ROOT / "examples" / template
            clone_base = "DrMario" if fighter == "Dr. Mario" else fighter
            run([tool, "clone-stock", iso, clone_base, project, fighter + " Effects"],
                case / "clone.log")
            run([tool, "import-parameters", project, source, project / "behavior.fms"],
                case / "import.log")
            run([tool, "script-build", project, project / "behavior.fms"],
                case / "script-build.log")
            package = bundled_id(run([tool, "--root", store, "bundle-project",
                                      project, image, iso], case / "bundle.log"))
            run([tool, "--root", store, "verify-package", package, iso],
                case / "verify.log")
            run([tool, "--root", store, "accept", package, iso], case / "accept.log")
            inputs = ROOT / "tests/native/parity_inputs" / recording
            run([sys.executable, ROOT / "tools/run_fighter_parity.py",
                 "--iso", iso, "--exe", snapshot / "bin/melee_native.exe",
                 "--stock-fighter", clone_base, "--package-id", package,
                 "--content-root", store, "--inputs", inputs,
                 "--out-dir", case / "runtime", "--stocks", "2",
                 "--independent", "--expect-motion", motion], case / "parity.log")
            paths = (source, inputs, project / "behavior.fms",
                     project / "behavior.generated.c", project / "fighter_parameters.fmp",
                     project / "module.dll")
            rows.append({"fighter": fighter, "exit_code": 0, "package": package,
                         "expected_motion": motion,
                         "sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
                                    for p in paths}})
            print(f"{fighter}: stock/clone match passed", flush=True)
    except (RuntimeError, OSError) as error:
        print(error, file=sys.stderr)
        rows.append({"exit_code": 1, "error": str(error)})
    (output / "result.json").write_text(json.dumps({
        "snapshot": str(snapshot),
        "snapshot_sha256": hashlib.sha256((snapshot / "freeze.json").read_bytes()).hexdigest(),
        "cases": rows,
        "limits": "Drawn fighter traces; model joint parity is a separate source fixture."
    }, indent=2) + "\n", encoding="utf-8")
    return int(len(rows) != 3 or any(row["exit_code"] for row in rows))


if __name__ == "__main__":
    raise SystemExit(main())
