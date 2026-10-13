"""Rendered, hidden and muted stock/source Ice Climbers special comparisons."""
import argparse
import hashlib
import json
import re
import struct
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SAMPLES = 360
# Retail ftPopo_MotionState: neutral 341/342, Squall 343..346,
# Belay 347..356, Blizzard 357/358.
CASES = (
    ("neutral", 0, 0, 1, {341, 342}),
    ("side", 60, 0, 1, set(range(343, 347))),
    ("down", 0, -60, 40, {357, 358}),
    ("up", 0, 60, 1, set(range(347, 357))),
)


def sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def controller_input(path, x, y, hold, samples=SAMPLES, starts=(100,)):
    data = bytearray(struct.pack("<8sQII", b"FMREPLAY", 0, 0x4D45584C, 1))
    for sample in range(samples):
        pads = bytearray(64)
        if any(start <= sample < start + hold for start in starts):
            struct.pack_into("<Hbb", pads, 0, 0x0200, x, y)
        data.extend(bytes(8) + pads + bytes(8))
    path.write_bytes(data)


def records(text, pattern):
    result = {}
    for match in re.finditer(pattern, text, re.M):
        key = tuple(match.groups()[:-1])
        result[key] = dict(field.split("=", 1) for field in match.groups()[-1].split())
    return result


def compare(stock, source):
    differences = {}
    for key in sorted(stock.keys() | source.keys()):
        left, right = stock.get(key), source.get(key)
        for field in sorted((left or {}).keys() | (right or {}).keys()):
            a, b = (left or {}).get(field), (right or {}).get(field)
            if a != b:
                if field not in differences:
                    differences[field] = {"count": 0, "first": {"record": key, "stock": a, "source": b}}
                differences[field]["count"] += 1
    return {"equal": stock == source, "stock_records": len(stock),
            "source_records": len(source), "differences": differences}


def run_game(command, directory, mode, samples=None):
    if samples is None:
        samples = SAMPLES
    stdout_path = directory / (mode + ".stdout.log")
    stderr_path = directory / (mode + ".stderr.log")
    start = time.monotonic()
    timed_out = False
    with stdout_path.open("wb") as stdout, stderr_path.open("wb") as stderr:
        process = subprocess.Popen(command, cwd=ROOT, stdout=stdout, stderr=stderr,
                                   creationflags=subprocess.CREATE_NO_WINDOW)
        print(f"{directory.name} {mode}: hidden muted PID {process.pid}", flush=True)
        try:
            code = process.wait(timeout=180)
        except subprocess.TimeoutExpired:
            timed_out = True
            process.kill()
            code = process.wait()
    stdout = stdout_path.read_text(errors="replace")
    stderr = stderr_path.read_text(errors="replace")
    # Stable port/role and visible state, rather than custom-vs-stock kind IDs.
    actions = re.findall(
        r"^\[(action|companion-action)\] sample (\d+) P(\d+) kind=\d+ behavior=\d+ "
        r"motion=(\d+) anim=(-?\d+) frame=([^\n]+)", stderr, re.M)
    fighters = records(stderr, r"^\[(fighter|companion)\] sample (\d+) P(\d+) (.*)$")
    items = records(stderr, r"^\[item\] sample (\d+) I(\d+) (.*)$")
    item_fields = records(stderr, r"^\[item-fields\] sample (\d+) I(\d+) (.*)$")
    # Item core/relations digest can include native-vs-script storage policy.
    item_visible = {key: {field: value for field, value in row.items()
                         if field in ("kind", "state", "owner", "pos", "vel")}
                    for key, row in items.items()}
    live = re.findall(r"^\[script\] Live fighter [^\n]+", stdout, re.M)
    damage = re.findall(r"^\[(damage|companion-damage)\] sample (\d+) P(\d+) (.*)$", stderr, re.M)
    fault = bool(re.search(r"^\[script\] (?:Fighter \d+ event|Item \d+ function)|faulted=[1-9]",
                           stdout + stderr, re.M))
    return {"exit": code, "timed_out": timed_out, "elapsed": time.monotonic() - start,
            "command": command, "script_fault": fault, "live": live,
            "eof": f"[replay] End after {samples} controller samples." in stderr,
            "damage": damage,
            "actions": actions, "fighters": fighters, "items": item_visible,
            "item_fields": item_fields,
            "logs": {str(path): sha(path) for path in (stdout_path, stderr_path)}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("exe", "iso", "content-root", "source-draft", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--package", required=True)
    parser.add_argument("--scenario", choices=("ground", "cpu-injury"), default="ground")
    args = parser.parse_args()
    injury = args.scenario == "cpu-injury"
    samples = 1200 if injury else SAMPLES
    output = args.output.resolve()
    if output.drive.upper() != "A:" or output.exists():
        parser.error("Use a fresh private output directory on A:")
    if not re.fullmatch(r"[0-9a-f]{64}", args.package):
        parser.error("Package must be a complete content ID")
    package = args.content_root / "cache" / (args.package + ".fmc")
    packet = package.read_bytes()
    for name, relative in (("popo", "behavior.fms"), ("nana", "partner/behavior.fms")):
        canonical = (ROOT / "examples" / (name + "_full.fms")).read_bytes()
        if (args.source_draft / relative).read_bytes() != canonical or canonical not in packet:
            parser.error("Draft and package must contain unchanged current full Popo/Nana sources")
    files = [args.exe, args.iso, package, Path(__file__), Path(__file__).with_name("README.md")]
    files += [path for path in args.source_draft.rglob("*") if path.is_file()]
    files += [ROOT / name for name in (
        "examples/popo_full.fms", "examples/nana_full.fms",
        "src/melee/gm/native_replay_state.c", "src/melee/ft/native_fighter_runtime.c",
        "src/melee/gm/gm_1798.c", "src/melee/ft/fighter.c",
        "src/melee/ft/kinds/ftCommon/ftCo_CatchPull.c",
        "src/melee/ft/kinds/ftCommon/ftCo_CapturePulled.c",
        "src/melee/it/kinds/itclimbersstring.c", "src/melee/ft/kinds/ftPopo/forward.h")]
    pins = {str(path.resolve()): sha(path) for path in files}
    output.mkdir(parents=True)
    results = []
    for name, x, y, hold, expected in CASES:
        directory = output / name
        directory.mkdir()
        replay = directory / "input.fmreplay"
        controller_input(replay, x, y, hold, samples, (100, 300, 500) if injury else (100,))
        pair = {}
        for mode in ("stock", "source"):
            command = [str(args.exe.resolve()), "--iso", str(args.iso.resolve()), "--match",
                       "--stage", "final-destination", "--p1",
                       "human:custom" if mode == "source" else "human:iceclimbers",
                       "--p2", "cpu:mario:9" if injury else "human:mario",
                       "--p3", "off", "--p4", "off", "--mode", "stock",
                       "--stocks", "1", "--time", "0", "--items", "off", "--hidden", "--mute", "--fast",
                       "--replay-inputs", str(replay), "--trace-actions", "--trace-fighter-state"]
            if mode == "source":
                command += ["--content-root", str(args.content_root.resolve()), "--p1-fighter", args.package]
            pair[mode] = run_game(command, directory, mode, samples)
            pair[mode]["entered_special"] = any(
                action[0] == "action" and action[2] == "1" and int(action[3]) in expected
                for action in pair[mode]["actions"])
        comparisons = {field: compare(pair["stock"][field], pair["source"][field])
                       for field in ("fighters", "items", "item_fields")}
        action_equal = pair["stock"]["actions"] == pair["source"]["actions"]
        exercised = all(run["exit"] == 0 and run["eof"] and run["entered_special"] and
                        not run["script_fault"] and not run["timed_out"] for run in pair.values())
        companion_counts = [sum(key[0] == "companion" and key[2] == "1"
                                for key in run["fighters"]) for run in pair.values()]
        # Gameplay tracing ends on entering results, before controller EOF.
        companions = (all(count > 0 for count in companion_counts) and
                      companion_counts[0] == companion_counts[1]) if injury else all(
                          count == samples for count in companion_counts)
        damaged = all(run["damage"] for run in pair.values()) if injury else True
        # Neutral/up scenarios reproduce the holder-before-victim grab boundary.
        captured = all(any(action[2] == "1" and int(action[3]) in (223, 226)
                           for action in run["actions"]) for run in pair.values()) if (
                               injury and name in ("neutral", "up")) else True
        healthy = exercised and companions and damaged and captured and action_equal and all(
            comparisons[field]["equal"] for field in ("fighters", "items"))
        summaries = {mode: {key: value for key, value in run.items()
                           if key not in ("fighters", "items", "item_fields")} for mode, run in pair.items()}
        row = {"case": name, "status": "PASS traced comparison" if healthy else "FAIL traced comparison",
               "runs": summaries, "input": {str(replay): sha(replay)},
               "action_timeline_equal": action_equal, "companion_frames_complete": companions,
               "damage_exercised": damaged if injury else None,
               "capture_exercised": captured if injury and name in ("neutral", "up") else None,
               "comparisons": comparisons}
        results.append(row)
        print(json.dumps({"case": name, "status": row["status"], "comparisons": comparisons}), flush=True)
    unchanged = all(sha(Path(path)) == value for path, value in pins.items())
    passing = unchanged and all(row["status"].startswith("PASS") for row in results)
    receipt = {"status": "PASS" if passing else "FAIL", "scenario": args.scenario,
               "samples": samples, "inputs": pins,
               "inputs_unchanged": unchanged, "results": results,
               "limits": ["Inputs-only playback does not validate replay signatures, RNG or state digests.",
                          "Four grounded entries against a neutral human or level-9 CPU; airborne/partner-death/full match-end acceptance remains separate.",
                          "Fighter comparison covers both native Player roles including Nana; Item visible fields are rounded diagnostic values.",
                          "Detailed Item digests are reported separately and are not a pass gate; no GPU image or compact snapshot proof."]}
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"receipt": str(output / "receipt.json"), "status": receipt["status"]}), flush=True)
    raise SystemExit(0 if passing else 1)


if __name__ == "__main__":
    main()
