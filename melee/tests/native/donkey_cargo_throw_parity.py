"""Compare authored cargo acquisition, locomotion and throws to stock.

Hidden runs keep drawing enabled. Intended states, facing, victim damage and
follow-up coverage are required; equality alone cannot accept an untested path.
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


def recording(path, throw, mode, facing):
    baseline = (ROOT / "tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr").read_bytes()
    frames = bytearray()
    release = 360 if mode == "air-landing" else 310 if facing == "left" else 280
    for tick in range(650):
        sample = bytearray(baseline[24:104])
        buttons, x, y = 0, 0, 0
        if 85 <= tick < 147: x = 100
        if tick == 200: buttons = 16  # Z: acquire the nearby fighter.
        if 210 <= tick < 215: x = 100  # Stock forward throw enters cargo hold.
        if facing == "left" and 245 <= tick < 247: x = -100
        jump_lead = 40 if mode == "air-landing" else 25
        if mode != "ground" and release - jump_lead <= tick < release - jump_lead + 8:
            buttons = 1 << 10
        if tick == release:
            buttons = 1 << 8
            direction = -1 if facing == "left" else 1
            if throw == "forward": x = 100 * direction
            elif throw == "back": x = -100 * direction
            elif throw == "up": y = 100
            else: y = -100
        struct.pack_into("<Hbb", sample, 8, buttons, x, y)
        frames.extend(sample)
    path.write_bytes(baseline[:24] + frames)


def covered(directory, name, motion, facing, mode):
    direction = "bf800000" if facing == "left" else "3f800000"
    for label in ("stock", "clone"):
        text = (directory / f"{name}-{label}.log").read_text(errors="replace")
        states = re.findall(r"^\[fighter\] sample \d+ P1 (.*)$", text, re.M)
        if not any(s.startswith(f"motion={motion} ") and "facing=" + direction in s for s in states):
            print(f"Missing throw entry/facing: {name} {label}")
            return False
        entry = next(i for i, s in enumerate(states) if s.startswith(f"motion={motion} "))
        followup = states[entry + 1:]
        if mode != "ground" and not any(s.startswith((f"motion={motion - 4} ", "motion=29 "))
                                        for s in followup):
            print(f"Missing aerial throw landing/finish: {name} {label}")
            return False
        if mode == "air-landing" and not any(s.startswith(f"motion={motion - 4} ") for s in followup):
            print(f"Missing cargo throw landing transition: {name} {label}")
            return False
        if not any(s.startswith("motion=14 ") for s in states[-200:]):
            print(f"Missing return to idle: {name} {label}")
            return False
    return True



# Carry paths use the same recorded acquisition as the throw matrix. They
# deliberately spend time in walking/jump/drop states before the final throw.
CARRY_CASES = {
    "walk-variants": ("final-destination", (352, 353, 354, 363)),
    "turn-walk-left": ("final-destination", (355, 353, 363)),
    "full-jump": ("final-destination", (356, 358, 359, 363)),
    "short-jump": ("final-destination", (356, 358, 359, 363)),
    "tap-jump": ("final-destination", (356, 358, 359, 363)),
    "tap-short-jump": ("final-destination", (356, 358, 359, 363)),
    "platform-drop": ("battlefield", (356, 358, 359, 357, 363)),
    "ledge-fall": ("final-destination", (354, 357, 367)),
}


def carry_recording(path, scenario):
    baseline = (ROOT / "tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr").read_bytes()
    frames = bytearray()
    throw_tick = 600 if scenario == "platform-drop" else 315 if scenario == "ledge-fall" else 365
    for tick in range(700):
        sample = bytearray(baseline[24:104])
        buttons, x, y = 0, 0, 0
        if scenario != "platform-drop" and 85 <= tick < 147: x = 100
        if tick == 200: buttons = 16
        if 210 <= tick < 215: x = 100
        if scenario == "walk-variants":
            if 245 <= tick < 263: x = 35
            elif 263 <= tick < 273: x = 55
            elif 273 <= tick < 284: x = 100
        if scenario == "turn-walk-left":
            if 245 <= tick < 247: x = -100
            if 280 <= tick < 305: x = -55
        if scenario == "full-jump" and 260 <= tick < 268: buttons = 1 << 10
        if scenario == "short-jump" and tick == 260: buttons = 1 << 10
        if scenario == "tap-jump" and 260 <= tick < 268: y = 100
        if scenario == "tap-short-jump" and tick == 260: y = 100
        if scenario == "platform-drop":
            # Battlefield starts Zelda on its top platform. Jump onto it
            # before acquiring her, then jump while carrying and drop through.
            buttons, x, y = 0, 0, 0
            if 110 <= tick < 118 or 140 <= tick < 148: buttons = 1 << 10
            if tick == 260: buttons = 16
            if 270 <= tick < 275: x = 100
            if 350 <= tick < 358: buttons = 1 << 10
            if 500 <= tick < 506: y = -100
        if scenario == "ledge-fall" and 245 <= tick < 309: x = 100
        if tick == throw_tick: buttons, x, y = 1 << 8, 0, 100
        struct.pack_into("<Hbb", sample, 8, buttons, x, y)
        frames.extend(sample)
    path.write_bytes(baseline[:24] + frames)


def carry_covered(directory, name, scenario):
    for label in ("stock", "clone"):
        text = (directory / f"{name}-{label}.log").read_text(errors="replace")
        states = re.findall(r"^\[fighter\] sample (\d+) P1 (.*)$", text, re.M)
        if scenario == "turn-walk-left" and not any(s.startswith("motion=363 ") and "facing=bf800000" in s for _, s in states):
            print(f"Carry turnaround did not persist into throw: {name} {label}")
            return False
        if scenario == "platform-drop":
            drop = next((i for i, (t, s) in enumerate(states) if int(t) >= 500 and s.startswith("motion=357 ")), None)
            if drop is None or drop == 0: return False
            previous = states[drop - 1][1]
            y_hex = re.search(r"pos=[a-f0-9]+,([a-f0-9]+),", previous)
            height = struct.unpack("!f", bytes.fromhex(y_hex[1]))[0] if y_hex else 0
            if height <= 5 or not previous.startswith("motion=351 "):
                print(f"Drop did not start on an elevated held platform: {name} {label}")
                return False
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    parser.add_argument("--case")
    parser.add_argument("--group", choices=("all", "throws", "locomotion"), default="all")
    parser.add_argument("--out-dir", type=Path, default=ROOT / ".local" /
                        ("donkey-cargo-" + datetime.datetime.now().strftime("%Y%m%d-%H%M%S")))
    args = parser.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=False)
    image = args.out_dir / "portrait.png"
    portrait(image)
    tool = str(ROOT / "build/native/native_content_tool.exe")
    project, store = args.out_dir / "project", args.out_dir / "store"
    source = args.out_dir / "owned.fms"
    run(tool, "clone-stock", args.iso, "Donkey Kong", project, "Donkey Cargo Parity")
    run(tool, "import-parameters", project, ROOT / "examples/donkey_wip.fms", source)
    run(tool, "script-build", project, source)
    package = bundled_id(run(tool, "--root", store, "bundle-project", project, image, args.iso))
    run(tool, "--root", store, "accept", package, args.iso)
    results = []
    for mode in (() if args.group == "locomotion" else ("ground", "air", "air-landing")):
        for facing in ("right", "left"):
            for offset, throw in enumerate(("forward", "back", "up", "down")):
                if mode == "air-landing" and throw != "up": continue
                name = f"cargo-{throw}-{mode}-{facing}"
                if args.case and args.case != name: continue
                motion = 361 + offset + (4 if mode != "ground" else 0)
                inputs = args.out_dir / (name + ".fmr")
                recording(inputs, throw, mode, facing)
                command = [sys.executable, str(ROOT / "tools/run_fighter_parity.py"),
                           "--iso", str(args.iso.resolve()), "--inputs", str(inputs.resolve()),
                           "--stock-fighter", "donkeykong", "--package-id", package,
                           "--content-root", str(store.resolve()), "--stocks", "2",
                           "--out-dir", str(args.out_dir / name), "--expect-motion", str(motion),
                           "--expect-damage-port", "2"]
                print("Comparing " + name, flush=True)
                result = subprocess.run(command, cwd=ROOT)
                exit_code = result.returncode
                if exit_code == 0 and not covered(args.out_dir / name, name, motion, facing, mode):
                    exit_code = 1
                results.append(dict(case=name, package=package, exit=exit_code))
    if args.group != "throws":
        for scenario, (stage, motions) in CARRY_CASES.items():
            name = "carry-" + scenario
            if args.case and args.case != name: continue
            inputs = args.out_dir / (name + ".fmr")
            carry_recording(inputs, scenario)
            command = [sys.executable, str(ROOT / "tools/run_fighter_parity.py"),
                       "--iso", str(args.iso.resolve()), "--inputs", str(inputs.resolve()),
                       "--stock-fighter", "donkeykong", "--package-id", package,
                       "--content-root", str(store.resolve()), "--stocks", "2", "--stage", stage,
                       "--out-dir", str(args.out_dir / name), "--expect-damage-port", "2"]
            for motion in motions: command.extend(("--expect-motion", str(motion)))
            print("Comparing " + name, flush=True)
            result = subprocess.run(command, cwd=ROOT)
            exit_code = result.returncode
            if exit_code == 0 and not carry_covered(args.out_dir / name, name, scenario): exit_code = 1
            results.append(dict(case=name, package=package, exit=exit_code))
    (args.out_dir / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    return 0 if results and all(r["exit"] == 0 for r in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
