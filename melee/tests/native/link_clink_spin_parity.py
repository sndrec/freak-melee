"""Compare fully owned Link/Young Link Spin Attack in hidden, drawn matches.

Run manually with --iso and a fresh --out-dir. The runner retains imported
parameters, packaged modules, recordings and both traces. Game runs use the
parity runner's --hidden --fast defaults, with drawing enabled and recorded
controller input only. No foreground window or keypress automation is used.
"""

import argparse
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from fighter_script_bundle_roundtrip import bundled_id, portrait, run

ROOT = Path(__file__).resolve().parents[2]
KINDS = (("Link", "link", 24), ("Young Link", "younglink", 26))
MODES = ("ground", "air", "edge", "landing")
START = 205


def recording(samples, mode, facing):
    template = (ROOT / "tests/native/parity_inputs/"
                "drmario-special-neutral-ground-inputs.fmr").read_bytes()
    if template[:8] != b"FMREPLAY" or len(template) < 104:
        raise AssertionError("Unexpected controller recording format")
    frames = bytearray()
    for tick in range(samples):
        sample = bytearray(template[24:104])
        buttons, x, y = 0, 0, 0
        # Inputs before the ready countdown ends are suppressed. Turn before
        # fixture placement/entry and wait until the fighter can act again.
        if facing < 0 and 100 <= tick < 110:
            x = -50
        if mode == "edge" and START - 4 <= tick < START:
            x = 50 * facing
        if mode == "landing" and START < tick < START + 36:
            x = -80 * facing
        if tick == START:
            buttons, y = 1 << 9, 80
        struct.pack_into("<Hbb", sample, 8, buttons, x, y)
        struct.pack_into("<Hbb", sample, 24, 0, 0, 0)
        frames.extend(sample)
    return template[:24] + frames


def exercise_coverage(path, mode, facing):
    states = []
    for line in path.read_text(errors="replace").splitlines():
        match = re.match(r"^\[fighter\] sample (\d+) P1 motion=(\d+) (.*)$", line)
        if match:
            states.append((int(match[1]), int(match[2]), match[3]))
    entered = 356 if mode in ("ground", "edge") else 357
    spin = [row for row in states if row[0] >= START and row[1] == entered]
    expected_facing = "bf800000" if facing < 0 else "3f800000"
    if not spin or not any("facing=" + expected_facing + " " in row[2] for row in spin):
        raise AssertionError(f"{path}: requested Spin entry/facing was not exercised")
    if mode == "ground" and not any(row[0] > spin[-1][0] and row[1] == 14 for row in states):
        raise AssertionError(f"{path}: grounded Spin did not finish into Wait")
    transitions = [(previous[1], current[1], current[2])
                   for previous, current in zip(states, states[1:])
                   if previous[1] != current[1] and current[0] >= START]
    if mode == "edge":
        continuation = [state for state in transitions if state[:2] == (356, 357)]
        if not continuation:
            raise AssertionError(f"{path}: grounded Spin did not continue off the edge")
        frame = re.search(r"\bframe=([0-9a-f]{8})\b", continuation[0][2])
        if frame is None or struct.unpack(">f", bytes.fromhex(frame[1]))[0] <= 0:
            raise AssertionError(f"{path}: edge continuation reset its animation frame")
    if mode == "landing" and not any(state[:2] == (357, 43) for state in transitions):
        raise AssertionError(f"{path}: special landing was not reached directly from Spin")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--case", action="append")
    parser.add_argument("--samples", type=int, default=650)
    parser.add_argument("--tool", type=Path, default=ROOT / "build/native/native_content_tool.exe")
    parser.add_argument("--exe", type=Path, default=ROOT / "build/native/melee_native.exe")
    parser.add_argument("--prepare-only", action="store_true",
                        help="Build both owned packages without launching games")
    parser.add_argument("--prepared", action="store_true",
                        help="Run existing packages without rebuilding their modules")
    args = parser.parse_args()
    if args.samples < 400:
        parser.error("At least 400 samples are required for Spin completion")
    names = {f"{fighter}-{mode}-{'left' if facing < 0 else 'right'}"
             for _, fighter, _ in KINDS for mode in MODES for facing in (1, -1)}
    if args.case and set(args.case) - names:
        parser.error("Unknown case: " + ", ".join(sorted(set(args.case) - names)))
    iso = args.iso.resolve()
    batch = args.out_dir.resolve()
    batch.mkdir(parents=True, exist_ok=args.prepared)
    image, store = batch / "portrait.png", batch / "store"
    if not args.prepared:
        portrait(image)
    tool = str(args.tool.resolve())
    results = []
    packages = {}
    for donor, fighter, endpoint in KINDS:
        cases = [(mode, facing, f"{fighter}-{mode}-{'left' if facing < 0 else 'right'}")
                 for mode in MODES for facing in (1, -1)]
        cases = [case for case in cases if not args.case or case[2] in args.case]
        if not cases:
            continue
        project, source = batch / donor, batch / (fighter + ".owned.fms")
        if not args.prepared:
            run(tool, "clone-stock", iso, donor, project, donor + " Spin comparison")
            run(tool, "import-parameters", project, ROOT / "examples/link_clink_wip.fms", source)
        parameters = (project / "fighter_parameters.fmp").read_text()
        if not re.search(rf"^int\s+spin_effect_joint\s*=\s*{endpoint}\s*$", parameters, re.M):
            raise AssertionError(f"{donor}: stock import did not select owned endpoint {endpoint}")
        if re.search(r"^\s*attribute\s", source.read_text(), re.M):
            raise AssertionError(f"{donor}: draft DAT attributes survived owned import")
        if not args.prepared:
            run(tool, "script-build", project, source)
        generated = (project / "behavior.generated.c").read_text()
        if "fighter->dat_attrs" in generated:
            raise AssertionError(f"{donor}: built Spin still reads donor DAT attributes")
        if args.prepared:
            package = (project / "comparison-package.txt").read_text().strip()
        else:
            package = bundled_id(run(tool, "--root", store, "bundle-project", project, image, iso))
            run(tool, "--root", store, "accept", package, iso)
            (project / "comparison-package.txt").write_text(package + "\n")
        packages[fighter] = package
        print(f"Prepared {donor}: owned joint {endpoint}, package {package}", flush=True)
    if args.prepare_only:
        return 0
    for donor, fighter, endpoint in KINDS:
        if fighter not in packages:
            continue
        package = packages[fighter]
        cases = [(mode, facing, f"{fighter}-{mode}-{'left' if facing < 0 else 'right'}")
                 for mode in MODES for facing in (1, -1)]
        cases = [case for case in cases if not args.case or case[2] in args.case]
        for mode, facing, name in cases:
            inputs, directory = batch / (name + ".fmr"), batch / name
            inputs.write_bytes(recording(args.samples, mode, facing))
            command = [sys.executable, str(ROOT / "tools/run_fighter_parity.py"),
                       "--iso", str(iso), "--inputs", str(inputs),
                       "--stock-fighter", fighter, "--package-id", package,
                       "--content-root", str(store), "--out-dir", str(directory),
                       "--exe", str(args.exe.resolve()),
                       "--stocks", "2", "--expect-motion",
                       "356" if mode in ("ground", "edge") else "357"]
            if mode == "air":
                command += ["--fixture-airborne", "1,180,-55,70",
                            "--expect-motion", "35", "--expect-motion", "43"]
            elif mode == "landing":
                # Recover from just below the edge and drift back onto the
                # stage, reaching its floor before the 80-frame Spin ends.
                command += ["--fixture-airborne", f"1,204,{90 * facing},-18",
                            "--expect-motion", "43"]
            elif mode == "edge":
                # FD's floor ends at 85.565697. Four walk samples move 1.327
                # units before Spin; its remaining momentum must cross that
                # edge after entry. Coverage fails if these inputs miss it.
                command += ["--fixture-airborne", f"1,150,{83.8 * facing},8",
                            "--expect-motion", "357"]
            print("Comparing " + name, flush=True)
            code = subprocess.run(command, cwd=ROOT, check=False).returncode
            if code == 0:
                try:
                    for label in ("stock", "clone"):
                        exercise_coverage(directory / f"{name}-{label}.log", mode, facing)
                except AssertionError as error:
                    print(error, file=sys.stderr, flush=True)
                    code = 1
            results.append(dict(case=name, package=package, samples=args.samples,
                                owned_spin_effect_joint=endpoint, exit=code))
            (batch / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    return int(not results or any(row["exit"] for row in results))


if __name__ == "__main__":
    raise SystemExit(main())
