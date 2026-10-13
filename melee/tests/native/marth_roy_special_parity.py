"""Compare packaged Marth/Roy specials to stock using real ticks.

Run manually with --iso. The output directory keeps the generated recordings,
owned parameter files, packages, and stock/clone traces for reproduction.
Rendering remains enabled; --fast only removes real-time pacing.
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
sys.path.insert(0, str(ROOT / "tools"))
from compare_fighter_parity import read_trace


def charge_recording(path, mode, direction, duration):
    baseline = (INPUTS / "drmario-special-neutral-ground-inputs.fmr").read_bytes()
    header = baseline[:24]
    neutral = baseline[24:104]
    frames = bytearray()
    for frame in range(500):
        sample = bytearray(neutral)
        buttons, x, y = 0, 0, 0
        if direction == "left" and 100 <= frame < 102:
            x = -40
        if mode == "air" and frame == 150:
            buttons = 1 << 10
        start = 170 if mode == "air" else 150
        if start <= frame < start + duration:
            buttons |= 1 << 9
        struct.pack_into("<Hbb", sample, 8, buttons, x, y)
        frames.extend(sample)
    path.write_bytes(header + frames)


def contact_recording(path, up, face_left):
    name = "drmario-up-contact" if up else "drmario-neutral-air-contact"
    recording = bytearray((INPUTS / (name + "-inputs.fmr")).read_bytes())
    # Reuse only the approach and final special from the Doctor recording;
    # its taunt and preliminary pill do not belong to this comparison.
    for frame in (170, 240):
        struct.pack_into("<H", recording, 24 + frame * 80 + 8, 0)
    # Different run speeds/body collisions place the fighters differently.
    # Marth passes P2; Roy stays to P2's left. Face the actual target.
    if face_left:
        for frame in range(230, 232):
            struct.pack_into("<b", recording, 24 + frame * 80 + 10, -40)
    path.write_bytes(recording)


def chain_recording(path, mode, route, early=False, attack=False):
    baseline = (INPUTS / "drmario-special-neutral-ground-inputs.fmr").read_bytes()
    frames = bytearray()
    start = 180 if mode == "air" else 150
    # Use the authored command windows before the air chain lands. Ground
    # routes retain the slower taps to cover the later portions of each window.
    offsets = (0, 9, 26, 49 if route == "low" else 44) if mode == "air" else (0, 14, 34, 57)
    taps = tuple(start + offset for offset in offsets)
    if early: taps = (start, start + 2, *taps[1:])
    for frame in range(330):
        sample = bytearray(baseline[24:104])
        buttons, x, y = 0, 0, 0
        # Roy's third low phase opens at frame 22. Full hop and delay the
        # double jump on this route to reach phase four before landing.
        if mode == "air" and (frame in (150, 165 if route == "low" else 157) or
                              (route == "low" and 150 <= frame <= 156)):
            buttons = 1 << 10
        if frame in taps:
            buttons = 1 << (8 if attack and frame != start else 9)
            x = 80 if frame == start else 0
            y = 0 if frame == start else {"high": 80, "neutral": 0, "low": -80}[route]
        if any(tap - 3 <= frame <= tap for tap in taps[1:]):
            y = {"high": 80, "neutral": 0, "low": -80}[route]
        struct.pack_into("<Hbb", sample, 8, buttons, x, y)
        frames.extend(sample)
    path.write_bytes(baseline[:24] + frames)


def counter_recording(path, mode, contact=False):
    baseline = (INPUTS / "drmario-special-neutral-ground-inputs.fmr").read_bytes()
    frames = bytearray()
    start = 170 if mode == "air" else 150
    for frame in range(330):
        sample = bytearray(baseline[24:104])
        buttons, y = 0, 0
        if mode == "air" and frame == 150: buttons = 1 << 10
        if frame == start: buttons, y = 1 << 9, -80
        struct.pack_into("<Hbb", sample, 8, buttons, 0, y)
        if contact:
            # Fox faces the counter and fires a real laser across the stage.
            x2 = -40 if 100 <= frame < 102 else 0
            b2 = 1 << 9 if frame == (160 if mode == "air" else 135) else 0
            struct.pack_into("<Hbb", sample, 24, b2, x2, 0)
        frames.extend(sample)
    path.write_bytes(baseline[:24] + frames)


def melee_counter_recording(path, fighter):
    baseline = (INPUTS / "drmario-special-neutral-ground-inputs.fmr").read_bytes()
    frames = bytearray()
    for frame in range(330):
        sample = bytearray(baseline[24:104])
        # Finish RunBrake before attempting down-B; it cannot interrupt that
        # motion. Approach lengths account for the fighters' different speeds.
        b1 = 1 << 9 if frame == 180 else 0
        x1 = 100 if 85 <= frame < (130 if fighter == "Marth" else 135) else 0
        y1 = -80 if frame == 180 else 0
        struct.pack_into("<Hbb", sample, 8, b1, x1, y1)
        x2 = -40 if 130 <= frame < 132 else 0
        b2 = 1 << 8 if frame == 180 else 0
        if frame == 180: x2 = -40
        struct.pack_into("<Hbb", sample, 24, b2, x2, 0)
        frames.extend(sample)
    path.write_bytes(baseline[:24] + frames)


def common_recording(path, case):
    baseline = (INPUTS / "drmario-special-neutral-ground-inputs.fmr").read_bytes()
    frames = bytearray()
    for frame in range(360):
        sample = bytearray(baseline[24:104])
        buttons, trigger = 0, 0
        if case == "jab" and frame in (150, 159, 168, 177, 186):
            buttons = 1 << 8
        if case in ("shield", "powershield") and 150 <= frame < 190:
            trigger = 200 if case == "shield" else 255
            if case == "powershield": buttons = 1 << 6
        if case == "wind" and 150 <= frame < 230:
            buttons = 1 << 9
        struct.pack_into("<Hbb", sample, 8, buttons, 0, 0)
        sample[14] = trigger
        frames.extend(sample)
    path.write_bytes(baseline[:24] + frames)


def peach_counter_recording(path, fighter="Marth"):
    baseline = (INPUTS / "drmario-special-neutral-ground-inputs.fmr").read_bytes()
    frames = bytearray()
    for frame in range(400):
        sample = bytearray(baseline[24:104])
        b1 = 1 << 9 if frame == 170 else 0
        x1 = 100 if 85 <= frame < (113 if fighter == "Marth" else 120) else 0
        y1 = -80 if frame == 170 else 0
        struct.pack_into("<Hbb", sample, 8, b1, x1, y1)
        b2, x2 = (1 << 9, -80) if frame == 150 else (0, 0)
        struct.pack_into("<Hbb", sample, 24, b2, x2, 0)
        frames.extend(sample)
    path.write_bytes(baseline[:24] + frames)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--fighter", choices=("Marth", "Roy"))
    parser.add_argument("--case", help="Run a single case by name")
    parser.add_argument("--case-prefix", help="Run cases matching a name prefix")
    parser.add_argument("--category", choices=("neutral", "up", "contact", "side", "counter", "common", "cpu", "cross"))
    parser.add_argument("--out-dir", type=Path, default=ROOT / ".local" /
                        ("sword-parity-" + datetime.datetime.now().strftime("%Y%m%d-%H%M%S")))
    args = parser.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=False)
    image = args.out_dir / "portrait.png"
    portrait(image)
    tool = str(ROOT / "build/native/native_content_tool.exe")
    cases = []
    for mode in ("ground", "air"):
        for direction in ("right", "left"):
            for charge, duration in (("short", 1), ("partial", 60), ("full", 300)):
                name = f"neutral-{mode}-{direction}-{charge}"
                recording = args.out_dir / (name + ".fmr")
                charge_recording(recording, mode, direction, duration)
                # Air charges can land before release; require the entry and
                # charge phase, and full-charge release on at least one side.
                motions = [345] if mode == "air" else [341]
                if charge != "short":
                    motions.append(346 if mode == "air" else 342)
                if mode == "ground":
                    motions.append(344 if charge == "full" else 343)
                cases.append((name, recording, motions, []))
    for mode in ("ground", "air"):
        for direction in ("default", "up", "left", "right", "diagonal"):
            name = f"up-{mode}-{direction}"
            recording = INPUTS / f"fighter-fire-{mode}-{direction}-inputs.fmr"
            if recording.exists():
                cases.append((name, recording, [368 if mode == "air" else 367], []))
    for up in (False, True):
        name = "up-contact" if up else "neutral-contact"
        recording = args.out_dir / (name + ".fmr")
        contact_recording(recording, up, True)
        cases.append((name, recording, [367] if up else [345, 347],
                      ["--expect-damage-port", "2"]))
    for mode in ("ground", "air"):
        for route in ("high", "neutral", "low"):
            name = f"side-{mode}-{route}"
            recording = args.out_dir / (name + ".fmr")
            chain_recording(recording, mode, route)
            expected = {"high": [350, 352, 355], "neutral": [351, 353, 356],
                        "low": [351, 354, 357]}[route]
            if mode == "air": expected = [motion + 9 for motion in expected]
            cases.append((name, recording, [358 if mode == "air" else 349, *expected], []))
        for route, early, attack in (("early", True, False), ("attack", False, True)):
            name = f"side-{mode}-{route}"
            recording = args.out_dir / (name + ".fmr")
            chain_recording(recording, mode, "neutral", early, attack)
            expected = [358 if mode == "air" else 349]
            if not early: expected += [360, 362, 365] if mode == "air" else [351, 353, 356]
            cases.append((name, recording, expected, []))
        for contact in (False, True):
            name = f"counter-{mode}-{'laser' if contact else 'whiff'}"
            recording = args.out_dir / (name + ".fmr")
            counter_recording(recording, mode, contact)
            motions = [371 if mode == "air" else 369]
            if contact: motions.append(370)
            cases.append((name, recording, motions, ["--p2", "human:fox"] if contact else []))
    recording = args.out_dir / "counter-ground-melee.fmr"
    melee_counter_recording(recording, "Marth")
    cases.append(("counter-ground-melee", recording, [369, 370],
                  ["--expect-damage-port", "2"]))
    for case, motions in (("jab", [44, 45]), ("shield", [178, 179, 180]),
                          ("powershield", [182, 179, 180]), ("wind", [341, 342, 343])):
        name = "common-" + case
        recording = args.out_dir / (name + ".fmr")
        common_recording(recording, case)
        # Corneria installs global cloth wind immediately. Charge effects use
        # local wind and do not enter that global override branch.
        cases.append((name, recording, motions, ["--stage", "corneria"] if case == "wind" else []))
    recording = args.out_dir / "cross-peach-counter.fmr"
    peach_counter_recording(recording)
    cases.append(("cross-peach-counter", recording, [369],
                  ["--p2", "human:peach", "--expect-port-motion", "2", "359"]))
    baseline = (INPUTS / "drmario-special-neutral-ground-inputs.fmr").read_bytes()
    for stage in ("final-destination", "battlefield"):
        name = "cpu-" + stage
        recording = args.out_dir / (name + ".fmr")
        recording.write_bytes(baseline[:24] + baseline[24:104] * 2400)
        cases.append((name, recording, [], ["--p1-type", "cpu", "--cpu-level", "9",
                      "--p2", "human:zelda", "--stocks", "4", "--stage", stage,
                      "--expect-damage-port", "2"]))
    if args.category:
        cases = [case for case in cases if
                 (case[0].endswith("-contact") if args.category == "contact" else
                  case[0].startswith(args.category + "-"))]
    if args.case_prefix:
        cases = [case for case in cases if case[0].startswith(args.case_prefix)]
    if args.case:
        cases = [case for case in cases if case[0] == args.case]
        if not cases:
            parser.error("unknown case")
    results = []
    for fighter in ((args.fighter,) if args.fighter else ("Marth", "Roy")):
        project = args.out_dir / fighter
        store = args.out_dir / (fighter + "-store")
        owned = args.out_dir / (fighter + "-owned.fms")
        run(tool, "clone-stock", args.iso, fighter, project, fighter + " Script Parity")
        run(tool, "import-parameters", project,
            ROOT / f"examples/{fighter.lower()}_wip.fms", owned)
        run(tool, "script-build", project, owned)
        package = bundled_id(run(tool, "--root", store, "bundle-project",
                                 project, image, args.iso))
        run(tool, "--root", store, "accept", package, args.iso)
        for name, recording, motions, extra in cases:
            if name == "common-jab" and fighter == "Roy":
                # Stock Roy has a single jab; he never requests jab 2/3.
                motions = [44]
            if name in ("neutral-contact", "up-contact"):
                recording = args.out_dir / (fighter + "-" + name + ".fmr")
                contact_recording(recording, name == "up-contact", fighter == "Marth")
            if name == "counter-ground-melee":
                recording = args.out_dir / (fighter + "-" + name + ".fmr")
                melee_counter_recording(recording, fighter)
            if name == "cross-peach-counter":
                recording = args.out_dir / (fighter + "-" + name + ".fmr")
                peach_counter_recording(recording, fighter)
            print(f"Comparing {fighter}: {name}", flush=True)
            command = [sys.executable, str(ROOT / "tools/run_fighter_parity.py"),
                       "--iso", str(args.iso.resolve()), "--inputs", str(recording.resolve()),
                       "--stock-fighter", fighter.lower(), "--package-id", package,
                       "--content-root", str(store.resolve()), "--stocks", "2",
                       "--out-dir", str(args.out_dir / (fighter + "-" + name)), *extra]
            for motion in motions:
                command.extend(("--expect-motion", str(motion)))
            result = subprocess.run(command, cwd=ROOT)
            exit_code = result.returncode
            if name.startswith("neutral-") and name != "neutral-contact":
                expected_facing = "bf800000" if "-left-" in name else "3f800000"
                traces = args.out_dir / (fighter + "-" + name)
                for label in ("stock", "clone"):
                    log = traces / (recording.stem + "-" + label + ".log")
                    states, _, _ = read_trace(log, {1}, {1})
                    if not any(re.search(r"(?:^| )motion=(?:341|345)(?: |$)", state)
                               and "facing=" + expected_facing in state
                               for _, _, state in states["fighter"]):
                        print(f"Expected {name} facing was not reached in {label}", flush=True)
                        exit_code = 1
            if name.startswith("side-") and name.endswith("-early"):
                traces = args.out_dir / (fighter + "-" + name)
                for label in ("stock", "clone"):
                    log = traces / (recording.stem + "-" + label + ".log")
                    states, _, _ = read_trace(log, {1}, {1})
                    if any(re.search(r"(?:^| )motion=(?:35[0-7]|36[0-6]|359)(?: |$)", state)
                           for _, _, state in states["fighter"]):
                        print(f"Early input incorrectly advanced the chain in {label}", flush=True)
                        exit_code = 1
            if name == "common-jab" and fighter == "Marth":
                traces = args.out_dir / (fighter + "-" + name)
                for label in ("stock", "clone"):
                    log = traces / (recording.stem + "-" + label + ".log")
                    states, _, _ = read_trace(log, {1}, {1})
                    saw_second = False
                    restarted = False
                    for _, _, state in states["fighter"]:
                        motion = int(re.search(r"(?:^| )motion=(\d+)", state)[1])
                        if motion == 45: saw_second = True
                        if motion == 44 and saw_second: restarted = True
                    if not restarted:
                        print(f"Jab restart was not reached in {label}", flush=True)
                        exit_code = 1
            if name == "common-wind":
                traces = args.out_dir / (fighter + "-" + name)
                for label in ("stock", "clone"):
                    log = traces / (recording.stem + "-" + label + ".log")
                    if not re.search(r"(?:^| )wind=[12](?: |$)", log.read_text(errors="replace"), re.M):
                        print(f"Wind activation was not reached in {label}", flush=True)
                        exit_code = 1
            if name == "cross-peach-counter":
                traces = args.out_dir / (fighter + "-" + name)
                for label in ("stock", "clone"):
                    log = traces / (recording.stem + "-" + label + ".log")
                    if not re.search(r"^\[body-contact\] sample \d+ P2 motion=360 other=P1 other_motion=(?:369|371)$",
                                     log.read_text(errors="replace"), re.M):
                        print(f"Body attack interception was not reached in {label}", flush=True)
                        exit_code = 1
            results.append(dict(fighter=fighter, case=name, package=package,
                                exit=exit_code))
            (args.out_dir / "summary.json").write_text(json.dumps(results, indent=2))
    passed = sum(result["exit"] == 0 for result in results)
    print(f"Passed {passed}/{len(results)} sword-special comparisons", flush=True)
    return int(passed != len(results))


if __name__ == "__main__":
    raise SystemExit(main())
