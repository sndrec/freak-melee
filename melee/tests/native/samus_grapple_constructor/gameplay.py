"""Load the composed Samus candidate normally and compare actual game replays.

This installs only in a fresh private collection on A:. It does not alter
the user's collection or inject script
hooks into a manually populated Fighter. Each case uses the normal package
loader, Fighter constructor, scheduler, stage collision, capture and rendering.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
KINDS = ("empty", "air", "throw-forward", "throw-back", "throw-up", "throw-down",
         "wall-expiry", "wall-pull", "modified-ground",
         "modified-wall-hold", "modified-wall-pull", "held-damage",
         "flight-damage", "death-reuse")
THROWS = {"throw-forward": 219, "throw-back": 220,
          "throw-up": 221, "throw-down": 222}


def samples(kind):
    if kind == "death-reuse": return 2800
    if kind.endswith("-damage"): return 900
    return 1100 if kind.startswith("modified-wall-") else 600


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def recording(baseline, destination, kind, facing):
    sign = 1 if facing == "right" else -1
    frames = bytearray()
    for tick in range(samples(kind)):
        sample = bytearray(baseline[24:104])
        buttons, x, y = 0, 0, 0
        if facing == "left" and 80 <= tick < 100:
            x = -80
        if tick == 200:
            buttons = 16  # Physical Z is mapped to shield + attack by Melee.
        if kind == "wall-pull" and tick == 300:
            buttons = 256
        if kind.startswith("modified-") or kind == "death-reuse":
            # Complete the retail code while the first, ordinary grapple is
            # active. The next creation consumes its persistent modified mode.
            if tick in (210, 214): buttons = 8
            if tick == 212: buttons = 4
            if tick == 216: buttons = 256
            if tick == 350: buttons = 16
            if kind == "modified-ground":
                if 370 <= tick < 400: buttons = 64  # Homing with physical L.
                if tick == 400: buttons = 256      # Activate endpoint capsule.
            elif kind.startswith("modified-wall-"):
                if 400 <= tick < 650: buttons = 64  # Preserve hang time.
                if kind == "modified-wall-pull" and tick == 650: buttons = 256
        if kind == "death-reuse" and tick in (710, 1200, 1550, 2400):
            buttons = 16
        if kind in THROWS and tick == 280:
            if kind == "throw-forward":
                x = 80 * sign
            elif kind == "throw-back":
                x = -80 * sign
            else:
                y = 80 if kind == "throw-up" else -80
        struct.pack_into("<Hbb", sample, 8, buttons, x, y)
        other_buttons = other_x = 0
        if kind.endswith("-damage"):
            # P2 normally faces left. Mirror that setup too when striking a
            # left-facing P1, rather than attacking away from the target.
            if facing == "left" and 80 <= tick < 100: other_x = 80
            attack = 210 if kind == "held-damage" else 225
            if tick == attack: other_buttons = 256
        struct.pack_into("<Hbb", sample, 24, other_buttons, other_x, 0)
        frames.extend(sample)
    destination.write_bytes(baseline[:24] + frames)


def coverage(directory, name, kind):
    """Require actual behavior, not just a clean exit or an unused package."""
    receipts = []
    for runtime in ("stock", "clone"):
        text = (directory / f"{name}-{runtime}.log").read_text(
            encoding="utf-8", errors="replace")
        items = [(int(sample), int(state)) for sample, state in re.findall(
            r"^\[item\] sample (\d+) I\d+ kind=96 state=(\d+) ", text, re.M)]
        motions = {int(value) for value in re.findall(
            r"^\[action\] sample \d+ P1 .*?motion=(\d+) ", text, re.M)}
        wall = kind.startswith(("wall-", "modified-wall-"))
        required = {357} if kind == "air" or wall else {212}
        if kind.startswith("modified-"): required.add(212)
        if wall:
            required.add(358)
        if kind in ("wall-expiry", "modified-wall-hold"):
            required.add(38)  # DamageFall on hang expiry.
        if kind in ("wall-pull", "modified-wall-pull"):
            required.add(261)  # CliffJumpSlow2 fallback when no ledge is caught.
        if kind in THROWS:
            required |= {213, 216, THROWS[kind]}
        states = {state for _, state in items}
        required_states = {0, 1, 5} if kind in THROWS else {0, 1, 4}
        if kind == "held-damage": required_states = {0}
        if kind == "flight-damage": required_states = {0, 1}
        if wall:
            required_states = {0, 1, 6, 8}
        if kind in ("wall-pull", "modified-wall-pull"):
            required_states.add(7)
        problems = []
        if not required <= motions:
            problems.append(f"missing fighter motions {sorted(required - motions)}")
        if not required_states <= states:
            problems.append(f"missing grapple states {sorted(required_states - states)}")
        if not items or items[-1][0] >= samples(kind) - 1:
            problems.append("grapple did not complete removal before replay ended")
        if f"[replay] End after {samples(kind)} controller samples." not in text:
            problems.append("replay did not consume every controller sample")
        counts = {int(value) for value in re.findall(
            r"^\[item-resources\] sample \d+ I\d+ count=(\d+)$", text, re.M)}
        expected_count = 60 if kind == "modified-ground" else 45 if kind == "air" or wall else 30
        if expected_count not in counts:
            problems.append(f"missing complete {expected_count}-node resource chain")
        if kind == "modified-ground" and not re.search(
            r"^\[geometry-hitbox\] P1 member=0 group=attack hit=0 state=[1-9]", text, re.M):
            problems.append("modified endpoint capsule was not active at sample 401")
        if kind.endswith("-damage"):
            damage = {int(value) for value in re.findall(
                r"^\[damage\] sample (\d+) P1 ", text, re.M)}
            occupied = {sample for sample, _ in items}
            if not any(tick - 1 in occupied and tick not in occupied for tick in damage):
                problems.append("live grapple was not removed on fighter damage")
        if kind == "death-reuse":
            deaths = [int(value) for value in re.findall(
                r"^\[action\] sample (\d+) P1 .*?motion=\d+ anim=-1 frame=-1.00", text, re.M)
                if int(value) > 300]
            # Natural blast-zone deaths must occur with a live chain, not an
            # already completed move or a fixture directly clearing callbacks.
            occupied = {sample for sample, _ in items}
            live_deaths = [tick for tick in deaths if tick - 1 in occupied and tick not in occupied]
            if len(live_deaths) != 2:
                problems.append(f"expected two live-chain deaths, observed {live_deaths}")
            node_records = [(int(tick), int(count)) for tick, count in re.findall(
                r"^\[item-resources\] sample (\d+) I\d+ count=(\d+)$", text, re.M)]
            if 60 not in counts:
                problems.append("modified ground chain was not constructed before death")
            for after in (1200, 2400):
                following = [count for tick, count in node_records if after <= tick < after + 100]
                if not following or any(count != 30 for count in following):
                    problems.append(f"ordinary 30-node grapple did not return after sample {after}")
        if re.search(r"^\[script\] (?:Live|Retired) fighter .*faulted=[1-9]", text, re.M):
            problems.append("fighter interpreter fault")
        receipts.append(dict(runtime=runtime, motions=sorted(motions),
                             grapple_states=sorted(states), node_counts=sorted(counts), problems=problems))
    return receipts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", required=True, type=Path)
    parser.add_argument("--base-project", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--case", action="append",
                        choices=[f"{kind}-{facing}" for kind in KINDS
                                 for facing in ("right", "left")])
    args = parser.parse_args()
    output = args.output.resolve()
    if output.drive.upper() != "A:" or output.exists():
        parser.error("a fresh A: output directory is required")
    output.mkdir(parents=True)
    project, store = output / "project", output / "store"
    shutil.copytree(args.base_project.resolve(), project)
    composition = load(HERE / "compose.py", "samus_gameplay_composition")
    helpers = load(ROOT / "tests/native/fighter_script_bundle_roundtrip.py",
                   "samus_gameplay_package_tools")
    inputs = composition.INPUTS + [HERE / "compose.py", Path(__file__),
        ROOT / "examples/samus_full.fms",
        ROOT / "build/native/native_content_tool.exe",
        ROOT / "build/native/melee_native.exe",
        ROOT / "tools/run_fighter_parity.py",
        ROOT / "tools/compare_fighter_parity.py",
        ROOT / "tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr"]
    hashes = {str(path): digest(path) for path in inputs}
    source = output / "candidate.fms"
    # Exercise exactly the normal Workshop template, keeping installation
    # private without replacing its behavior or adding a test-only declaration.
    source.write_bytes((ROOT / "examples/samus_full.fms").read_bytes())
    owned, portrait = output / "owned.fms", output / "portrait.png"
    helpers.portrait(portrait)
    tool = ROOT / "build/native/native_content_tool.exe"
    receipt = dict(status="RUNNING", input_hashes=hashes, cases=[],
                   limitations=["No terrain-blocked exit, exhaustive interruption/retirement "
                                 "matrix or screenshot-level visual acceptance."])

    def save():
        (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")

    def command(label, arguments):
        result = subprocess.run(list(map(str, arguments)), cwd=ROOT,
            capture_output=True, text=True, timeout=90,
            creationflags=subprocess.CREATE_NO_WINDOW)
        (output / f"{label}.log").write_text(result.stdout + result.stderr,
                                            encoding="utf-8")
        if result.returncode:
            raise RuntimeError(f"{label} exited {result.returncode}; inspect its log")
        return result.stdout

    save()
    try:
        command("parameters", [tool, "import-parameters", project, source, owned])
        command("audit", [tool, "source-audit", project, owned])
        command("build", [tool, "script-build", project, owned])
        package_text = command("package", [tool, "--root", store,
                                            "package-project", project, portrait])
        package = re.search(r"[0-9a-f]{64}", package_text).group(0)
        receipt["package"] = package
        command("verify", [tool, "--root", store, "verify-package", package])
        command("accept", [tool, "--root", store, "accept", package])
        baseline = inputs[-1].read_bytes()
        for kind in KINDS:
            for facing in ("right", "left"):
                name = f"{kind}-{facing}"
                if args.case and name not in args.case:
                    continue
                replay, directory = output / f"{name}.fmr", output / name
                recording(baseline, replay, kind, facing)
                call = [sys.executable, ROOT / "tools/run_fighter_parity.py",
                    "--iso", args.iso.resolve(), "--inputs", replay,
                    "--stock-fighter", "samus", "--package-id", package,
                    "--content-root", store, "--out-dir", directory,
                    "--stocks", "4" if kind == "death-reuse" else "2",
                    "--independent", "--expect-item-kind", "96",
                    "--p2", "human:mario", "--expect-motion",
                    "357" if kind == "air" or kind.startswith("wall-") else "212"]
                if kind == "air":
                    call += ["--fixture-airborne", "1,190,-60,70"]
                if kind.startswith("wall-"):
                    sign = 1 if facing == "right" else -1
                    call += ["--fixture-airborne", f"1,190,{-90 * sign},-15",
                             "--expect-motion", "358"]
                if kind == "modified-ground":
                    call += ["--trace-geometry-frame", "401"]
                if kind.startswith("modified-wall-"):
                    sign = 1 if facing == "right" else -1
                    call += ["--fixture-airborne", f"1,340,{-90 * sign},-15",
                             "--expect-motion", "357", "--expect-motion", "358"]
                if kind.endswith("-damage"):
                    sign = 1 if facing == "right" else -1
                    placement = 180 if kind == "held-damage" else 220
                    opponent_x = -60 + 20 * sign if kind == "held-damage" else -50 if sign > 0 else -80
                    call += ["--fixture-airborne", f"2,{placement},{opponent_x},1",
                             "--expect-damage-port", "1",
                             "--expect-item-removed-on-damage", "96", "1"]
                if kind == "death-reuse":
                    sign = 1 if facing == "right" else -1
                    call += ["--fixture-airborne", f"1,700,{180*sign},-110",
                             "--fixture-airborne", f"1,1540,{180*sign},-110",
                             "--expect-motion", "357"]
                if kind in THROWS:
                    sign = 1 if facing == "right" else -1
                    call += ["--fixture-airborne", "1,180,-60,1",
                        "--fixture-airborne", f"2,180,{-60 + 20 * sign},1",
                        "--expect-motion", str(THROWS[kind]),
                        "--expect-damage-port", "2"]
                print(f"Comparing {name}", flush=True)
                result = subprocess.run(list(map(str, call)), cwd=ROOT,
                    capture_output=True, text=True, creationflags=subprocess.CREATE_NO_WINDOW)
                (output / f"{name}.log").write_text(result.stdout + result.stderr,
                                                    encoding="utf-8")
                observed = coverage(directory, name, kind) if result.returncode == 0 else []
                row = dict(case=name, exit=result.returncode, coverage=observed,
                    replay_hash=digest(replay), pass_case=result.returncode == 0 and
                    all(not entry["problems"] for entry in observed))
                receipt["cases"].append(row)
                print(f"{name}: {'PASS' if row['pass_case'] else 'FAIL'}", flush=True)
                save()
        receipt["inputs_unchanged"] = all(digest(Path(path)) == value
                                           for path, value in hashes.items())
        receipt["status"] = "PASS" if receipt["inputs_unchanged"] and receipt["cases"] and all(
            row["pass_case"] for row in receipt["cases"]) else "FAIL"
    except (RuntimeError, subprocess.TimeoutExpired) as error:
        receipt.update(status="FAIL", error=str(error))
    save()
    return int(receipt["status"] != "PASS")


if __name__ == "__main__":
    raise SystemExit(main())
