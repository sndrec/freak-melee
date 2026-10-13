"""Compare scripted heavy-item carrying to stock in hidden, drawn runs.

Both clients receive the same offline common-item fixture and controller input.
Every accepted case must actually pick up the barrel and enter its target states.
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


def motion_ids():
    text = (ROOT / "src/melee/ft/kinds/ftCommon/forward.h").read_text()
    block = text.split("typedef enum ftCommon_MotionState {")[1].split("}")[0]
    value, result = -2, {}
    for entry in block.split(","):
        name = entry.strip().split("=")[0].strip()
        if not name: continue
        value = int(entry.split("=")[1]) if "=" in entry else value + 1
        result[name.removeprefix("ftCo_MS_")] = value
    return result


def recording(path, case):
    source = (ROOT / "tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr").read_bytes()
    samples = bytearray()
    for tick in range(3000 if case in ("giant-walk", "tiny-walk") else 700):
        sample = bytearray(source[24:104])
        buttons, x, y = 0, 0, 0
        if tick == 220: buttons = 1 << 8
        if case in ("walk-variants", "giant-walk", "tiny-walk"):
            if 300 <= tick < 318: x = 35
            elif 318 <= tick < 328: x = 55
            elif 328 <= tick < 339: x = 100
        if case in ("turn-left", "turn-early-throw", "turn-late-throw"):
            if 300 <= tick < 302: x = -100
            if case == "turn-early-throw" and tick == 302: buttons, y = 1 << 8, 100
            if case == "turn-late-throw" and tick == 309: buttons, y = 1 << 8, 100
        if case in ("full-jump", "short-jump", "tap-jump", "tap-short-jump"):
            held = 300 <= tick < 308 if case in ("full-jump", "tap-jump") else tick == 300
            if held:
                if case.startswith("tap"): y = 100
                else: buttons = 1 << 10
        if case == "ledge-fall" and 300 <= tick < 440: x = 100
        if case.startswith("damage-") and 300 <= tick < 382: x = 100
        throw_tick = 435 if case == "ledge-fall" else 480
        if tick == throw_tick:
            buttons = 1 << 8
            if case == "throw-forward": x = 100
            elif case == "throw-back": x = -100
            elif case == "throw-down": y = -100
            else: x, y = 0, 100
        struct.pack_into("<Hbb", sample, 8, buttons, x, y)
        if tick == 400:
            if case == "damage-keeps-item": struct.pack_into("<Hbb", sample, 24, 1 << 9, 0, 0)
            if case == "damage-drops-item": struct.pack_into("<Hbb", sample, 24, 1 << 8, 0, -100)
        samples.extend(sample)
    path.write_bytes(source[:24] + samples)


def covered(directory, case):
    for label in ("stock", "clone"):
        text = (directory / f"{case}-{label}.log").read_text(errors="replace")
        states = re.findall(r"^\[fighter\] sample (\d+) P1 (.*)$", text, re.M)
        if not any(s.startswith("motion=341 ") for _, s in states): return False
        if case in ("giant-walk", "tiny-walk"):
            scale = "3fe66666" if case == "giant-walk" else "3f000000"
            start = "KinokoGiantStart" if case == "giant-walk" else "KinokoSmallStart"
            end = "KinokoGiantEnd" if case == "giant-walk" else "KinokoSmallEnd"
            if not all(any(s.startswith(f"motion={motion_ids()[name]} ") for _, s in states)
                       for name in (start, end)): return False
            for motion in (342, 343, 344):
                if not any(s.startswith(f"motion={motion} ") and f"scale={scale}" in s
                           for _, s in states): return False
            if not all("scale=3f800000" in s for _, s in states[-100:]): return False
        if case == "ledge-fall":
            targets = (347, motion_ids()["HeavyThrowHi4"], 29, 0, 12, 13)
            cursor = 0
            for target in targets:
                while cursor < len(states) and not states[cursor][1].startswith(f"motion={target} "):
                    cursor += 1
                if cursor == len(states): return False
                cursor += 1
        elif case == "damage-drops-item":
            hit = re.search(r"^\[damage\] sample (\d+) P1 ", text, re.M)
            if hit is None: return False
            tick = int(hit[1])
            if not any(int(t) == tick and s.startswith("motion=88 ") for t, s in states): return False
            if not re.search(rf"^\[item\] sample {tick} I\d+ kind=2 state=3 ", text, re.M): return False
        elif not any(s.startswith("motion=14 ") for _, s in states[-150:]): return False
        if case.startswith("turn"):
            if not any(s.startswith("motion=345 ") and "facing=bf800000" in s for _, s in states): return False
            throw_time = next((int(t) for t, s in states if s.startswith(f"motion={motion_ids()['HeavyThrowHi4']} ")), None)
            if case == "turn-early-throw" and throw_time != 481: return False
            if case == "turn-late-throw" and throw_time != 310: return False
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--case")
    parser.add_argument("--out-dir", type=Path, default=ROOT / ".local" /
                        ("donkey-heavy-" + datetime.datetime.now().strftime("%Y%m%d-%H%M%S")))
    args = parser.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=False)
    tool = str(ROOT / "build/native/native_content_tool.exe")
    project, store, source = args.out_dir / "project", args.out_dir / "store", args.out_dir / "owned.fms"
    image = args.out_dir / "portrait.png"
    portrait(image)
    run(tool, "clone-stock", args.iso, "Donkey Kong", project, "Donkey Heavy Parity")
    run(tool, "import-parameters", project, ROOT / "examples/donkey_wip.fms", source)
    run(tool, "script-build", project, source)
    package = bundled_id(run(tool, "--root", store, "bundle-project", project, image, args.iso))
    run(tool, "--root", store, "accept", package, args.iso)
    ids = motion_ids()
    cases = {"throw-" + direction: (341, ids["HeavyThrow" + suffix + "4"])
             for direction, suffix in (("forward", "F"), ("back", "B"), ("up", "Hi"), ("down", "Lw"))}
    cases.update({"walk-variants": (342, 343, 344), "turn-left": (345,),
                  "turn-early-throw": (345,), "turn-late-throw": (345,),
                  "ledge-fall": (344, 347)})
    cases.update({"damage-keeps-item": (341, 350), "damage-drops-item": (341,),
                  "giant-walk": (342, 343, 344), "tiny-walk": (342, 343, 344)})
    for case in ("full-jump", "short-jump", "tap-jump", "tap-short-jump"):
        cases[case] = (346, 348, 349)
    results = []
    for case, motions in cases.items():
        if args.case and args.case != case: continue
        inputs = args.out_dir / (case + ".fmr")
        recording(inputs, case)
        command = [sys.executable, str(ROOT / "tools/run_fighter_parity.py"), "--iso", str(args.iso.resolve()),
                   "--inputs", str(inputs.resolve()), "--stock-fighter", "donkeykong", "--package-id", package,
                   "--content-root", str(store.resolve()), "--stocks", "2", "--out-dir", str(args.out_dir / case),
                   "--fixture-item", "2,120,-55,10", "--expect-item-kind", "2"]
        for motion in motions: command.extend(("--expect-motion", str(motion)))
        if case.startswith("damage-"): command.extend(("--expect-damage-port", "1"))
        if case == "damage-drops-item": command.extend(("--p2", "human:ganondorf"))
        if case in ("giant-walk", "tiny-walk"):
            kind = 26 if case == "giant-walk" else 27
            command.extend(("--fixture-item", f"{kind},160,-60,10", "--expect-item-kind", str(kind)))
        print("Comparing " + case, flush=True)
        result = subprocess.run(command, cwd=ROOT)
        exit_code = result.returncode
        if not exit_code and not covered(args.out_dir / case, case):
            print("Missing pickup, finish or timing coverage: " + case, flush=True)
            exit_code = 1
        results.append(dict(case=case, package=package, exit=exit_code))
    (args.out_dir / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    return 0 if results and all(r["exit"] == 0 for r in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
