"""Batch recorded special behavior for independent stock clones.

Fresh normal packages, actual controller inputs and no callback injection. Every
selected fighter/scenario is attempted after failures. Default inputs exercise
grounded/airborne entries; --matrix supplies explicit recorded move contracts.
Neither cohort is an exhaustive special or whole-fighter completion claim.
"""

import argparse
import json
from pathlib import Path
import re
import struct
import sys

from fighter_roster_source_parity import FIGHTERS, ROOT, execute, sha
from fighter_script_bundle_roundtrip import portrait


SCENARIOS = tuple((name + "-" + mode, x, y, mode == "air")
    for name, x, y in (("neutral", 0, 0), ("side", 60, 0),
                       ("up", 0, 60), ("down", 0, -60))
    for mode in ("ground", "air"))

# Entry behavior from the stock handlers, rather than the input's starting
# ground/air state. None means stock deliberately offers no aerial special.
ENTRY_MODES = {
    ("donkeykong", "down-air"): None,
    ("peach", "down-air"): None,
    ("bowser", "down-ground"): 1,
    ("yoshi", "side-ground"): 1,
    ("yoshi", "down-ground"): 1,
    ("mrgameandwatch", "up-ground"): 1,
}


def recording(path, x, y, air, seed, samples):
    data = bytearray(struct.pack("<8sQII", b"FMREPLAY", 0, seed, 1))
    for frame in range(samples):
        pads = bytearray(64)
        if air and 88 <= frame < 92:
            struct.pack_into("<Hbb", pads, 0, 0x400, 0, 0)
        if 100 <= frame < 145:
            struct.pack_into("<Hbb", pads, 0, 0x200, x, y)
        data.extend(bytes(8) + pads + bytes(8))
    path.write_bytes(data)


def coverage(log, samples, cloned, entry, source_actors):
    text = log.read_text(errors="replace")
    rows = re.findall(
        r"^\[action\] sample (\d+) P1 kind=(\d+) behavior=(\d+) motion=(\d+) .*", text, re.M)
    states = re.findall(r"^\[fighter\] sample (\d+) P1 motion=(\d+) .*?ga=(\d+) ", text, re.M)
    entries = [(int(sample), int(motion), int(ga)) for sample, motion, ga in states
               if 100 <= int(sample) < 155 and int(motion) >= 341]
    diagnostics = re.findall(r"^\[script\] (?:Item \d+ function|Fighter \d+ event|Source \d+).*", text, re.M)
    faults = list(map(int, re.findall(r"^\[script\] Live fighter .*?faulted=(\d+)", text, re.M)))
    result = dict(log_sha256=sha(log), motions=sorted({int(row[3]) for row in rows}),
                  special_entry=entries[0] if entries else None, diagnostics=diagnostics,
                  faults=faults, full_replay=f"[replay] End after {samples} controller samples." in text)
    errors = []
    if not result["full_replay"]:
        errors.append("Replay incomplete")
    if entry is not None:
        air, entry_mode = entry
        starting = next((int(ga) for sample, _, ga in states if int(sample) == 100), None)
        result["starting_ground_air"] = starting
        if starting != int(air):
            errors.append("Expected pre-input grounded/airborne state absent")
        if entry_mode is None:
            if entries:
                errors.append("Unexpected aerial special for stock's unavailable entry")
        elif not entries or entries[0][2] != entry_mode:
            errors.append("Expected grounded/airborne special entry absent")
    if diagnostics or cloned and (len(faults) != source_actors or any(faults)):
        errors.append("Missing VM report or source/fighter/item fault")
    return result, errors


ARGUMENT_ARITY = {
    "--stage": 1, "--stocks": 1, "--p2": 1, "--p1-type": 1, "--cpu-level": 1,
    "--expect-motion": 1, "--expect-port-motion": 2, "--expect-damage-port": 1,
    "--expect-item-kind": 1, "--expect-item-owner-transfer": 3,
    "--expect-item-event": 2, "--expect-item-removed-on-damage": 2,
    "--expect-item-state": 5, "--expect-item-direction-change": 2,
    "--expect-motion-window": 4,
    "--fixture-airborne": 1, "--fixture-item": 1, "--expect-attachment": 0,
}


def matrix_cases(path):
    manifest = json.loads(path.read_text())
    rows, seen = [], set()
    for original in manifest["cases"]:
        row = dict(original)
        pair = row["fighter"], row["scenario"]
        if pair in seen or row["fighter"] not in {f[1] for f in FIGHTERS} or not re.fullmatch(r"[a-z0-9-]+", row["scenario"]):
            raise ValueError("Duplicate or invalid matrix fighter/scenario: " + str(pair))
        if row.get("opponent") is not None and row["opponent"] not in {f[1] for f in FIGHTERS}:
            raise ValueError("Invalid scripted opponent: " + str(row["opponent"]))
        seen.add(pair)
        row["recording"] = (path.parent / row["recording"]).resolve()
        arguments = row.setdefault("arguments", [])
        cursor = 0
        while cursor < len(arguments):
            flag = arguments[cursor]
            if flag not in ARGUMENT_ARITY or cursor + ARGUMENT_ARITY[flag] >= len(arguments):
                raise ValueError("Unsupported or incomplete matrix argument: " + str(flag))
            cursor += ARGUMENT_ARITY[flag] + 1
        if row.get("opponent"):
            p2 = arguments[arguments.index("--p2") + 1] if "--p2" in arguments else "human:zelda"
            parts = p2.split(":")
            if len(parts) not in (2, 3) or parts[0] not in ("human", "cpu") or parts[1] != row["opponent"] or arguments.count("--p2") > 1:
                raise ValueError("Scripted opponent must match a single reference --p2: " + str(pair))
        # A matrix must demand actual move coverage, not just identical idle play.
        if "--expect-motion" not in arguments:
            raise ValueError("Matrix case needs an explicit expected motion: " + str(pair))
        raw = row["recording"].read_bytes()
        if raw[:8] != b"FMREPLAY" or len(raw) < 104 or (len(raw) - 24) % 80:
            raise ValueError("Invalid matrix recording: " + str(row["recording"]))
        row.update(samples=(len(raw) - 24) // 80, seed=struct.unpack_from("<I", raw, 16)[0], entry=None)
        rows.append(row)
    if not rows:
        raise ValueError("Empty gameplay matrix")
    return rows, [(path.parent / name).resolve() for name in manifest.get("input_files", [])]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--fighter", choices=[row[1] for row in FIGHTERS], action="append")
    parser.add_argument("--scenario", action="append")
    parser.add_argument("--matrix", type=Path,
                        help="Recorded gameplay cases with explicit move/interaction coverage")
    parser.add_argument("--retry-failed-from", type=Path,
                        help="Freshly prepare and rerun failed pairs from a previous receipt")
    parser.add_argument("--samples", type=int, default=480)
    parser.add_argument("--seed", type=lambda s: int(s, 0), default=0x1BADB002)
    args = parser.parse_args()
    if args.samples < 360 or not args.iso.is_file():
        parser.error("Need an ISO and at least 360 samples")
    failed = None
    if args.retry_failed_from:
        previous = json.loads(args.retry_failed_from.read_text())
        failed = {(row["fighter"], row["scenario"]) for row in previous["cases"]
                  if row["status"] == "FAIL"}
    extra_pins = []
    if args.matrix:
        rows, extra_pins = matrix_cases(args.matrix.resolve())
        extra_pins.append(args.matrix.resolve())
    else:
        rows = [dict(fighter=f[1], scenario=name, samples=args.samples, seed=args.seed,
                     arguments=[], x=x, y=y,
                     entry=(air, ENTRY_MODES.get((f[1], name), int(air))))
                for f in FIGHTERS for name, x, y, air in SCENARIOS]
    if args.scenario and set(args.scenario) - {row["scenario"] for row in rows}:
        parser.error("Unknown selected scenario")
    rows = [row for row in rows if (not args.fighter or row["fighter"] in args.fighter)
            and (not args.scenario or row["scenario"] in args.scenario)
            and (failed is None or (row["fighter"], row["scenario"]) in failed)]
    pairs = {(row["fighter"], row["scenario"]) for row in rows}
    if not pairs:
        parser.error("No fighter/scenario pairs selected")
    out = args.out_dir.resolve()
    out.mkdir(parents=True, exist_ok=False)
    iso = args.iso.resolve()
    tool, exe = (ROOT / "build/native" / name for name in
                 ("native_content_tool.exe", "melee_native.exe"))
    required = {row["fighter"] for row in rows} | {row["opponent"] for row in rows if row.get("opponent")}
    fighters = [row for row in FIGHTERS if row[1] in required]
    store = out / "store"
    prepared = {}
    image = out / "portrait.png"
    portrait(image)
    for row in rows:
        if args.matrix:
            original = row["recording"]
            extra_pins.append(original)
            row["recording"] = out / (row["fighter"] + "-" + row["scenario"] + ".fmr")
            row["recording"].write_bytes(original.read_bytes())
        else:
            row["recording"] = out / (row["scenario"] + ".fmr")
            recording(row["recording"], row["x"], row["y"], row["entry"][0], args.seed, args.samples)
    pins = [Path(__file__), tool, exe, iso, image,
            ROOT / "tests/native/fighter_roster_source_parity.py",
            ROOT / "tests/native/fighter_script_bundle_roundtrip.py",
            ROOT / "tools/run_fighter_parity.py", ROOT / "tools/compare_fighter_parity.py"]
    pins += list({row["recording"] for row in rows}) + extra_pins
    pins += [ROOT / "examples" / (row[2] + ".fms") for row in fighters]
    if args.retry_failed_from:
        pins.append(args.retry_failed_from.resolve())
    receipt = dict(status="RUNNING", samples=args.samples if not args.matrix else None,
                   seed=args.seed if not args.matrix else None,
                   inputs={str(path): sha(path) for path in pins}, fighters=[], cases=[],
                   limits=["Explicit recorded move contracts, not every branch or interruption.",
                           "Single bases; family packages have separate acceptance.",
                           "Strict emitted state/RNG/resources, not pixels/audio or complete restored state."])

    def save():
        (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")

    save()
    for stock, launch, template in fighters:
        directory = out / launch
        directory.mkdir()
        project, source = directory / "project", directory / "owned.fms"
        entry = dict(fighter=launch, template=template, status="RUNNING", steps=[])
        receipt["fighters"].append(entry)
        print("Preparing " + launch, flush=True)
        try:
            for stage, command in (
                ("clone", [tool, "clone-stock", iso, stock, project, "Specials " + stock]),
                ("parameters", [tool, "import-parameters", project,
                    ROOT / "examples" / (template + ".fms"), source]),
                ("build", [tool, "script-build", project, source]),
                ("package", [tool, "--root", store, "package-project", project, image])):
                result = execute(command, directory / (stage + ".log"))
                entry["steps"].append(result)
                save()
                if result["exit"] != 0:
                    raise ValueError(stage + " failed")
            match = re.search(r"Cached source fighter ([0-9a-f]{64})",
                              (directory / "package.log").read_text(errors="replace"))
            if not match:
                raise ValueError("Missing package identity")
            identity = match.group(1)
            entry["package"] = identity
            for stage in ("verify-package", "accept"):
                result = execute([tool, "--root", store, stage, identity], directory / (stage + ".log"))
                entry["steps"].append(result)
                if result["exit"] != 0:
                    raise ValueError(stage + " failed")
            for path in [*project.rglob("*"), source, store / "cache" / (identity + ".fmc")]:
                if path.is_file():
                    receipt["inputs"][str(path)] = sha(path)
            entry["status"] = "PASS"
            prepared[launch] = identity
        except Exception as error:
            entry.update(status="FAIL", error=str(error))
            save()
            continue
    save()
    for selected in rows:
        launch, name = selected["fighter"], selected["scenario"]
        opponent = selected.get("opponent")
        case_dir = out / launch / name
        case_dir.mkdir()
        row = dict(fighter=launch, scenario=name, status="RUNNING", coverage={},
                   samples=selected["samples"], seed=selected["seed"],
                   expected_entry=selected["entry"], opponent=opponent)
        receipt["cases"].append(row)
        save()
        if launch not in prepared or opponent and opponent not in prepared:
            row.update(status="FAIL", errors=["Required fighter preparation failed"])
            save()
            continue
        print("Comparing " + launch + " " + name, flush=True)
        opponent_args = ["--p2-package-id", prepared[opponent]] if opponent else []
        result = execute([sys.executable, ROOT / "tools/run_fighter_parity.py",
            "--iso", iso, "--inputs", selected["recording"], "--stock-fighter", launch,
            "--package-id", prepared[launch], "--content-root", store, "--independent",
            "--stocks", "4"] + opponent_args + ["--out-dir", case_dir] + selected["arguments"], case_dir / "compare.log")
        row["comparison"] = result
        errors = []
        for label in ("stock", "clone"):
            log = case_dir / (selected["recording"].stem + "-" + label + ".log")
            if not log.is_file():
                errors.append(label + " log absent")
                continue
            facts, failures = coverage(log, selected["samples"], label == "clone", selected["entry"], 2 if opponent else 1)
            row["coverage"][label] = facts
            errors.extend(label + ": " + error for error in failures)
        if result["exit"] != 0:
            errors.append("Stock/source comparison failed")
        row.update(status="FAIL" if errors else "PASS", errors=errors)
        print(launch + " " + name + " " + row["status"], flush=True)
        save()
    receipt["changed_inputs"] = [path for path, digest in receipt["inputs"].items()
                                 if not Path(path).is_file() or sha(Path(path)) != digest]
    receipt["inputs_unchanged"] = not receipt["changed_inputs"]
    receipt["status"] = "PASS" if receipt["inputs_unchanged"] and all(
        row["status"] == "PASS" for row in receipt["fighters"] + receipt["cases"]) else "FAIL"
    save()
    print(json.dumps({"status": receipt["status"], "receipt": str(out / "receipt.json")}), flush=True)
    return int(receipt["status"] != "PASS")


if __name__ == "__main__":
    raise SystemExit(main())
