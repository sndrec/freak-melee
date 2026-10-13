"""Reusable Marth/Roy independence acceptance with explicit matching binaries.

Coordinate the runtime slot before running. Games are hidden, muted, fast and
drawn through the shared parity runner. --candidate adds independence only in
fresh isolated package source; final runs require the shipped template to own
independent behavior. --plan-only launches no tool or game; --prepare-only
builds the two isolated packages but launches no game.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import subprocess
import sys

from fighter_script_bundle_roundtrip import bundled_id, portrait

ROOT = Path(__file__).resolve().parents[2]
INPUTS = ROOT / "tests/native/parity_inputs"
MANIFESTS = ("sword-acceptance.json", "sword-side-counter-acceptance.json",
             "sword-common-policy-acceptance.json", "sword-cross-cpu-acceptance.json",
             "sword-owned-cpu-acceptance.json")
FOCUSED = (
    "neutral-ground-left-full", "neutral-ground-right-partial", "neutral-air-right-short",
    "up-ground-diagonal", "up-air-right", "up-contact",
    "side-ground-high", "side-ground-neutral", "side-ground-low",
    "side-air-high", "side-air-neutral", "side-air-low",
    "side-ground-early", "side-ground-attack",
    "counter-ground-laser", "counter-air-laser", "counter-ground-melee",
    "common-jab", "common-shield", "common-powershield", "common-wind",
    "cross-peach-counter", "cpu-final-destination", "cpu-battlefield")
LIFECYCLE = ("lifecycle-boost-land-repeat", "lifecycle-boost-death-repeat")
INDEPENDENT = re.compile(r"^\s*behavior independent\s*$", re.M)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def sample_count(raw):
    if raw[:8] != b"FMREPLAY" or len(raw) < 104 or (len(raw) - 24) % 80:
        raise ValueError("Unexpected input recording layout")
    return (len(raw) - 24) // 80


def accepted_catalog(manifest_root):
    """Use the exact accepted FMR, including fighter-specific corrected inputs."""
    result = {}
    for name in MANIFESTS:
        path = manifest_root / name
        for row in json.loads(path.read_text(encoding="utf-8"))["comparisons"]:
            if row["exit"] or row["fighter"] not in ("Marth", "Roy"):
                continue
            stock = ROOT / Path(row["stock"].replace("\\", "/"))
            stem = stock.name.removesuffix("-stock.log")
            candidates = [stock.parent.parent / (stem + ".fmr"), INPUTS / (stem + ".fmr")]
            recording = next((p.resolve() for p in candidates if p.is_file()), None)
            if recording is None:
                raise ValueError(f"Accepted recording missing for {row['fighter']} {row['case']}: {candidates}")
            # The last manifest replaces older CPU evidence with the owned-profile batch.
            result[(row["fighter"].lower(), row["case"])] = {
                "recording": recording, "acceptance_manifest": path.resolve(),
                "historical_stock_log": stock.resolve(), "historical_comparison": row["comparison"]}
    return result


def expectations(name, fighter):
    options = {"stage": "final-destination", "stocks": 2, "p2": "human:zelda",
               "p1_type": "human", "motions": [], "damage_ports": []}
    if name.startswith("neutral-") and name != "neutral-contact":
        _, mode, _, charge = name.split("-")
        options["motions"] = [345 if mode == "air" else 341]
        if charge != "short": options["motions"].append(346 if mode == "air" else 342)
        if mode == "ground": options["motions"].append(344 if charge == "full" else 343)
    elif name.startswith("up-"):
        options["motions"] = [368 if name.startswith("up-air-") else 367]
        if name == "up-contact": options["damage_ports"] = [2]
    elif name == "neutral-contact":
        options["motions"], options["damage_ports"] = [345, 347], [2]
    elif name.startswith("side-"):
        _, mode, route = name.split("-")
        options["motions"] = [358 if mode == "air" else 349]
        phases = {"high": [350, 352, 355], "neutral": [351, 353, 356], "low": [351, 354, 357]}
        if route != "early":
            follow = phases["neutral" if route == "attack" else route]
            options["motions"] += [x + 9 if mode == "air" else x for x in follow]
    elif name.startswith("counter-"):
        options["motions"] = [371 if name.startswith("counter-air-") else 369]
        if name.endswith("laser"):
            options["p2"] = "human:fox"
            options["motions"].append(370)
        elif name.endswith("melee"):
            options["motions"].append(370)
            options["damage_ports"] = [2]
    elif name.startswith("common-"):
        options["motions"] = {"common-jab": [44] if fighter == "roy" else [44, 45],
            "common-shield": [178, 179, 180], "common-powershield": [182, 179, 180],
            "common-wind": [341, 342, 343]}[name]
        if name == "common-wind": options["stage"] = "corneria"
    elif name.startswith("cpu-"):
        options.update(p1_type="cpu", stocks=4, damage_ports=[2])
        if name == "cpu-battlefield": options["stage"] = "battlefield"
    elif name == "cross-peach-counter":
        options.update(p2="human:peach", motions=[369, 370])
    elif name in LIFECYCLE:
        options["motions"] = [358]
        options["stocks"] = 3
    else:
        raise ValueError(f"No source-backed coverage contract for {name}")
    return options


def lifecycle_inputs(name):
    """Normal pad actions only; new trajectories stay unaccepted until gated."""
    raw = (INPUTS / "drmario-special-neutral-ground-inputs.fmr").read_bytes()
    sample_count(raw)
    frames = bytearray()
    count = 620 if name == LIFECYCLE[0] else 1050
    for frame in range(count):
        sample = bytearray(raw[24:104])
        for port in range(4):
            struct.pack_into("<HbbbbBB", sample, 8 + port * 16, 0, 0, 0, 0, 0, 0, 0)
        buttons, x, y = 0, 0, 0
        # Hold the first jump through squat, then double-jump near its apex.
        # A short first hop made Roy land inside the second side special,
        # which exercised its landing collision instead of common Landing.
        if 150 <= frame <= 155 or frame == 172: buttons = 1 << 10
        if frame in (195, 226): buttons, x = 1 << 9, 80
        if name == LIFECYCLE[0]:
            if 360 <= frame <= 365 or frame == 382: buttons = 1 << 10
            if frame in (405, 436): buttons, x = 1 << 9, 80
        else:
            # Walk off normally after consuming the air boost. Holding right
            # through the blast boundary ensures a genuine stock loss; no
            # damage, position, death or state is injected by the fixture.
            if 340 <= frame < 600: x = 100
            # Consume a fresh boost after leaving the stage, so the ensuing
            # death has a used boost and no intervening landing. RebirthWait
            # directly checks air-special input (ft_0D4D.c:305), allowing the
            # next fresh boost from the platform without a prior landing.
            if frame in (480, 800, 831): buttons, x = 1 << 9, 80
        struct.pack_into("<Hbb", sample, 8, buttons, x, y)
        frames.extend(sample)
    return raw[:24] + frames


def fighter_rows(text):
    return [(int(m[1]), m[2]) for m in re.finditer(r"^\[fighter\] sample (\d+) P1 (.*)$", text, re.M)]


def integer(row, field):
    match = re.search(rf"\b{field}=(\d+)", row)
    if not match: raise AssertionError(f"Missing {field} trace")
    return int(match[1])


def vector(row, field):
    match = re.search(rf"\b{field}=([0-9a-f]+),([0-9a-f]+),([0-9a-f]+)\b", row)
    if not match: raise AssertionError(f"Missing {field} trace")
    values = [struct.unpack(">f", bytes.fromhex(value))[0] for value in match.groups()]
    if not all(math.isfinite(value) for value in values): raise AssertionError("Nonfinite trajectory")
    return values


def coverage(log, name, fighter):
    text = log.read_text(encoding="utf-8", errors="replace")
    rows = fighter_rows(text)
    if not rows: raise AssertionError("No P1 fighter states")
    result = {}
    if name.startswith("neutral-") and name != "neutral-contact":
        direction = "bf800000" if "-left-" in name else "3f800000"
        if not any(integer(row, "motion") in (341, 345) and "facing=" + direction in row for _, row in rows):
            raise AssertionError("Neutral entry did not reach requested facing")
        result["entry_facing"] = direction
    if name.startswith("side-") and name.endswith("early"):
        if any(integer(row, "motion") in range(350, 358) or integer(row, "motion") in range(359, 367) for _, row in rows):
            raise AssertionError("Early side input advanced the chain")
        result["early_chain_rejected"] = True
    if name == "common-jab" and fighter == "marth":
        second = False; restarted = False
        for _, row in rows:
            motion = integer(row, "motion")
            if motion == 45: second = True
            if motion == 44 and second: restarted = True
        if not restarted: raise AssertionError("Marth did not restart jab1 after jab2")
        result["jab_restart"] = True
    if name == "common-wind":
        if not re.search(r"\bwind=[12](?: |$)", text, re.M):
            raise AssertionError("No actual global wind activation")
        result["global_wind"] = True
    if name == "cross-peach-counter":
        contacts = re.findall(r"^\[body-contact\] sample (\d+) P2 motion=360 other=P1 other_motion=(?:369|371)$", text, re.M)
        if not contacts: raise AssertionError("No inert Peach contact in the counter window")
        result["body_contact_samples"] = list(map(int, contacts))
    if name.startswith("cpu-"):
        active = 0
        for _, row in rows:
            stick = re.search(r"\bstick=([0-9a-f]+),([0-9a-f]+)\b", row)
            buttons = re.search(r"\bbuttons=([0-9a-f]+),([0-9a-f]+)\b", row)
            if not stick or not buttons: raise AssertionError("CPU controller fields missing")
            # Floating negative zero remains neutral input.
            active += any(int(x, 16) & 0x7FFFFFFF for x in stick.groups()) or any(int(x, 16) for x in buttons.groups())
        damage = [(float(m[1]), float(m[2])) for m in re.finditer(r"^\[damage\] sample \d+ P2 (\S+) -> (\S+)\s*$", text, re.M)]
        if not active or not any(after > before for before, after in damage):
            raise AssertionError("CPU lacked active inputs or actual opponent damage")
        result.update(active_input_samples=active, opponent_damage_events=len(damage))
    if name in LIFECYCLE:
        entries = []
        previous = None
        for sample, row in rows:
            motion = integer(row, "motion")
            if motion == 358 and motion != previous:
                if integer(row, "ga") != 1: raise AssertionError("Side entry was not airborne")
                entries.append((sample, vector(row, "vel")[1]))
            previous = motion
        expected_entries = 4 if name == LIFECYCLE[0] else 5
        if len(entries) != expected_entries:
            raise AssertionError(f"Expected {expected_entries} distinct air side entries, reached {len(entries)}")
        fresh = (0, 2) if name == LIFECYCLE[0] else (0, 2, 3)
        if not all(v > 0 if i in fresh else v <= 0 for i, (_, v) in enumerate(entries)):
            raise AssertionError(f"Fresh/consumed air boost pattern missing: {entries}")
        same_air = ((0, 1), (2, 3)) if name == LIFECYCLE[0] else ((0, 1), (3, 4))
        for left, right in same_air:
            if any(entries[left][0] < sample < entries[right][0] and integer(row, "ga") == 0 for sample, row in rows):
                raise AssertionError("Same-air boost reuse crossed a landing")
        left, right = (1, 2) if name == LIFECYCLE[0] else (2, 3)
        interval = [(sample, row) for sample, row in rows if entries[left][0] < sample < entries[right][0]]
        if name == LIFECYCLE[0]:
            if not any(integer(row, "motion") == 42 and integer(row, "ga") == 0 for _, row in interval):
                raise AssertionError("No common landing reset between the two jumps")
        else:
            deaths = [sample for sample, row in interval if integer(row, "motion") in range(11)]
            rebirth = [sample for sample, row in interval if integer(row, "motion") in (12, 13)]
            if not deaths or not rebirth or min(deaths) >= min(rebirth):
                raise AssertionError("No genuine death then rebirth between boost sequences")
            if any(sample < min(deaths) and integer(row, "ga") == 0 for sample, row in interval):
                raise AssertionError("Consumed offstage boost landed before death")
            # To prove the death reset rather than a later landing reset, the
            # surviving post-rebirth sequence must reach entry3 before Landing.
            if any(sample > min(rebirth) and integer(row, "motion") == 42 for sample, row in interval):
                raise AssertionError("Post-rebirth landing confounded the death-only reset gate")
            result.update(death_samples=deaths, rebirth_samples=rebirth)
        result["air_entries"] = [{"sample": sample, "vertical_velocity": velocity} for sample, velocity in entries]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--tool", type=Path, required=True)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--sdk", type=Path, help="Frozen headers; default TOOL/../.. /sdk")
    parser.add_argument("--clang", type=Path, help="Matching compiler; default TOOL/../.. /compiler/clang.exe")
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--manifest-root", type=Path, default=ROOT / ".local")
    parser.add_argument("--fighter", choices=("marth", "roy"), action="append")
    parser.add_argument("--suite", choices=("focused", "all-accepted", "lifecycle"), default="focused")
    parser.add_argument("--case", action="append", help="Restrict suite to named cases; repeat for a selected batch")
    parser.add_argument("--include-lifecycle", action="store_true")
    parser.add_argument("--candidate", action="store_true", help="Explicitly allow fixture-only independence before shipped promotion")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--plan-only", action="store_true")
    mode.add_argument("--prepare-only", action="store_true")
    args = parser.parse_args()
    iso, tool, exe, output = (getattr(args, key).resolve() for key in ("iso", "tool", "exe", "out_dir"))
    sdk = (args.sdk or tool.parent.parent / "sdk").resolve()
    clang = (args.clang or tool.parent.parent / "compiler/clang.exe").resolve()
    for path in (iso, tool, exe, clang, sdk / "src/melee/ft/native_fighter_module.h"):
        if not path.is_file(): parser.error(f"Missing file: {path}")
    fighters = args.fighter or ["marth", "roy"]
    if len(set(fighters)) != len(fighters): parser.error("Duplicate fighter selection")
    catalog = accepted_catalog(args.manifest_root.resolve())
    names = list(FOCUSED if args.suite == "focused" else LIFECYCLE if args.suite == "lifecycle" else
                 dict.fromkeys(name for _, name in catalog))
    if args.include_lifecycle: names += [name for name in LIFECYCLE if name not in names]
    if args.case:
        if len(set(args.case)) != len(args.case) or any(name not in names for name in args.case):
            parser.error("Duplicate case or case outside selected suite")
        names = [name for name in names if name in args.case]
    if not names: parser.error("Empty suite")
    templates = {fighter: ROOT / "examples" / (fighter + "_wip.fms") for fighter in fighters}
    for template in templates.values():
        if not INDEPENDENT.search(template.read_text(encoding="utf-8")) and not args.candidate:
            parser.error(f"Final acceptance requires shipped independent behavior: {template}; use explicit --candidate for unpromoted fixture source")
    output.mkdir(parents=True, exist_ok=False)
    paths = [tool, exe, clang, Path(__file__).resolve(), ROOT / "tools/run_fighter_parity.py",
             ROOT / "tools/compare_fighter_parity.py", ROOT / "tests/native/fighter_script_bundle_roundtrip.py",
             *templates.values(), *sorted((sdk / "src").rglob("*.h")),
             *sorted((sdk / "extern/dolphin/include").rglob("*.h"))]
    if any(name in LIFECYCLE for name in names): paths.append(INPUTS / "drmario-special-neutral-ground-inputs.fmr")
    monitored = {str(p): digest(p) for p in paths}
    plan = []
    for fighter in fighters:
        for name in names:
            destination = output / "inputs" / (fighter + "-" + name + ".fmr")
            destination.parent.mkdir(exist_ok=True)
            historical = None
            if name in LIFECYCLE:
                raw = lifecycle_inputs(name)
                provenance = {"status": "new trajectory; coverage pending", "generation": "normal pad actions only"}
            else:
                historical = catalog[(fighter, name)]
                raw = historical["recording"].read_bytes()
                provenance = {key: str(value) for key, value in historical.items()}
                monitored[str(historical["recording"])] = digest(historical["recording"])
                monitored[str(historical["acceptance_manifest"])] = digest(historical["acceptance_manifest"])
            destination.write_bytes(raw)
            plan.append({"fighter": fighter, "case": name, "recording": str(destination),
                         "recording_sha256": digest(destination), "samples": sample_count(raw),
                         "expectations": expectations(name, fighter), "provenance": provenance})
    plan_meta = {"mode": "candidate" if args.candidate else "shipped independent", "tool": str(tool), "exe": str(exe),
        "sdk": str(sdk), "clang": str(clang),
        "iso": {"path": str(iso), "size": iso.stat().st_size, "mtime_ns": iso.stat().st_mtime_ns},
        "input_sha256": monitored, "cases": plan,
        "runtime": {"hidden": True, "mute": True, "fast": True, "drawing": True},
        "remaining": "Results/demo construction has no match-replay coverage here; it needs a separate real scene/source acceptance. New lifecycle trajectories are unaccepted until both traces pass gates."}
    (output / "plan.json").write_text(json.dumps(plan_meta, indent=2) + "\n", encoding="utf-8", newline="\n")
    if args.plan_only:
        print(f"Prepared {len(plan)} cases; no tool or game launched")
        return 0

    def run(command, log):
        result = subprocess.run([str(x) for x in command], cwd=ROOT, capture_output=True, text=True)
        log.write_text(subprocess.list2cmdline([str(x) for x in command]) + "\n" + result.stdout + result.stderr,
                       encoding="utf-8", newline="\n")
        if result.returncode: raise RuntimeError(f"Exit {result.returncode}; inspect {log}")
        return result.stdout

    packages, results = {}, []
    try:
        image, store = output / "portrait.png", output / "store"
        portrait(image)
        for fighter in fighters:
            folder = output / fighter; folder.mkdir()
            project = folder / "project"
            run([tool, "clone-stock", iso, fighter.capitalize(), project, fighter.capitalize() + " Closure"], folder / "clone.log")
            source = project / "behavior.fms"
            run([tool, "import-parameters", project, templates[fighter], source], folder / "import.log")
            owned = source.read_text(encoding="utf-8")
            if not INDEPENDENT.search(owned):
                if not args.candidate: raise RuntimeError("Importer lost independent behavior declaration")
                source.write_text("# Explicit fixture-only independent candidate.\nfighter:\n    behavior independent\n\n" + owned,
                                  encoding="utf-8", newline="\n")
            # script-build embeds a live source include directory in the tool.
            # Use the actual compiler directly with immutable snapshot headers.
            code_key = re.search(r"(?m)^code=(\S+)\s*$", (project / "project.txt").read_text(encoding="utf-8"))
            if not code_key: raise RuntimeError("Missing copied-base code key")
            generated, module = project / "behavior.generated.c", project / "module.dll"
            run([tool, "script-c", source, code_key[1], generated], folder / "script-c.log")
            run([clang, "-std=gnu11", "-O2", "-shared", "-fuse-ld=lld-link", "-DMELEE_NATIVE=1",
                "-D_CRT_SECURE_NO_WARNINGS", "-D_DLL", "-D_MT", "-I" + str(sdk / "src"),
                "-I" + str(sdk / "extern/dolphin/include"), generated, "-o", module], folder / "module-build.log")
            # module-probe deliberately passes a null base; generated fighter
            # modules require the copied base. Bundle/verify supplies that base
            # and validates the module together with its owned parameters.
            package = bundled_id(run([tool, "--root", store, "bundle-project", project, image, iso], folder / "bundle.log"))
            run([tool, "--root", store, "verify-package", package, iso], folder / "verify.log")
            run([tool, "--root", store, "accept", package, iso], folder / "accept.log")
            packages[fighter] = {"package": package, "project": str(project),
                "sha256": {name: digest(project / name) for name in ("behavior.fms", "behavior.generated.c",
                    "fighter_parameters.fmp", "cpu_profile.fcp", "module.dll")}}
        (output / "packages.json").write_text(json.dumps(packages, indent=2) + "\n", encoding="utf-8", newline="\n")
        commands = []
        for case in plan:
            fighter, name = case["fighter"], case["case"]
            folder = output / (fighter + "-" + name)
            opts = case["expectations"]
            command = [sys.executable, ROOT / "tools/run_fighter_parity.py", "--iso", iso, "--exe", exe,
                "--inputs", case["recording"], "--stock-fighter", fighter, "--package-id", packages[fighter]["package"],
                "--content-root", store, "--out-dir", folder, "--independent", "--stage", opts["stage"],
                "--stocks", opts["stocks"], "--p2", opts["p2"], "--p1-type", opts["p1_type"], "--cpu-level", 9]
            for motion in opts["motions"]: command.extend(("--expect-motion", motion))
            for port in opts["damage_ports"]: command.extend(("--expect-damage-port", port))
            commands.append({"fighter": fighter, "case": name, "command": [str(x) for x in command],
                             "output": str(folder), "requires_post_run_coverage": True})
        (output / "commands.json").write_text(json.dumps(commands, indent=2) + "\n", encoding="utf-8", newline="\n")
        if args.prepare_only:
            print(f"Prepared {len(packages)} packages and {len(commands)} commands; no game launched", flush=True)
        else:
            for case, invocation in zip(plan, commands):
                fighter, name = case["fighter"], case["case"]
                folder = Path(invocation["output"]); folder.mkdir()
                command = invocation["command"]
                row = {"fighter": fighter, "case": name, "exit_code": 0, "package": packages[fighter]["package"],
                       "recording_sha256": case["recording_sha256"]}
                try:
                    run(command, folder / "parity.log")
                    stem = Path(case["recording"]).stem
                    row["coverage"] = {label: coverage(folder / (stem + "-" + label + ".log"), name, fighter)
                                       for label in ("stock", "clone")}
                except (RuntimeError, AssertionError, OSError) as error:
                    row.update(exit_code=1, error=str(error))
                results.append(row)
                (output / "results.json").write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8", newline="\n")
                print(f"{fighter} {name}: {'passed' if row['exit_code'] == 0 else row['error']}", flush=True)
        for path, expected in monitored.items():
            if digest(Path(path)) != expected: raise RuntimeError(f"Input changed during batch: {path}")
    except (RuntimeError, OSError) as error:
        results.append({"exit_code": 1, "error": str(error)})
        (output / "results.json").write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8", newline="\n")
        print(error, file=sys.stderr)
        return 1
    return int(not args.prepare_only and (len(results) != len(plan) or any(row["exit_code"] for row in results)))


if __name__ == "__main__":
    raise SystemExit(main())
