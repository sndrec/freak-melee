"""Compare authored Yoshi specials/articles against stock in hidden drawn matches."""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from fighter_script_bundle_roundtrip import bundled_id, portrait, run

ROOT = Path(__file__).resolve().parents[2]
EGG, STAR = 86, 88


def recording(seed, samples, special, charge, aim, facing):
    template = bytearray((ROOT / "tests/native/parity_inputs/"
                          "drmario-special-neutral-ground-inputs.fmr").read_bytes())
    if template[:8] != b"FMREPLAY" or len(template) < 104:
        raise AssertionError("Unexpected recording fixture format")
    struct.pack_into("<I", template, 16, seed)
    frames = bytearray()
    for tick in range(samples):
        sample = bytearray(template[24:104])
        # Leave the recording's RNG metadata intact while authoring both pads.
        struct.pack_into("<Hbb", sample, 8, 0, 0, 0)
        struct.pack_into("<Hbb", sample, 24, 0, 0, 0)
        # The ready countdown suppresses input before sample 100. Turn first,
        # then start the special after returning to an actionable state.
        if facing == -1 and 100 <= tick < 110:
            struct.pack_into("<Hbb", sample, 8, 0, -50, 0)
        if 130 <= tick < 130 + charge:
            y = 100 if special == "egg" and tick == 130 else 0
            if special == "pound": y = -100
            # A diagonal first input can select Egg Roll before Egg Throw.
            x = 0 if special == "egg" and tick == 130 else aim
            struct.pack_into("<Hbb", sample, 8, 1 << 9, x, y)
        frames.extend(sample)
    return template[:24] + frames


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--case", action="append")
    parser.add_argument("--samples", type=int, default=480)
    parser.add_argument("--seed", type=lambda value: int(value, 0), default=0x4E082EBA)
    args = parser.parse_args()
    if args.samples < 200:
        parser.error("At least 200 samples are required to exercise release/landing")
    cases = [
        ("egg-ground-tap", "egg", 1, 0, 1, None),
        ("egg-ground-charge", "egg", 28, 0, 1, None),
        ("egg-ground-aim-forward", "egg", 28, 80, 1, None),
        ("egg-ground-aim-backward", "egg", 28, -80, 1, None),
        ("egg-ground-facing-left", "egg", 28, -80, -1, None),
        ("egg-air-tap", "egg", 1, 0, 1, "1,120,-40,70"),
        ("egg-air-charge", "egg", 28, 80, 1, "1,120,-40,70"),
        ("pound-ground", "pound", 1, 0, 1, None),
        ("pound-air", "pound", 1, 0, 1, "1,120,-40,70"),
        ("pound-air-facing-left", "pound", 1, 0, -1, "1,120,-40,70"),
    ]
    if args.case:
        missing = set(args.case) - {case[0] for case in cases}
        if missing:
            parser.error("Unknown case: " + ", ".join(sorted(missing)))
        cases = [case for case in cases if case[0] in args.case]
    batch = args.out_dir.resolve()
    batch.mkdir(parents=True, exist_ok=False)
    project, store, source, image = (batch / name for name in
                                    ("project", "store", "owned.fms", "portrait.png"))
    tool = str(ROOT / "build/native/native_content_tool.exe")
    iso = args.iso.resolve()
    portrait(image)
    run(tool, "clone-stock", iso, "Yoshi", project, "Yoshi special comparison")
    run(tool, "import-parameters", project, ROOT / "examples/yoshi_wip.fms", source)
    run(tool, "script-build", project, source)
    package = bundled_id(run(tool, "--root", store, "bundle-project", project, image, iso))
    run(tool, "--root", store, "accept", package, iso)
    (batch / "package.txt").write_text(package + "\n")
    results = []
    for name, special, charge, aim, facing, airborne in cases:
        inputs = batch / (name + ".fmr")
        inputs.write_bytes(recording(args.seed, args.samples, special, charge, aim, facing))
        command = [sys.executable, str(ROOT / "tools/run_fighter_parity.py"),
                   "--iso", str(iso), "--inputs", str(inputs),
                   "--stock-fighter", "yoshi", "--package-id", package,
                   "--content-root", str(store), "--out-dir", str(batch / name),
                   "--p2", "human:zelda", "--expect-item-kind",
                   str(EGG if special == "egg" else STAR)]
        command += ["--expect-motion", str(365 if airborne and special == "egg"
                                           else 364 if special == "egg"
                                           else 368 if airborne else 366)]
        if special == "pound":
            command += ["--expect-motion", "367"]
        if airborne:
            command += ["--fixture-airborne", airborne]
        # run_fighter_parity uses --hidden --fast and retains drawing by default.
        code = subprocess.run(command, cwd=ROOT, check=False).returncode
        motion = 365 if airborne and special == "egg" else 364 if special == "egg" else 368 if airborne else 366
        expected_facing = "bf800000" if facing < 0 else "3f800000"
        for variant in ("stock", "clone"):
            trace = (batch / name / (name + "-" + variant + ".log")).read_text()
            first_move = re.search(r"^\[fighter\] sample \d+ P1 motion=" + str(motion) +
                                   r"\b.*? facing=([0-9a-f]+)", trace, re.MULTILINE)
            if first_move is None or first_move.group(1) != expected_facing:
                print(name + ": " + variant + " did not enter with the intended facing")
                code = 1
        results.append(dict(case=name, package=package, seed=args.seed,
                            samples=args.samples, exit=code))
        (batch / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    return int(any(row["exit"] for row in results))


if __name__ == "__main__":
    raise SystemExit(main())
