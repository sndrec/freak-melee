"""Compare owned Link/Young Link Bombs with hidden, accelerated, drawn matches.

Preparation can use --sdk-root and --dolphin-include to compile against frozen
headers while other work changes the live SDK. Retain the matching executable,
DLLs and ui directory separately; --prepared never recompiles packages.
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
KINDS = (("Link", "link", 58, 64, 24),
         ("Young Link", "younglink", 59, 68, 26))
MODES = ("ground", "air", "throw", "drop")
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
        if facing < 0 and 100 <= tick < 110:
            x = -50
        if tick == START:
            buttons, y = 1 << 9, -80
        if mode == "throw" and tick == 270:
            buttons, y = 1 << 9, -80
        if mode == "drop" and tick == 270:
            buttons = 1 << 4
        struct.pack_into("<Hbb", sample, 8, buttons, x, y)
        struct.pack_into("<Hbb", sample, 24, 0, 0, 0)
        frames.extend(sample)
    return template[:24] + frames


def exercise_coverage(path, mode, facing, kind):
    text = path.read_text(errors="replace")
    motion = 359 if mode == "air" else 358
    fighter_rows = re.findall(r"^\[fighter\] sample (\d+) P1 motion=(\d+) (.*)$",
                              text, re.M)
    expected_facing = "bf800000" if facing < 0 else "3f800000"
    if not any(int(sample) >= START and int(state) == motion and
               "facing=" + expected_facing + " " in detail
               for sample, state, detail in fighter_rows):
        raise AssertionError(f"{path}: requested Bomb pull/facing not exercised")
    states = [int(state) for state in re.findall(
        rf"^\[item\] sample \d+ I\d+ kind={kind} state=(\d+) ", text, re.M)]
    if 0 not in states or 5 not in states:
        raise AssertionError(f"{path}: held Bomb and explosion not both exercised")
    if mode == "throw" and 2 not in states:
        raise AssertionError(f"{path}: throw did not enter article state 2")
    if mode == "drop" and 3 not in states:
        raise AssertionError(f"{path}: drop did not enter article state 3")
    if mode == "air":
        transitions = [(int(before[1]), int(after[1]))
                       for before, after in zip(fighter_rows, fighter_rows[1:])]
        if (359, 358) not in transitions:
            raise AssertionError(f"{path}: airborne pull did not land into ground pull")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--case", action="append")
    parser.add_argument("--samples", type=int, default=750)
    parser.add_argument("--script", type=Path, default=ROOT / "examples/link_clink_wip.fms")
    parser.add_argument("--tool", type=Path, default=ROOT / "build/native/native_content_tool.exe")
    parser.add_argument("--exe", type=Path, default=ROOT / "build/native/melee_native.exe")
    parser.add_argument("--sdk-root", type=Path)
    parser.add_argument("--dolphin-include", type=Path)
    parser.add_argument("--compiler", type=Path,
                        default=Path("C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe"))
    parser.add_argument("--prepare-only", action="store_true")
    parser.add_argument("--prepared", action="store_true")
    args = parser.parse_args()
    if args.samples < 650:
        parser.error("At least 650 samples are required to cover fuse expiry")
    if bool(args.sdk_root) != bool(args.dolphin_include):
        parser.error("Frozen compilation needs both header paths")
    names = {f"{fighter}-{mode}-{'left' if facing < 0 else 'right'}"
             for _, fighter, _, _, _ in KINDS for mode in MODES for facing in (1, -1)}
    if args.case and set(args.case) - names:
        parser.error("Unknown case: " + ", ".join(sorted(set(args.case) - names)))
    batch = args.out_dir.resolve()
    batch.mkdir(parents=True, exist_ok=args.prepared or args.prepare_only)
    tool, iso = str(args.tool.resolve()), args.iso.resolve()
    image, store = batch / "portrait.png", batch / "store"
    if not args.prepared:
        portrait(image)
    packages = {}
    results = []
    for donor, fighter, kind, holding, endpoint in KINDS:
        if args.case and not any(name.startswith(fighter + "-") for name in args.case):
            continue
        project = batch / donor
        source = project / "behavior.fms"
        if not args.prepared:
            run(tool, "clone-stock", iso, donor, project, donor + " Bomb comparison")
            run(tool, "import-parameters", project, args.script.resolve(), source)
        parameters = (project / "fighter_parameters.fmp").read_text()
        for name, value in (("spin_effect_joint", endpoint), ("bomb_holding_joint", holding)):
            if not re.search(rf"^int\s+{name}\s*=\s*{value}\s*$", parameters, re.M):
                raise AssertionError(f"{donor}: expected owned {name}={value}")
        if re.search(r"^\s*attribute\s", source.read_text(), re.M):
            raise AssertionError(f"{donor}: DAT declarations survived import")
        if not args.prepared:
            if args.sdk_root:
                key = re.search(r"^code=(.*)$", (project / "project.txt").read_text(), re.M)[1]
                generated = project / "behavior.generated.c"
                run(tool, "script-c", source, key, generated)
                subprocess.run([str(args.compiler.resolve()), "-O2", "-shared", "-std=gnu11",
                                "-fuse-ld=lld-link", "-DMELEE_NATIVE=1", "-D_CRT_SECURE_NO_WARNINGS",
                                "-D_DLL", "-D_MT", "-I" + str(args.sdk_root.resolve()),
                                "-I" + str(args.dolphin_include.resolve()), str(generated),
                                "-o", str(project / "module.dll")], check=True)
            else:
                run(tool, "script-build", project, source)
        generated = (project / "behavior.generated.c").read_text()
        if "fighter->dat_attrs" in generated or "x4_specialAttributes" in generated:
            raise AssertionError(f"{donor}: generated Bomb has a donor attribute read")
        if args.prepared:
            package = (project / "comparison-package.txt").read_text().strip()
        else:
            package = bundled_id(run(tool, "--root", store, "bundle-project", project, image, iso))
            run(tool, "--root", store, "accept", package, iso)
            (project / "comparison-package.txt").write_text(package + "\n")
        packages[fighter] = package
        print(f"Prepared {donor}: holding joint {holding}, package {package}", flush=True)
    if args.prepare_only:
        return 0
    for donor, fighter, kind, holding, endpoint in KINDS:
        if fighter not in packages:
            continue
        for mode in MODES:
            for facing in (1, -1):
                name = f"{fighter}-{mode}-{'left' if facing < 0 else 'right'}"
                if args.case and name not in args.case:
                    continue
                inputs, directory = batch / (name + ".fmr"), batch / name
                inputs.write_bytes(recording(args.samples, mode, facing))
                command = [sys.executable, str(ROOT / "tools/run_fighter_parity.py"),
                           "--iso", str(iso), "--inputs", str(inputs), "--stock-fighter", fighter,
                           "--package-id", packages[fighter], "--content-root", str(store),
                           "--out-dir", str(directory), "--exe", str(args.exe.resolve()),
                           "--stocks", "2", "--expect-motion", "359" if mode == "air" else "358",
                           "--expect-item-kind", str(kind)]
                if mode == "air":
                    command += ["--fixture-airborne", "1,204,-55,5", "--expect-motion", "358"]
                elif mode == "throw" and facing < 0:
                    # Throw toward the floor instead of immediately beyond its
                    # left edge; retain the requested negative facing.
                    command += ["--fixture-airborne", "1,150,55,8"]
                elif mode == "drop":
                    # Neutral Z on the ground throws forward. In the air it
                    # takes the common item-release path, exercising dropped.
                    command += ["--fixture-airborne", "1,269,0,35"]
                print("Comparing " + name, flush=True)
                code = subprocess.run(command, cwd=ROOT, check=False).returncode
                if code == 0:
                    try:
                        for label in ("stock", "clone"):
                            exercise_coverage(directory / f"{name}-{label}.log", mode, facing, kind)
                    except AssertionError as error:
                        print(error, file=sys.stderr, flush=True)
                        code = 1
                results.append(dict(case=name, package=packages[fighter], samples=args.samples,
                                    owned_bomb_holding_joint=holding, exit=code))
                (batch / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    return int(not results or any(row["exit"] for row in results))


if __name__ == "__main__":
    raise SystemExit(main())
