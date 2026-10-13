"""Compare authored Donkey Kong specials with stock in hidden, drawn matches.

Requires the retail ISO. Keeps independent project/package data, inputs and
both traces in the output directory. This is selected-move acceptance, not a
claim that cargo or the complete Donkey Kong template is cloneable.
"""

import argparse
import datetime
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from fighter_script_bundle_roundtrip import run, portrait, bundled_id

ROOT = Path(__file__).resolve().parents[2]
INPUTS = ROOT / "tests/native/parity_inputs"


def recording(path, move, mode, direction, steer, contact=False, variant=""):
    baseline = (INPUTS / "drmario-special-neutral-ground-inputs.fmr").read_bytes()
    frames = bytearray()
    start = 170 if mode == "air" else 150
    if contact: start = 180
    for tick in range(1000 if move == "neutral" else 430):
        sample = bytearray(baseline[24:104])
        buttons, x, y = 0, 0, 0
        if direction == "left" and 100 <= tick < 102: x = -40
        # Hand Slap's capsules are narrower than Headbutt/Spinning Kong.
        approach_end = 147 if move == "down" else 141
        if contact and 85 <= tick < approach_end: x = 100
        if mode == "air" and 145 <= tick < 153: buttons = 1 << 10
        if tick == start:
            buttons = 1 << 9
            if move == "side": x = -80 if direction == "left" else 80
            elif move == "up": y = 80
            elif move == "down": y = -80
        if steer and start + 1 <= tick < start + 55:
            x = 80 if direction == "left" else -80
        if move == "neutral":
            if variant == "release" and tick == (200 if mode == "air" else start + 80): buttons = 1 << 9
            if variant in ("cancel", "damaged") and start + 70 <= tick < start + 72: buttons = 1 << 6
            if variant == "roll" and tick == start + 70: buttons, x = 1 << 6, 80
            if variant in ("full", "mixed") and tick == 650: buttons = 1 << 9
            if variant == "mixed" and tick == 500: buttons, y = 1 << 9, 80
            if variant in ("damaged", "active-damage") and tick == 410: buttons = 1 << 9
            if variant in ("damaged", "active-damage") and tick == 480: buttons = 1 << 9
        if move == "down" and variant == "repeat" and tick in (180, 200, 220, 240, 260):
            buttons = 1 << 9
        struct.pack_into("<Hbb", sample, 8, buttons, x, y)
        if (variant == "damaged" and tick == 300) or (variant == "active-damage" and tick == 220):
            struct.pack_into("<Hbb", sample, 24, 1 << 9, 0, 0)
        frames.extend(sample)
    path.write_bytes(baseline[:24] + frames)


def check_coverage(directory, name, move, mode, direction, steer, variant=""):
    motion = {"side": 379, "up": 381, "neutral": 369, "down": 383}[move]
    if mode == "air": motion += 5 if move == "neutral" else 1
    facing = "bf800000" if direction == "left" else "3f800000"
    for label in ("stock", "clone"):
        log = directory / (name + "-" + label + ".log")
        states = re.findall(r"^\[fighter\] sample \d+ P1 (.*)$",
                            log.read_text(errors="replace"), re.M)
        if not any(state.startswith(f"motion={motion} ") and
                   "facing=" + facing in state for state in states):
            print(f"Expected entry facing was not reached in {name} {label}")
            return False
        required = []
        if move == "neutral":
            required.append(375 if mode == "air" else 370)
            if variant in ("full", "mixed"): required.extend((373, 14))
            if variant == "mixed": required.append(381)
            if variant == "release": required.append(377 if mode == "air" else 372)
            if variant == "cancel": required.append(371)
            if variant == "roll": required.append(233)
            if variant == "active-damage": required.append(372)
        if move == "down": required.extend((384, 385, 14))
        if mode == "air" and move == "up": required.append(35)
        if mode == "air" and move == "up" and not steer: required.extend((43, 14))
        if mode == "ground" and move == "up" and direction == "right" and steer:
            required.append(382)
        for expected in required:
            if not any(state.startswith(f"motion={expected} ") for state in states):
                print(f"Expected follow-up motion {expected} absent in {name} {label}")
                return False
        if move == "down" and variant == "repeat":
            loop_frames = [struct.unpack("<f", bytes.fromhex(
                re.search(r" frame=([0-9a-f]{8})", state)[1])[::-1])[0]
                for state in states if state.startswith("motion=384 ")]
            restarts = sum(current < previous
                           for previous, current in zip(loop_frames, loop_frames[1:]))
            if restarts < 2:
                print(f"Hand Slap did not repeat at least twice in {name} {label}")
                return False
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    parser.add_argument("--case")
    parser.add_argument("--out-dir", type=Path, default=ROOT / ".local" /
                        ("donkey-parity-" + datetime.datetime.now().strftime("%Y%m%d-%H%M%S")))
    args = parser.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=False)
    image = args.out_dir / "portrait.png"
    portrait(image)
    tool = str(ROOT / "build/native/native_content_tool.exe")
    project, store = args.out_dir / "project", args.out_dir / "store"
    source = args.out_dir / "owned.fms"
    run(tool, "clone-stock", args.iso, "Donkey Kong", project, "Donkey Script Parity")
    run(tool, "import-parameters", project, ROOT / "examples/donkey_wip.fms", source)
    run(tool, "script-build", project, source)
    package = bundled_id(run(tool, "--root", store, "bundle-project", project, image, args.iso))
    run(tool, "--root", store, "accept", package, args.iso)
    results = []
    cases = []
    for move in ("side", "up"):
        for mode in ("ground", "air"):
            for direction in ("right", "left"):
                for steer in ((False, True) if move == "up" else (False,)):
                    name = f"{move}-{mode}-{direction}" + ("-steer" if steer else "")
                    cases.append((name, move, mode, direction, steer, False, "", "final-destination"))
        cases.append((move + "-ground-contact", move, "ground", "right", False, True, "", "final-destination"))
    for mode in ("ground", "air"):
        for direction in ("right", "left"):
            for variant in ("release", "full", "cancel"):
                cases.append((f"neutral-{mode}-{direction}-{variant}", "neutral", mode, direction, False, False, variant, "final-destination"))
    for variant in ("roll", "mixed", "release", "damaged", "active-damage"):
        cases.append(("neutral-ground-" + variant + ("-contact" if variant in ("release", "damaged", "active-damage") else ""), "neutral", "ground", "right", False, variant in ("release", "damaged", "active-damage"), variant, "final-destination"))
    for direction in ("right", "left"):
        for variant in ("", "repeat"):
            cases.append((f"down-ground-{direction}" + ("-" + variant if variant else ""), "down", "ground", direction, False, False, variant, "final-destination"))
    cases.append(("down-ground-contact", "down", "ground", "right", False, True, "", "final-destination"))
    cases.append(("down-ground-slope", "down", "ground", "right", False, False, "repeat", "yoshis-story"))
    for name, move, mode, direction, steer, contact, variant, stage in cases:
        if args.case and args.case != name: continue
        inputs = args.out_dir / (name + ".fmr")
        recording(inputs, move, mode, direction, steer, contact, variant)
        motion = {"side": 379, "up": 381, "neutral": 369, "down": 383}[move]
        if mode == "air": motion += 5 if move == "neutral" else 1
        command = [sys.executable, str(ROOT / "tools/run_fighter_parity.py"),
                   "--iso", str(args.iso.resolve()), "--inputs", str(inputs.resolve()),
                   "--stock-fighter", "donkeykong", "--package-id", package,
                   "--content-root", str(store.resolve()), "--stocks", "2",
                   "--out-dir", str(args.out_dir / name), "--expect-motion", str(motion), "--stage", stage]
        if mode == "air" and move == "side": command.extend(("--expect-motion", "379"))
        if contact: command.extend(("--expect-damage-port", "1" if variant in ("damaged", "active-damage") else "2"))
        print("Comparing Donkey Kong: " + name, flush=True)
        result = subprocess.run(command, cwd=ROOT)
        exit_code = result.returncode
        if exit_code == 0 and not check_coverage(args.out_dir / name, name, move,
                                                 mode, direction, steer, variant):
            exit_code = 1
        results.append(dict(case=name, package=package, exit=exit_code))
    (args.out_dir / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    return 0 if results and all(result["exit"] == 0 for result in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
