"""Manual stock/script side-dash comparisons with rendering and real project values."""

import argparse
import datetime
import json
from pathlib import Path
import subprocess
import sys

from fighter_script_bundle_roundtrip import run, portrait, bundled_id

ROOT = Path(__file__).resolve().parents[2]
INPUTS = ROOT / "tests/native/parity_inputs"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, default=ROOT / ".local" /
                        ("side-parity-" + datetime.datetime.now().strftime("%Y%m%d-%H%M%S")))
    args = parser.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=False)
    image = args.out_dir / "portrait.png"
    portrait(image)
    tool = str(ROOT / "build/native/native_content_tool.exe")
    results = []
    for fighter in ("Fox", "Falco"):
        project = args.out_dir / fighter
        store = args.out_dir / (fighter + "-store")
        owned = args.out_dir / (fighter + "-owned.fms")
        run(tool, "clone-stock", args.iso, fighter, project, fighter + " Script Parity")
        run(tool, "import-parameters", project,
            ROOT / "examples" / (fighter.lower() + "_wip.fms"), owned)
        run(tool, "script-build", project, owned)
        package = bundled_id(run(tool, "--root", store, "bundle-project",
                                 project, image, args.iso))
        run(tool, "--root", store, "accept", package, args.iso)
        cases = [
            (f"fighter-side-{mode}-{direction}", [350, 351] if mode == "air" else [347, 348], [])
            for mode in ("ground", "air", "short") for direction in ("right", "left")
        ]
        cases.append(("fighter-side-contact-right", [347, 348, 349], ["--expect-damage-port", "2"]))
        for name, motions, extra in cases:
            print(f"Comparing {fighter}: {name}", flush=True)
            item_kind = 56 if fighter == "Fox" else 57
            command = [sys.executable, str(ROOT / "tools/run_fighter_parity.py"),
                       "--iso", str(args.iso.resolve()),
                       "--inputs", str(INPUTS / (name + "-inputs.fmr")),
                       "--stock-fighter", fighter.lower(), "--package-id", package,
                       "--content-root", str(store.resolve()), "--stocks", "2",
                       "--out-dir", str(args.out_dir / (fighter + "-traces")),
                       "--expect-item-kind", str(item_kind), *extra]
            for motion in motions:
                command.extend(("--expect-motion", str(motion)))
            result = subprocess.run(command, cwd=ROOT)
            results.append(dict(fighter=fighter, case=name, package=package,
                                exit=result.returncode))
            (args.out_dir / "summary.json").write_text(json.dumps(results, indent=2))
    passed = sum(result["exit"] == 0 for result in results)
    print(f"Passed {passed}/{len(results)} side-dash comparisons", flush=True)
    return int(passed != len(results))


if __name__ == "__main__":
    raise SystemExit(main())
