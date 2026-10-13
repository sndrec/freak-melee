"""Focused independent Fox/Falco Fire and active CPU parity recordings.

Coordinate the runtime slot before running. The shared runner keeps games
hidden, muted and fast, with drawing enabled. --prepare-only creates isolated
packages/recordings without launching games. Existing evidence is never reused
as an output directory. These paths do not cover every special or CPU branch.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from fighter_script_bundle_roundtrip import bundled_id, portrait, run

ROOT = Path(__file__).resolve().parents[2]
INPUTS = ROOT / "tests/native/parity_inputs"
CASES = ("fire-ground-left", "fire-ground-right", "fire-air-left",
         "fire-air-right", "cpu-level4", "cpu-level9")
INDEPENDENT = re.compile(r"^\s*behavior independent\s*$", re.M)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def sample_count(recording):
    data = recording.read_bytes()
    if data[:8] != b"FMREPLAY" or len(data) < 104 or (len(data) - 24) % 80:
        raise AssertionError(f"Unexpected input recording: {recording}")
    return (len(data) - 24) // 80


def cpu_recording():
    # Identical to the accepted baseline17 preparation: 1,800 neutral samples,
    # retaining frame metadata and clearing all four controller payloads.
    base = (INPUTS / "fighter-left-neutral-inputs.fmr").read_bytes()
    if base[:8] != b"FMREPLAY" or len(base) < 104:
        raise AssertionError("Unexpected base recording")
    frames = bytearray()
    for _ in range(1800):
        frame = bytearray(base[24:104])
        for port in range(4):
            struct.pack_into("<HbbbbBB", frame, 8 + port * 16, 0, 0, 0, 0, 0, 0, 0)
        frames.extend(frame)
    return base[:24] + frames


def cpu_coverage(log):
    text = log.read_text(encoding="utf-8", errors="replace")
    active = 0
    for row in re.finditer(r"^\[fighter\] sample \d+ P1 (.*)$", text, re.M):
        sticks = re.search(r"\bstick=([0-9a-f]+),([0-9a-f]+)\b", row[1])
        buttons = re.search(r"\bbuttons=([0-9a-f]+),([0-9a-f]+)\b", row[1])
        if not sticks or not buttons:
            raise AssertionError(f"{log}: missing CPU controller trace fields")
        # A sign-bit-only negative zero is still neutral input.
        if any(int(value, 16) & 0x7fffffff for value in sticks.groups()) or \
                any(int(value, 16) for value in buttons.groups()):
            active += 1
    damage = []
    for row in re.finditer(r"^\[damage\] sample \d+ P2 (\S+) -> (\S+)\s*$", text, re.M):
        before, after = map(float, row.groups())
        if after > before:
            damage.append(after)
    if not active:
        raise AssertionError(f"{log}: CPU produced no active controller input")
    if not damage:
        raise AssertionError(f"{log}: CPU inflicted no damage on the neutral opponent")
    return dict(active_input_samples=active, damage_events=len(damage),
                peak_opponent_damage=max(damage))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--tool", type=Path, default=ROOT / "build/native/native_content_tool.exe")
    parser.add_argument("--exe", type=Path, default=ROOT / "build/native/melee_native.exe")
    parser.add_argument("--fighter", type=str.lower, choices=("fox", "falco"), action="append")
    parser.add_argument("--case", choices=CASES, action="append")
    parser.add_argument("--prepare-only", action="store_true")
    args = parser.parse_args()
    iso, tool, exe, batch = (path.resolve() for path in
                            (args.iso, args.tool, args.exe, args.out_dir))
    for path in (iso, tool, exe):
        if not path.is_file():
            parser.error(f"Missing file: {path}")
    selected_fighters = [name for name in ("fox", "falco") if not args.fighter or name in args.fighter]
    selected_cases = [name for name in CASES if not args.case or name in args.case]
    batch.mkdir(parents=True, exist_ok=False)
    image = batch / "portrait.png"
    portrait(image)
    cpu = batch / "cpu.fmr"
    cpu.write_bytes(cpu_recording())
    packages, planned = {}, []
    files = [Path(__file__), tool, exe, ROOT / "tools/run_fighter_parity.py",
             ROOT / "tools/compare_fighter_parity.py", cpu]
    for name in selected_fighters:
        fighter = name.title()
        template = ROOT / "examples" / (name + "_wip.fms")
        if not INDEPENDENT.search(template.read_text()):
            raise AssertionError(f"{fighter} template must declare behavior independent")
        files.append(template)
        project, store, owned = batch / fighter, batch / (fighter + "-store"), batch / (fighter + "-owned.fms")
        run(str(tool), "clone-stock", iso, fighter, project, fighter + " independent closure comparison")
        run(str(tool), "import-parameters", project, template, owned)
        if not INDEPENDENT.search(owned.read_text()):
            raise AssertionError(f"Imported {fighter} script lost independent behavior")
        run(str(tool), "script-build", project, owned)
        package = bundled_id(run(str(tool), "--root", store, "bundle-project", project, image, iso))
        run(str(tool), "--root", store, "accept", package, iso)
        (project / "package.txt").write_text(package + "\n")
        packages[fighter] = dict(package=package, project=str(project), store=str(store))
        files.extend((owned, project / "behavior.fms", project / "behavior.generated.c",
                      project / "fighter_parameters.fmp", project / "module.dll", project / "project.txt"))
        for case in selected_cases:
            is_cpu = case.startswith("cpu-")
            record = cpu if is_cpu else INPUTS / ("fighter-" + case + "-inputs.fmr")
            command = [sys.executable, str(ROOT / "tools/run_fighter_parity.py"),
                       "--exe", str(exe), "--iso", str(iso), "--inputs", str(record),
                       "--stock-fighter", fighter, "--package-id", package,
                       "--content-root", str(store), "--out-dir", str(batch / (fighter + "-" + case)),
                       "--stocks", "3", "--independent"]
            if is_cpu:
                command += ["--p1-type", "cpu", "--cpu-level", case.removeprefix("cpu-level"),
                            "--expect-damage-port", "2"]
            else:
                command += ["--expect-motion", "355" if "ground" in case else "356"]
            planned.append(dict(fighter=fighter, case=case, package=package,
                                input=str(record), samples=sample_count(record), command=command))
            files.append(record)
    manifest = dict(independent=True, iso=str(iso), packages=packages, cases=planned,
                    files={str(path): digest(path) for path in dict.fromkeys(files)})
    (batch / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    if args.prepare_only:
        print("Prepared independent Fox/Falco packages and recordings: " + str(batch))
        return 0
    results = []
    for row in planned:
        fighter, case = row["fighter"], row["case"]
        directory = batch / (fighter + "-" + case)
        log = batch / (fighter + "-" + case + "-result.log")
        with log.open("wb") as stream:
            code = subprocess.run(row["command"], cwd=ROOT, stdout=stream,
                                  stderr=subprocess.STDOUT).returncode
        coverage = {}
        if code == 0 and case.startswith("cpu-"):
            try:
                for label in ("stock", "clone"):
                    coverage[label] = cpu_coverage(directory / (Path(row["input"]).stem + "-" + label + ".log"))
            except AssertionError as error:
                with log.open("a") as stream:
                    stream.write(str(error) + "\n")
                code = 1
        results.append(dict(fighter=fighter, case=case, exit_code=code, package=row["package"],
                            samples=row["samples"], input=row["input"], log=str(log), coverage=coverage))
        (batch / "results.json").write_text(json.dumps(results, indent=2) + "\n")
        print(f"{fighter} {case}: exit {code}", flush=True)
    return int(any(row["exit_code"] for row in results))


if __name__ == "__main__":
    raise SystemExit(main())
