"""Independent Pikachu/Pichu acceptance using exact historically accepted FMRs.

Coordinate the game slot before execution. --plan-only uses no tools/games;
--prepare-only compiles isolated packages with frozen script-c and direct Clang.
Final mode requires shipped independence; --candidate changes only scratch FMS.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from fighter_script_bundle_roundtrip import bundled_id, portrait
from pikachu_pichu_costume_parity import independent_costume

ROOT = Path(__file__).resolve().parents[2]
INDEPENDENT = re.compile(r"^\s*behavior independent\s*$", re.M)
FOCUSED = (
    "jolt/ground-right", "jolt/air-left", "jolt/reflected", "jolt/absorbed",
    "skull/ground-full-charge", "skull/air-left-tap", "skull/ground-contact",
    "up/second-dash", "up/forced-turn", "up/ground-slope",
    "thunder/air-catch-left", "thunder/platform", "thunder/victim",
    "thunder/shield", "thunder/interrupted", "thunder/absorb",
    "policy/jab-repeat", "policy/forward-smash-contact", "policy/held-bat-up-special",
    "policy/cpu-idle-level4", "policy/cpu-battlefield-level9",
    "recovery/far-left", "recovery/near-right", "costume/color-1-thunder")
LIMITS = (
    "No live Thunder-chain stock loss/owner removal, auxiliary attachments or runtime instance-scale fixture is accepted.",
    "Source scale/lifecycle checks do not substitute for those missing gameplay trajectories.",
    "Skull wall/air contact/interrupted charge, Jolt pre-surface reflection/shield bounce, direct up-special wall/ceiling/ledge/startup interruption remain unproven.",
    "Costume suite checks startup/death-selector initialization; it does not exercise a second stock-loss/reset with live articles.",
    "No direct audio/pixel, results scene, Workshop or network rollback acceptance follows this runner.")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def samples(raw):
    if raw[:8] != b"FMREPLAY" or len(raw) < 104 or (len(raw) - 24) % 80:
        raise ValueError("Unexpected input recording layout")
    return (len(raw) - 24) // 80


def local_path(value):
    path = Path(value.replace("\\", "/"))
    return path if path.is_absolute() else ROOT / path


def catalog(manifest_root):
    result = {}
    for group in ("up", "jolt", "skull", "thunder", "policy", "variant-recovery"):
        manifest = manifest_root / ("pikachu-pichu-" + group + "-acceptance.json")
        data = json.loads(manifest.read_text(encoding="utf-8"))
        lists = (("costume", data["costume_comparisons"]), ("recovery", data["cpu_recovery_comparisons"])) if group == "variant-recovery" else ((group, data["cases"]),)
        for actual_group, rows in lists:
            for row in rows:
                full = row["case"]
                fighter, suffix = full.split("-", 1)
                if fighter not in ("pikachu", "pichu"):
                    continue
                if actual_group == "skull":
                    # Corrected tap gates accept comparison_exit0 even though
                    # the original obsolete end-of-tick Hold gate failed.
                    if row.get("comparison_exit") != 0:
                        continue
                elif row.get("exit", 0) != 0:
                    continue
                if "accepted_directory" in row:
                    directory = local_path(row["accepted_directory"])
                    recording = directory.parent / (full + ".fmr")
                    stock = directory / (full + "-stock.log")
                elif actual_group == "costume":
                    directory = manifest_root / "pikachu-pichu-costume-accepted"
                    recording = directory / (full + ".fmr")
                    stock = directory / full / (full + "-stock.log")
                elif actual_group == "recovery":
                    directory = manifest_root / "pikachu-pichu-recovery-first"
                    recording = directory / "cpu-recovery.fmr"
                    stock = directory / full / "cpu-recovery-stock.log"
                else:
                    directory = local_path(row["folder"]) if Path(row["folder"].replace("\\", "/")).is_absolute() else manifest_root / row["folder"]
                    recording = directory / (full + ".fmr")
                    stock = directory / full / (full + "-stock.log")
                # Manifest rows can include regressions of other moves; keep
                # only the actual group's declared behavior surface.
                if not supported(actual_group, suffix):
                    continue
                if not recording.is_file() or not stock.is_file():
                    raise ValueError(f"Accepted inputs/log missing: {actual_group}/{full}")
                actual_hash = digest(recording)
                expected = next((value for path, value in row.get("sha256", {}).items()
                                 if local_path(path).resolve() == recording.resolve()), None)
                if expected and expected != actual_hash:
                    raise ValueError(f"Historical recording changed: {recording}")
                result[(fighter, actual_group + "/" + suffix)] = {
                    "recording": str(recording.resolve()), "historical_stock_log": str(stock.resolve()),
                    "acceptance_manifest": str(manifest.resolve()), "recording_sha256": actual_hash,
                    "historical_row": row}
    return result


def supported(group, name):
    names = {
        "up": ("ground-up-right", "ground-up-left", "ground-horizontal-right", "ground-horizontal-left",
               "air-diagonal-right", "air-diagonal-left", "air-down", "second-dash", "ground-slope",
               "platform-down", "neutral-stick", "small-stick-left", "air-horizontal", "forced-turn"),
        "jolt": ("ground-right", "ground-left", "air-right", "air-left", "ground-slope", "repeated", "reflected", "absorbed"),
        "skull": ("ground-right-tap", "ground-left-tap", "air-right-tap", "air-left-tap", "ground-full-charge", "ground-contact", "ground-slope"),
        "thunder": ("ground-right", "ground-left", "air-right", "air-left", "air-catch-right", "air-catch-left", "repeat", "slope", "platform", "offstage", "victim", "shield", "interrupted", "absorb"),
        "policy": ("jab-repeat", "forward-smash-contact", "held-bat-up-special", "cpu-idle-level9", "cpu-shield-level9", "cpu-idle-level4", "cpu-battlefield-level9"),
        "recovery": ("far-left", "far-right", "near-left", "near-right"),
        "costume": tuple(f"color-{color}-{move}" for color in range(4) for move in ("thunder", "up-special")),
    }
    return name in names[group]


def options(fighter, case):
    group, name = case.split("/")
    opts = {"stage": "final-destination", "stocks": 2, "p2": "human:zelda",
            "p1_type": "human", "cpu_level": 9, "flags": [], "color": 0}
    flags = opts["flags"]
    def motions(*values):
        for value in values:
            flags.extend(("--expect-motion", str(value)))
    jolt = 89 if fighter == "pikachu" else 91
    thunder = 81 if fighter == "pikachu" else 82
    if group == "jolt":
        motions(342 if name.startswith("air") else 341)
        flags += ["--expect-item-kind", str(jolt), "--expect-item-kind", str(jolt + 1)]
        if name == "reflected":
            opts["p2"] = "human:fox"
            flags += ["--expect-item-owner-transfer", str(jolt + 1), "1", "2"]
        elif name == "absorbed":
            opts["p2"] = "human:ness"
            flags += ["--expect-item-event", "absorb", str(jolt + 1)]
        if name == "ground-slope": opts["stage"] = "yoshis-story"
    elif group == "skull":
        air = name.startswith("air")
        motions(348 if air else 343, 352 if air else 347, 350)
        if name in ("ground-full-charge", "ground-slope"): motions(344)
        if name == "ground-contact": flags += ["--expect-damage-port", "2"]
        if name == "ground-slope": opts["stage"] = "yoshis-story"
    elif group == "up":
        air = name.startswith("air") or name == "platform-down"
        motions(356 if air else 353)
        motions(354 if name in ("ground-horizontal-right", "ground-horizontal-left", "second-dash", "forced-turn") else 357)
        if name == "second-dash": motions(357)
        if name == "forced-turn": opts["p2"] = "human:mario"
        if name == "ground-slope": opts["stage"] = "yoshis-story"
        if name == "platform-down": opts["stage"] = "battlefield"
    elif group == "thunder":
        air = name.startswith("air") or name in ("platform", "offstage")
        motions(363 if air else 359)
        flags += ["--expect-item-kind", str(thunder)]
        if name not in ("offstage", "interrupted", "absorb", "platform"):
            motions(365 if name.startswith("air-catch") else 361)
        if name == "platform": motions(362); opts["stage"] = "battlefield"
        if name == "slope": opts["stage"] = "yoshis-story"
        if name in ("victim", "shield", "interrupted"): opts["p2"] = "human:mario"
        if name == "victim": flags += ["--expect-damage-port", "2"]
        if name == "shield": flags += ["--expect-port-motion", "2", "181"]
        if name == "interrupted": flags += ["--expect-damage-port", "1"]
        if name == "absorb":
            opts["p2"] = "human:ness"
            flags += ["--expect-item-event", "absorb", str(thunder)]
    elif group == "policy":
        if name.startswith("cpu"):
            opts.update(p1_type="cpu", stocks=4, cpu_level=4 if "level4" in name else 9)
            flags += ["--expect-damage-port", "2"]
            if "battlefield" in name: opts["stage"] = "battlefield"
        elif name == "jab-repeat": motions(44)
        elif name == "forward-smash-contact": motions(60); flags += ["--expect-damage-port", "2"]
        else:
            motions(92, 353)
            flags += ["--fixture-item", "11,120,-55,10", "--expect-item-kind", "11"]
    elif group == "recovery":
        distance, side = name.split("-")
        x, y = (175, -5) if distance == "far" else (110, -25)
        opts.update(p1_type="cpu", stocks=4)
        flags += ["--fixture-airborne", f"1,120,{-x if side == 'left' else x},{y}"]
        motions(356, 357, 358)
    elif group == "costume":
        opts["color"] = int(name.split("-")[1])
        if name.endswith("thunder"):
            motions(359, 361); flags += ["--expect-item-kind", str(thunder)]
        else: motions(353, 354, 357)
    return opts


def coverage(path, case, fighter):
    text = path.read_text(encoding="utf-8", errors="replace")
    group, name = case.split("/")
    rows = [(int(sample), int(motion), row) for sample, motion, row in re.findall(
        r"^\[fighter\] sample (\d+) P1 motion=(\d+) (.*)$", text, re.M)]
    if not rows: raise AssertionError("No traced P1 states")
    result = {}
    flags = options(fighter, case)["flags"]
    widths = {"--expect-motion": 1, "--expect-port-motion": 2, "--expect-damage-port": 1,
              "--expect-item-kind": 1, "--expect-item-event": 2, "--expect-item-owner-transfer": 3,
              "--fixture-item": 1, "--fixture-airborne": 1}
    cursor = 0
    while cursor < len(flags):
        flag = flags[cursor]; width = widths[flag]
        values = flags[cursor + 1:cursor + 1 + width]; cursor += width + 1
        found = True
        if flag in ("--expect-motion", "--expect-port-motion"):
            port, motion = ("1", values[0]) if width == 1 else values
            found = bool(re.search(rf"^\[action\] sample \d+ P{port} .*?\bmotion={motion}(?: |$)", text, re.M))
        elif flag == "--expect-damage-port":
            found = bool(re.search(rf"^\[damage\] sample \d+ P{values[0]} ", text, re.M))
        elif flag == "--expect-item-kind":
            found = bool(re.search(rf"^\[item\] sample \d+ I\d+ kind={values[0]} ", text, re.M))
        elif flag == "--expect-item-event":
            found = bool(re.search(rf"^\[item-event\] sample \d+ kind={values[1]} event={values[0]}$", text, re.M))
        elif flag == "--expect-item-owner-transfer":
            kind, source, target = values
            previous, found = {}, False
            for sample, index, owner in re.findall(rf"^\[item\] sample (\d+) I(\d+) kind={kind} .*? owner=P([0-4])(?: |$)", text, re.M):
                if previous.get(index) == (int(sample) - 1, source) and owner == target:
                    found = True; break
                previous[index] = (int(sample), owner)
        if not found: raise AssertionError(f"Missing coverage {flag} {' '.join(values)} in {path}")
    if (name.endswith("-left") or "-left-" in name) and name != "small-stick-left":
        starts = {"up": (353, 356), "skull": (343, 348), "jolt": (341, 342), "thunder": (359, 363)}
        if group in starts and not any(motion in starts[group] and "facing=bf800000" in row for _, motion, row in rows):
            raise AssertionError("Requested left-facing entry absent")
    if group == "up" and name == "forced-turn":
        turned = next((sample for sample, motion, row in rows if motion in (355, 358) and "facing=bf800000" in row), None)
        if turned is None or not any(sample > turned and motion in (354, 357) and "facing=3f800000" in row for sample, motion, row in rows):
            raise AssertionError("Cape recovery turn followed by rightward dash absent")
        result["turned_recovery_sample"] = turned
    if group == "thunder" and name == "interrupted":
        if not any(300 < sample < 400 and 75 <= motion <= 91 for sample, motion, _ in rows):
            raise AssertionError("Opponent interruption absent; self-damage is insufficient")
        result["opponent_interrupt"] = True
    if group == "policy" and name == "jab-repeat":
        frames = [struct.unpack(">f", bytes.fromhex(re.search(r"\bframe=([0-9a-f]{8})", row)[1]))[0]
                  for _, motion, row in rows if motion == 44]
        restarts = sum(b < a for a, b in zip(frames, frames[1:]))
        if restarts < 2 or any(motion == 45 for _, motion, _ in rows):
            raise AssertionError("First-jab repetition absent or wrong second jab")
        result["jab_restarts"] = restarts
    if group == "policy" and name.startswith("cpu"):
        actions = [motion for _, motion, _ in rows if motion >= 341 or 44 <= motion <= 74]
        if len(set(actions)) < 2: raise AssertionError("No meaningful CPU attack activity")
        result["attack_motions"] = sorted(set(actions))
    if group == "recovery":
        decisions = re.findall(r"^\[cpu-recovery\] sample=(\d+) P1 from=([^\n]+)", text, re.M)
        if not decisions: raise AssertionError("No actual CPU recovery decision")
        coords = re.match(r"([-\d.]+),[-\d.]+ target=([-\d.]+),", decisions[0][1])
        if coords is None: raise AssertionError("Missing CPU recovery positions")
        distance = abs(float(coords[1]) - float(coords[2]))
        if (distance > 60) != name.startswith("far"): raise AssertionError("Wrong recovery distance branch")
        dashes = len(re.findall(r"^\[action\] sample (?:1[2-9]\d|2[0-4]\d) P1 .*?motion=357 ", text, re.M))
        if dashes < 2: raise AssertionError("Both recovery dashes absent")
        result.update(decisions=decisions, horizontal_distance=distance, initial_dash_entries=dashes)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("iso", "tool", "exe", "out-dir"): parser.add_argument("--" + name, required=True, type=Path)
    parser.add_argument("--sdk", type=Path)
    parser.add_argument("--clang", type=Path)
    parser.add_argument("--manifest-root", type=Path, default=ROOT / ".local")
    parser.add_argument("--fighter", choices=("pikachu", "pichu"), action="append")
    parser.add_argument("--suite", choices=("focused", "all-accepted", "costume"), default="focused")
    parser.add_argument("--case", action="append", help="group/name, repeat to filter selected suite")
    parser.add_argument("--candidate", action="store_true")
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--plan-only", action="store_true")
    modes.add_argument("--prepare-only", action="store_true")
    args = parser.parse_args()
    tool, exe, iso, output = (getattr(args, name).resolve() for name in ("tool", "exe", "iso", "out_dir"))
    snapshot = tool.parent.parent
    sdk = (args.sdk or snapshot / "sdk").resolve()
    clang = (args.clang or snapshot / "compiler/clang.exe").resolve()
    freeze = snapshot / "freeze.json"
    for path in (tool, exe, iso, clang, freeze):
        if not path.is_file(): parser.error(f"Missing {path}")
    frozen = json.loads(freeze.read_text())
    # All immutable snapshot headers and executables must match their receipt.
    for path in (tool, exe, clang, *sdk.rglob("*.h")):
        try: key = str(path.relative_to(snapshot))
        except ValueError: parser.error(f"Not inside matching snapshot: {path}")
        if frozen.get(key) != digest(path): parser.error(f"Snapshot mismatch: {path}")
    fighters = args.fighter or ["pikachu", "pichu"]
    if len(set(fighters)) != len(fighters): parser.error("Duplicate fighter")
    accepted = catalog(args.manifest_root.resolve())
    names = list(FOCUSED) if args.suite == "focused" else sorted({name for _, name in accepted if (args.suite != "costume" or name.startswith("costume/"))})
    if args.case:
        if len(set(args.case)) != len(args.case) or any(name not in names for name in args.case): parser.error("Duplicate/unknown case")
        names = [name for name in names if name in args.case]
    if not names: parser.error("Empty suite")
    output.mkdir(parents=True, exist_ok=False)
    plan, monitored = [], {str(p): digest(p) for p in (tool, exe, clang, freeze, ROOT / "examples/pikachu_wip.fms", ROOT / "examples/pichu_wip.fms", Path(__file__),
        ROOT / "tools/run_fighter_parity.py", ROOT / "tools/compare_fighter_parity.py",
        ROOT / "tests/native/pikachu_pichu_costume_parity.py", ROOT / "tests/native/fighter_script_bundle_roundtrip.py")}
    for fighter in fighters:
        for name in names:
            historical = accepted[(fighter, name)]
            recording = Path(historical["recording"])
            path = output / "inputs" / (fighter + "-" + name.replace("/", "-") + ".fmr")
            path.parent.mkdir(exist_ok=True)
            path.write_bytes(recording.read_bytes())
            for key in ("recording", "acceptance_manifest", "historical_stock_log"):
                monitored[historical[key]] = digest(Path(historical[key]))
            # Replay the new supplemental gates against accepted historical
            # traces now, without confusing that with fresh runtime parity.
            historical_stock = Path(historical["historical_stock_log"])
            historical_clone = historical_stock.with_name(historical_stock.name.removesuffix("-stock.log") + "-clone.log")
            historic_coverage = {label: coverage(path, name, fighter) for label, path in
                                (("stock", historical_stock), ("clone", historical_clone))}
            monitored[str(historical_clone)] = digest(historical_clone)
            plan.append({"fighter": fighter, "case": name, "recording": str(path),
                "sha256": digest(path), "samples": samples(path.read_bytes()), "options": options(fighter, name),
                "provenance": historical, "historical_supplemental_coverage": historic_coverage})
    meta = {"mode": "isolated candidate" if args.candidate else "shipped independent", "tool": str(tool),
        "exe": str(exe), "sdk": str(sdk), "clang": str(clang), "input_sha256": monitored, "cases": plan,
        "runtime": {"hidden": True, "muted": True, "fast": True, "drawing": True}, "unproven": LIMITS}
    (output / "plan.json").write_text(json.dumps(meta, indent=2) + "\n")
    if args.plan_only:
        print(f"Planned {len(plan)} pairs / {sum(row['samples'] for row in plan)} samples; no tool/game launched")
        return 0

    def run(command, log):
        command = [str(value) for value in command]
        done = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        log.write_text(subprocess.list2cmdline(command) + "\n" + done.stdout + done.stderr)
        if done.returncode: raise RuntimeError(f"Exit {done.returncode}: {log}")
        return done.stdout

    results, packages, commands = [], {}, []
    try:
        image, store = output / "portrait.png", output / "store"
        portrait(image)
        for fighter in fighters:
            template = ROOT / "examples" / (fighter + "_wip.fms")
            if not args.candidate and not INDEPENDENT.search(template.read_text()):
                parser.error("Shipped independent fighter source required")
            folder = output / fighter; folder.mkdir()
            project = folder / "project"
            run([tool, "clone-stock", iso, fighter.capitalize(), project, fighter + " Closure"], folder / "clone.log")
            source = project / "behavior.fms"
            run([tool, "import-parameters", project, template, source], folder / "import.log")
            (folder / "imported.fms").write_bytes(source.read_bytes())
            if not INDEPENDENT.search(source.read_text()):
                if not args.candidate: raise RuntimeError("Importer lost independence")
                source.write_text("# Explicit fixture-only independent candidate\nfighter:\n    behavior independent\n\n" + source.read_text())
            key = re.search(r"(?m)^code=(\S+)\s*$", (project / "project.txt").read_text())
            if not key: raise RuntimeError("Missing copied-base code key")
            generated, module = project / "behavior.generated.c", project / "module.dll"
            run([tool, "script-c", source, key[1], generated], folder / "script-c.log")
            compile_command = [clang, "-std=gnu11", "-O2", "-ffp-contract=off", "-shared", "-fuse-ld=lld-link", "-DMELEE_NATIVE=1",
                 "-D_CRT_SECURE_NO_WARNINGS", "-D_DLL", "-D_MT", "-I" + str(sdk / "src"),
                 "-I" + str(sdk / "extern/dolphin/include"), generated, "-o", module]
            setup = Path("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat")
            compile_script = folder / "compile.cmd"
            compile_script.write_text("@echo off\ncall " + subprocess.list2cmdline([str(setup), "-vcvars_ver=14.44"]) +
                " >nul\n" + subprocess.list2cmdline([str(x) for x in compile_command]) + "\n")
            run(["cmd", "/c", compile_script], folder / "module-build.log")
            package = bundled_id(run([tool, "--root", store, "bundle-project", project, image, iso], folder / "bundle.log"))
            run([tool, "--root", store, "verify-package", package, iso], folder / "verify.log")
            run([tool, "--root", store, "accept", package, iso], folder / "accept.log")
            skins = {}
            for color in sorted({row["options"]["color"] for row in plan if row["fighter"] == fighter} - {0}):
                original, custom = folder / f"retail-{color}.dat", folder / f"skin-{color}.dat"
                run([tool, "extract-stock", iso, fighter.capitalize(), color, original], folder / f"extract-{color}.log")
                custom.write_bytes(independent_costume(original.read_bytes(), fighter.capitalize(), ("Re", "Bu", "Gr")[color - 1]))
                response = run([tool, "--root", store, "import-custom-costume", custom, package, fighter + " variant", color], folder / f"skin-{color}.log")
                skins[color] = re.search(r"ID: ([a-f0-9]{64})", response)[1]
            packages[fighter] = {"package": package, "skins": skins, "project": str(project),
                "sha256": {str(path.relative_to(folder)): digest(path) for path in folder.rglob("*") if path.is_file()}}
        (output / "packages.json").write_text(json.dumps(packages, indent=2) + "\n")
        for row in plan:
            fighter, case, opts = row["fighter"], row["case"], row["options"]
            folder = output / (fighter + "-" + case.replace("/", "-"))
            command = [sys.executable, ROOT / "tools/run_fighter_parity.py", "--exe", exe, "--iso", iso,
                "--inputs", row["recording"], "--stock-fighter", fighter, "--package-id", packages[fighter]["package"],
                "--content-root", store, "--out-dir", folder, "--independent", "--stage", opts["stage"],
                "--stocks", opts["stocks"], "--p2", opts["p2"], "--p1-type", opts["p1_type"],
                "--cpu-level", opts["cpu_level"], "--stock-color", opts["color"], *opts["flags"]]
            if opts["color"]: command += ["--clone-skin", packages[fighter]["skins"][opts["color"]]]
            commands.append({"fighter": fighter, "case": case, "command": [str(x) for x in command], "output": str(folder), "post_run_coverage_required": True})
        (output / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        if args.prepare_only:
            print(f"Prepared {len(packages)} packages / {len(commands)} pairs; no game launched")
        else:
            for row, invocation in zip(plan, commands):
                folder = Path(invocation["output"]); folder.mkdir()
                result = {"fighter": row["fighter"], "case": row["case"], "samples": row["samples"], "exit_code": 0,
                          "package": packages[row["fighter"]]["package"], "recording_sha256": row["sha256"]}
                try:
                    run(invocation["command"], folder / "parity.log")
                    stem = Path(row["recording"]).stem
                    result["coverage"] = {label: coverage(folder / (stem + "-" + label + ".log"), row["case"], row["fighter"]) for label in ("stock", "clone")}
                    if row["case"].startswith("recovery/") and result["coverage"]["stock"]["decisions"] != result["coverage"]["clone"]["decisions"]:
                        raise AssertionError("Different CPU recovery decisions")
                except (RuntimeError, AssertionError, OSError) as error:
                    result.update(exit_code=1, error=str(error))
                results.append(result)
                (output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
                print(f"{row['fighter']} {row['case']}: exit {result['exit_code']}", flush=True)
        for path, expected in monitored.items():
            if digest(Path(path)) != expected: raise RuntimeError(f"Input changed: {path}")
    except (RuntimeError, AssertionError, OSError) as error:
        results.append({"exit_code": 1, "error": str(error)})
        (output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
        print(error, file=sys.stderr)
        return 1
    return int(not args.prepare_only and (len(results) != len(plan) or any(row["exit_code"] for row in results)))


if __name__ == "__main__":
    raise SystemExit(main())
