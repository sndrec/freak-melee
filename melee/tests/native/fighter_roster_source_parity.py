"""Compare fresh independent Single Workshop packages with stock CPU gameplay.

All selected fighters are attempted, including after a preparation or gameplay
failure. Paired alternate/companion packages are deliberately separate. Games
are hidden, muted and rendered. A receipt pins source/tools before and after the
batch and records full replay consumption, VM faults and actual motion coverage.
This is a fixed-input CPU cohort, not exhaustive fighter or rollback acceptance.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from fighter_script_bundle_roundtrip import portrait


ROOT = Path(__file__).resolve().parents[2]
FIGHTERS = (
    ("Mario", "mario", "mario_full"),
    ("DrMario", "drmario", "drmario_full"),
    ("Captain Falcon", "captainfalcon", "captain_wip"),
    ("Ganon", "ganondorf", "ganon_wip"),
    ("Luigi", "luigi", "luigi_wip"),
    ("Jigglypuff", "jigglypuff", "purin_wip"),
    ("Fox", "fox", "fox_wip"),
    ("Falco", "falco", "falco_wip"),
    ("Marth", "marth", "marth_wip"),
    ("Roy", "roy", "roy_wip"),
    ("Donkey Kong", "donkeykong", "donkey_wip"),
    ("Bowser", "bowser", "koopa_wip"),
    ("Pikachu", "pikachu", "pikachu_wip"),
    ("Pichu", "pichu", "pichu_wip"),
    ("Samus", "samus", "samus_full"),
    ("Link", "link", "link_full"),
    ("Young Link", "younglink", "young_link_full"),
    ("Mewtwo", "mewtwo", "mewtwo_wip"),
    ("Yoshi", "yoshi", "yoshi_wip"),
    ("Ness", "ness", "ness_wip"),
    ("Peach", "peach", "peach_full"),
    ("Game & Watch", "mrgameandwatch", "gamewatch_full"),
)


def sha(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        while block := stream.read(1024 * 1024):
            value.update(block)
    return value.hexdigest()


def execute(command, log, timeout=240):
    command = list(map(str, command))
    try:
        with log.open("wb") as output:
            process = subprocess.run(command, cwd=ROOT, stdout=output,
                                     stderr=subprocess.STDOUT, timeout=timeout,
                                     creationflags=subprocess.CREATE_NO_WINDOW)
        return dict(command=command, exit=process.returncode, log=str(log),
                    log_sha256=sha(log))
    except subprocess.TimeoutExpired:
        return dict(command=command, exit=None, timed_out=True, log=str(log),
                    log_sha256=sha(log))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--fighter", choices=[f[1] for f in FIGHTERS], action="append")
    parser.add_argument("--samples", type=int, default=1800)
    parser.add_argument("--seed", type=lambda s: int(s, 0), default=0x1BADB002)
    args = parser.parse_args()
    if args.samples < 360:
        parser.error("At least 360 samples are required beyond match startup")
    iso = args.iso.resolve()
    if not iso.is_file():
        parser.error("Missing ISO")
    out = args.out_dir.resolve()
    out.mkdir(parents=True, exist_ok=False)
    tool, exe = (ROOT / "build/native" / name for name in
                 ("native_content_tool.exe", "melee_native.exe"))
    selected = [f for f in FIGHTERS if not args.fighter or f[1] in args.fighter]
    base = ROOT / "tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr"
    data = base.read_bytes()
    if data[:8] != b"FMREPLAY" or len(data) < 104 or (len(data) - 24) % 80:
        raise ValueError("Invalid seed recording")
    header, sample = bytearray(data[:24]), bytearray(data[24:104])
    struct.pack_into("<I", header, 16, args.seed)
    # Preserve frame metadata; clear each port's 16-byte controller payload.
    for port in range(4):
        sample[8 + port * 16:24 + port * 16] = bytes(16)
    replay = out / "cpu-neutral.fmr"
    replay.write_bytes(header + sample * args.samples)
    image = out / "portrait.png"
    portrait(image)
    pins = [Path(__file__), tool, exe, iso, base, replay, image,
            ROOT / "tests/native/fighter_script_bundle_roundtrip.py",
            ROOT / "tools/run_fighter_parity.py",
            ROOT / "tools/compare_fighter_parity.py",
            ROOT / "src/melee/ui/native_shell_workshop_editor_script.cpp"]
    pins += [ROOT / "examples" / (f[2] + ".fms") for f in selected]
    pins += sorted((ROOT / "src/melee/it").glob("native_article_resource*.[ch]"))
    pins += sorted((ROOT / "src/melee/mp").glob("native_collision*.[ch]"))
    pins += [ROOT / "src/melee/mp" / name for name in ["mplib.c", "mpcoll.c", "mpisland.c"]]
    pins += [ROOT / "src/melee/gr/grdynamicattr.c"]
    receipt = dict(status="RUNNING", samples=args.samples, seed=args.seed,
                   input_hashes={str(p): sha(p) for p in pins}, cases=[],
                   limits=["Single bases only; paired members need family packages.",
                           "Level-nine CPU against neutral human Zelda on Final Destination.",
                           "Exact diagnostic traces are not pixel/audio fidelity or complete mutable state.",
                           "No complete move/lifecycle/results/rollback acceptance."])
    def save():
        (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    save()
    for stock_name, launch_name, template in selected:
        directory = out / launch_name
        directory.mkdir()
        project, store, source = (directory / name for name in ("project", "store", "owned.fms"))
        entry = dict(fighter=launch_name, template=template, steps=[], status="RUNNING")
        receipt["cases"].append(entry)
        save()
        print("Preparing " + launch_name, flush=True)
        commands = [
            ("clone", [tool, "clone-stock", iso, stock_name, project, "Roster " + stock_name]),
            ("parameters", [tool, "import-parameters", project, ROOT / "examples" / (template + ".fms"), source]),
            ("build", [tool, "script-build", project, source]),
            ("package", [tool, "--root", store, "package-project", project, image]),
        ]
        try:
            for stage, command in commands:
                result = execute(command, directory / (stage + ".log"))
                entry["steps"].append(result)
                save()
                if result["exit"] != 0:
                    raise ValueError(stage + " failed")
            match = re.search(r"Cached source fighter ([0-9a-f]{64})",
                              (directory / "package.log").read_text(errors="replace"))
            if not match:
                raise ValueError("Package ID missing from successful publication")
            package = match.group(1)
            entry["package"] = package
            for stage in ("verify-package", "accept"):
                result = execute([tool, "--root", store, stage, package], directory / (stage + ".log"))
                entry["steps"].append(result)
                save()
                if result["exit"] != 0:
                    raise ValueError(stage + " failed")
            # Freeze the actual author source/assets and canonical package for gameplay.
            for path in [*project.rglob("*"), source, store / "cache" / (package + ".fmc")]:
                if path.is_file():
                    receipt["input_hashes"][str(path)] = sha(path)
            save()
            print("Comparing " + launch_name, flush=True)
            result = execute([sys.executable, ROOT / "tools/run_fighter_parity.py",
                              "--iso", iso, "--inputs", replay, "--stock-fighter", launch_name,
                              "--package-id", package, "--content-root", store, "--independent",
                              "--p1-type", "cpu", "--cpu-level", "9", "--stocks", "4",
                              "--out-dir", directory / "game"], directory / "compare.log")
            entry["steps"].append(result)
            entry["coverage"] = {}
            for label in ("stock", "clone"):
                log = directory / "game" / (replay.stem + "-" + label + ".log")
                if not log.is_file():
                    raise ValueError(label + " gameplay log absent")
                text = log.read_text(errors="replace")
                rows = re.findall(r"^\[action\] sample \d+ P1 .*?motion=(\d+) ", text, re.M)
                entry["coverage"][label] = dict(
                    motions=sorted(set(map(int, rows))), transitions=len(rows),
                    full_replay=f"[replay] End after {args.samples} controller samples." in text,
                    log_sha256=sha(log))
                if not entry["coverage"][label]["full_replay"]:
                    raise ValueError(label + " did not consume the complete replay")
                if label == "clone":
                    diagnostics = re.findall(
                        r"^\[script\] (?:Item \d+ function|Fighter \d+ event|Source \d+).*", text, re.M)
                    entry["coverage"][label]["diagnostics"] = diagnostics
                    faults = re.findall(r"^\[script\] Live fighter .*?faulted=(\d+)", text, re.M)
                    entry["coverage"][label]["faults"] = list(map(int, faults))
                    if diagnostics:
                        raise ValueError("Interpreter source/item/fighter diagnostics: " + str(len(diagnostics)))
                    if not faults or any(int(v) != 0 for v in faults):
                        raise ValueError("Missing interpreter report or VM fault")
            if result["exit"] != 0:
                raise ValueError("Stock/source trace comparison failed")
            entry["status"] = "PASS"
        except Exception as error:
            entry.update(status="FAIL", error=str(error))
        print(launch_name + " " + entry["status"], flush=True)
        save()
    receipt["changed_inputs"] = [p for p, value in receipt["input_hashes"].items()
                                 if not Path(p).is_file() or sha(Path(p)) != value]
    receipt["inputs_unchanged"] = not receipt["changed_inputs"]
    receipt["status"] = "PASS" if receipt["inputs_unchanged"] and all(
        entry["status"] == "PASS" for entry in receipt["cases"]) else "FAIL"
    save()
    print(json.dumps({"status": receipt["status"], "receipt": str(out / "receipt.json")}), flush=True)
    return int(receipt["status"] != "PASS")


if __name__ == "__main__":
    raise SystemExit(main())
