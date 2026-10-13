"""Run a focused independent DK/Bowser closure batch from source-checked packages.

Inputs are existing accepted recordings. Frozen binaries/SDKs and source-batch
packages are checked before and after the entire hidden, muted, drawn batch.
No registration in the user's collection or project modification occurs.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
CASES = (
    ("donkey", "cpu-fd-level4", "donkey-cpu-final/idle-final-destination-level4-human-zelda.fmr",
     ("--p1-type", "cpu", "--cpu-level", "4", "--stocks", "4", "--expect-damage-port", "2")),
    ("donkey", "cpu-battlefield-level9", "donkey-cpu-final/idle-battlefield-level9-human-zelda.fmr",
     ("--p1-type", "cpu", "--stage", "battlefield", "--stocks", "4", "--expect-damage-port", "2")),
    ("koopa", "cpu-fd-level4", "bowser-cpu-final2/idle-final-destination-level4.fmr",
     ("--p1-type", "cpu", "--cpu-level", "4", "--stocks", "4", "--expect-damage-port", "2")),
    ("koopa", "cpu-battlefield-level9", "bowser-cpu-final2/idle-battlefield-level9.fmr",
     ("--p1-type", "cpu", "--stage", "battlefield", "--stocks", "4", "--expect-damage-port", "2")),
    ("donkey", "cargo-forward-ground", "donkey-cargo-throw-first-batch/cargo-forward-ground-right.fmr",
     ("--stocks", "2", "--expect-motion", "351", "--expect-motion", "361", "--expect-damage-port", "2")),
    ("donkey", "heavy-throw-forward", "donkey-heavy-pass1/throw-forward.fmr",
     ("--stocks", "2", "--fixture-item", "2,120,-55,10", "--expect-item-kind", "2", "--expect-motion", "341", "--expect-damage-port", "2")),
    ("donkey", "punch-release-contact", "donkey-all-special-first-batch/neutral-ground-release-contact.fmr",
     ("--stocks", "2", "--expect-motion", "369", "--expect-motion", "372", "--expect-damage-port", "2")),
    ("koopa", "flame-contact", "bowser-breath-first/ground-contact.fmr",
     ("--stocks", "2", "--expect-motion", "341", "--expect-item-kind", "100", "--expect-damage-port", "2")),
    ("koopa", "down-contact-victim", "bowser-special-final/down-ground-contact.fmr",
     ("--stocks", "2", "--expect-motion", "361", "--expect-damage-port", "2")),
    ("koopa", "capture-back-victim", "bowser-capture-final/ground-back.fmr",
     ("--stocks", "2", "--expect-motion", "347", "--expect-motion", "350", "--expect-damage-port", "2")),
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check(hashes):
    for name, expected in hashes.items():
        if digest(Path(name)) != expected:
            raise RuntimeError("Input changed: " + name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", required=True, type=Path)
    parser.add_argument("--source-batch", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--iso", type=Path, default=ROOT.parent / "Super Smash Bros. Melee (USA) (v1.02).iso")
    parser.add_argument("--candidate", action="store_true")
    parser.add_argument("--case", action="append")
    args = parser.parse_args()
    snapshot, source, out = (getattr(args, key).resolve() for key in ("snapshot", "source_batch", "output"))
    if out.exists() or snapshot == out or snapshot in out.parents or source == out or source in out.parents:
        parser.error("Use a fresh output outside source batch and snapshot")
    receipt_path = source / "acceptance.json"
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    required_mode = "candidate" if args.candidate else "shipped independent"
    if receipt["failures"] or receipt["mode"] != required_mode:
        raise RuntimeError("Source comparison mode/result does not match request")
    if args.case and set(args.case) - {f"{fighter}/{name}" for fighter, name, _, _ in CASES}:
        parser.error("Unknown closure case")
    frozen_path = snapshot / "freeze.json"
    frozen = json.loads(frozen_path.read_text(encoding="utf-8"))
    hashes = {str(snapshot / name): value for name, value in frozen.items()}
    hashes[str(frozen_path)] = digest(frozen_path)
    hashes[str(receipt_path)] = digest(receipt_path)
    for name, value in receipt["input_sha256"].items():
        hashes[name] = value
    for name, value in receipt["owned_source_sha256"].items():
        hashes[str(source / name)] = value
    for name, value in receipt["packages"]["package_files"].items():
        hashes[str(source / name)] = value
    hashes[str(args.iso.resolve())] = digest(args.iso)
    hashes[str(Path(__file__).resolve())] = digest(Path(__file__))
    for name in ("run_fighter_parity.py", "compare_fighter_parity.py"):
        hashes[str(ROOT / "tools" / name)] = digest(ROOT / "tools" / name)
    cases = []
    for fighter, name, recording, flags in CASES:
        if args.case and f"{fighter}/{name}" not in args.case:
            continue
        path = ROOT / ".local" / recording
        raw = path.read_bytes()
        if raw[:8] != b"FMREPLAY" or (len(raw) - 24) % 80:
            raise RuntimeError("Invalid recording " + str(path))
        hashes[str(path)] = digest(path)
        cases.append(dict(fighter=fighter, case=name, recording=str(path), flags=flags,
                          samples=(len(raw) - 24) // 80))
    check(hashes)
    out.mkdir(parents=True)
    store = out / "store"
    shutil.copytree(source / "store", store)
    tool = snapshot / "bin/native_content_tool.exe"
    exe = snapshot / "bin/melee_native.exe"
    commands = []
    def run(command, log):
        commands.append(list(map(str, command)))
        result = subprocess.run(commands[-1], cwd=ROOT, capture_output=True, text=True)
        log.write_text(subprocess.list2cmdline(commands[-1]) + "\n" + result.stdout + result.stderr, encoding="utf-8")
        if result.returncode:
            raise RuntimeError(f"Exit {result.returncode}: {log}")
        return result.stdout.strip()
    for fighter in ("donkey", "koopa"):
        package = receipt["packages"][fighter]["id"]
        run([tool, "--root", store, "verify-package", package, args.iso], out / (fighter + "-verify.log"))
        run([tool, "--root", store, "accept", package, args.iso], out / (fighter + "-accept.log"))
    (out / "plan.json").write_text(json.dumps(dict(mode=required_mode, cases=cases, input_sha256=hashes), indent=2), encoding="utf-8")
    results = []
    for case in cases:
        folder = out / (case["fighter"] + "-" + case["case"])
        folder.mkdir()
        recording = folder / "inputs.fmr"
        recording.write_bytes(Path(case["recording"]).read_bytes())
        command = [sys.executable, ROOT / "tools/run_fighter_parity.py", "--iso", args.iso,
                   "--exe", exe, "--inputs", recording, "--stock-fighter",
                   "donkey-kong" if case["fighter"] == "donkey" else "bowser",
                   "--package-id", receipt["packages"][case["fighter"]]["id"],
                   "--content-root", store, "--out-dir", folder, "--independent", *case["flags"]]
        try:
            result = dict(case, exit=0, comparison=run(command, folder / "parity.log"))
        except RuntimeError as error:
            result = dict(case, exit=1, error=str(error))
        results.append(result)
        (out / "results.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
        print(f"{case['fighter']}/{case['case']}: {result}", flush=True)
    check(hashes)
    (out / "commands.json").write_text(json.dumps(commands, indent=2), encoding="utf-8")
    if any(row["exit"] for row in results):
        raise RuntimeError("Closure batch failed; inspect preserved results")
    evidence = dict(mode=required_mode, pairs=len(results), controller_samples=sum(row["samples"] for row in results),
                    results=results, input_sha256=hashes, hidden=True, muted=True, drawing=True,
                    limits="Selected gameplay/CPU/common-item/capture paths only; no pixel/audio, complete planner/costume/lifecycle matrix, Workshop or network rollback acceptance.")
    (out / "acceptance.json").write_text(json.dumps(evidence, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
