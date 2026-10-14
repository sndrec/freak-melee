"""Run a finite, muted gameplay exercise of the installed Glim package."""

import argparse
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--package", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    frames = [(0, 0, 0) for _ in range(1500)]

    def input_at(first, last, buttons=0, x=0, y=0):
        for frame in range(first, last):
            frames[frame] = buttons, x, y

    # Neutral pulls at several charge lengths, both-facing dash/recovery,
    # reflector window, and two aerial jumps followed by authored aerials.
    input_at(100, 130, 0x200)
    input_at(220, 222, 0x200)
    input_at(330, 363, x=80)
    input_at(420, 450, 0x200)
    input_at(550, 552, 0x200, x=80)
    input_at(640, 642, 0x200, x=-80)
    input_at(750, 752, 0x200, y=80)
    input_at(950, 952, 0x200, y=-80)
    input_at(1060, 1062, 0x400)
    input_at(1075, 1077, 0x400)
    input_at(1090, 1092, 0x400)
    input_at(1120, 1122, 0x100)
    input_at(1280, 1282, 0x400)
    input_at(1295, 1297, 0x400)
    input_at(1310, 1312, 0x100, y=-70)
    data = bytearray(struct.pack("<8sQII", b"FMREPLAY", 0, 0x1740A19, 1))
    for buttons, x, y in frames:
        pad = struct.pack("<HbbbbBBBBbBI", buttons, x, y, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        data.extend(struct.pack("<Q", 0) + pad * 1 + bytes(48) + bytes(8))
    recording = output / "inputs.fmr"
    recording.write_bytes(data)
    command = [
        str(ROOT / "melee/build/native/melee_native.exe"),
        "--iso",
        str(args.iso.resolve()),
        "--match",
        "--stage",
        "practice",
        "--p1",
        "human:custom",
        "--p1-fighter",
        args.package,
        "--p2",
        "off",
        "--p3",
        "off",
        "--p4",
        "off",
        "--mode",
        "stock",
        "--stocks",
        "9",
        "--time",
        "0",
        "--replay-inputs",
        str(recording),
        "--trace-actions",
        "--trace-fighter-state",
        "--trace-geometry-frame",
        "456",
        "--fast",
        "--hidden",
        "--mute",
    ]
    with (output / "stdout.log").open("w") as stdout, (output / "stderr.log").open(
        "w"
    ) as stderr:
        result = subprocess.run(
            command, cwd=output, stdout=stdout, stderr=stderr, timeout=180
        )
    log = (output / "stderr.log").read_text(errors="replace")
    transitions = re.findall(
        r"\[action\] sample (\d+) P1 .*?motion=(\d+).*?frame=([^\s]+)", log
    )
    motions = sorted({int(motion) for _, motion, _ in transitions})
    checks = {
        "charge": 346 in motions,
        "release": 348 in motions,
        "skip": 364 in motions,
        "updraft": 366 in motions,
        "ward": 369 in motions or 370 in motions,
        "aerial_jump": 341 in motions,
        "second_jump": 342 in motions,
        "neutral_air": 65 in motions,
        "down_air": 69 in motions,
    }
    receipt = dict(
        exit_code=result.returncode,
        checks=checks,
        motions=motions,
        transitions=transitions,
        finite_replay="End after 1500 controller samples" in log,
    )
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt))
    if result.returncode or not receipt["finite_replay"] or not all(checks.values()):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
