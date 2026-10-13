"""Exercise authored Egg Roll startup, travel, turns, stops and end states."""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from fighter_script_bundle_roundtrip import bundled_id, portrait, run

ROOT = Path(__file__).resolve().parents[2]


def recording(seed, samples, direction, turn, stop):
    data = bytearray((ROOT / "tests/native/parity_inputs/"
                      "drmario-special-neutral-ground-inputs.fmr").read_bytes())
    if data[:8] != b"FMREPLAY" or len(data) < 104:
        raise AssertionError("Unexpected recording fixture format")
    struct.pack_into("<I", data, 16, seed)
    frames = bytearray()
    for tick in range(samples):
        sample = bytearray(data[24:104])
        struct.pack_into("<Hbb", sample, 8, 0, 0, 0)
        struct.pack_into("<Hbb", sample, 24, 0, 0, 0)
        if tick == 130:
            struct.pack_into("<Hbb", sample, 8, 1 << 9, 80 * direction, 0)
        if turn and 185 <= tick < 245:
            struct.pack_into("<Hbb", sample, 8, 0, -80 * direction, 0)
        if stop and tick == stop:
            struct.pack_into("<Hbb", sample, 8, 1 << 9, 0, 0)
        frames.extend(sample)
    return data[:24] + frames


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--case", action="append")
    parser.add_argument("--samples", type=int, default=600)
    parser.add_argument("--seed", type=lambda value: int(value, 0), default=0x4E082EBA)
    args = parser.parse_args()
    if args.samples < 300:
        parser.error("At least 300 samples are required")
    cases = [
        ("roll-ground-right", 1, False, 0, None, [360, 361, 357]),
        ("roll-ground-left", -1, False, 0, None, [360, 361, 357]),
        ("roll-ground-turn", 1, True, 0, None, [360, 357, 358]),
        ("roll-ground-stop", 1, False, 210, None, [360, 357, 359]),
        ("roll-air-right", 1, False, 0, "1,120,-40,150", [360, 361, 362]),
        ("roll-air-left", -1, False, 0, "1,120,40,150", [360, 361, 362]),
        ("roll-air-stop", 1, False, 235, "1,120,-40,230", [360, 361, 363]),
        ("roll-ground-turn-stop", 1, True, 235, None, [360, 357, 358, 359]),
    ]
    if args.case:
        unknown = set(args.case) - {case[0] for case in cases}
        if unknown: parser.error("Unknown case: " + ", ".join(sorted(unknown)))
        cases = [case for case in cases if case[0] in args.case]
    batch = args.out_dir.resolve()
    batch.mkdir(parents=True, exist_ok=False)
    project, store, source, image = (batch / name for name in
                                   ("project", "store", "owned.fms", "portrait.png"))
    tool = ROOT / "build/native/native_content_tool.exe"
    iso = args.iso.resolve()
    portrait(image)
    run(tool, "clone-stock", iso, "Yoshi", project, "Yoshi Egg Roll comparison")
    run(tool, "import-parameters", project, ROOT / "examples/yoshi_wip.fms", source)
    run(tool, "script-build", project, source)
    package = bundled_id(run(tool, "--root", store, "bundle-project", project, image, iso))
    run(tool, "--root", store, "accept", package, iso)
    (batch / "package.txt").write_text(package + "\n")
    results = []
    for name, direction, turn, stop, airborne, motions in cases:
        inputs = batch / (name + ".fmr")
        inputs.write_bytes(recording(args.seed, args.samples, direction, turn, stop))
        command = [sys.executable, str(ROOT / "tools/run_fighter_parity.py"),
                   "--iso", str(iso), "--inputs", str(inputs),
                   "--stock-fighter", "yoshi", "--package-id", package,
                   "--content-root", str(store), "--out-dir", str(batch / name),
                   "--p2", "human:zelda"]
        for motion in motions: command += ["--expect-motion", str(motion)]
        if airborne: command += ["--fixture-airborne", airborne]
        code = subprocess.run(command, cwd=ROOT, check=False).returncode
        for variant in ("stock", "clone"):
            trace = (batch / name / (name + "-" + variant + ".log")).read_text()
            entry = re.search(r"^\[fighter\] sample \d+ P1 motion=360\b.*? facing=([0-9a-f]+)",
                              trace, re.MULTILINE)
            expected = "bf800000" if direction < 0 else "3f800000"
            if entry is None or entry.group(1) != expected:
                print(name + ": " + variant + " did not start with intended facing")
                code = 1
        results.append(dict(case=name, package=package, samples=args.samples,
                            seed=args.seed, exit=code))
        (batch / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    return int(any(row["exit"] for row in results))


if __name__ == "__main__":
    raise SystemExit(main())
