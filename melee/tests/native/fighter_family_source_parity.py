"""Fresh interpreted family packages against stock CPU gameplay, including companions.

Each member uses the stock asset writer and normal parameter/source preparation.
The fixture writes the documented family manifest explicitly; this is not a test
of the Workshop's create-button transaction. Packages/actors use production paths.
"""

import argparse
import importlib.util
import json
from pathlib import Path
import re
import struct
import sys

from fighter_roster_source_parity import ROOT, execute, sha
from fighter_script_bundle_roundtrip import portrait


FAMILIES = (
    ("zelda", "alternate", ("Zelda", "Sheik"), ("zelda_full", "sheik_full")),
    ("sheik", "alternate", ("Sheik", "Zelda"), ("sheik_full", "zelda_full")),
    ("iceclimbers", "companion", ("Popo", "Nana"), ("popo_full", "nana_full")),
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--family", choices=[f[0] for f in FAMILIES], action="append")
    parser.add_argument("--samples", type=int)
    parser.add_argument("--scenario", choices=("cpu", "ground", "air"), default="cpu")
    parser.add_argument("--seed", type=lambda s: int(s, 0), default=0x1BADB002)
    args = parser.parse_args()
    if args.samples is None:
        args.samples = 1800 if args.scenario == "cpu" else 3200
    if args.scenario != "cpu" and (args.samples < 3200 or
                                  args.family and args.family != ["zelda"]):
        parser.error("Transform sequences require starting Zelda and at least 3200 samples")
    if args.samples < 360 or not args.iso.is_file():
        parser.error("Need an ISO and at least 360 samples beyond startup")
    out = args.out_dir.resolve()
    out.mkdir(parents=True, exist_ok=False)
    iso = args.iso.resolve()
    tool, exe = (ROOT / "build/native" / name for name in
                 ("native_content_tool.exe", "melee_native.exe"))
    selected = [f for f in FAMILIES if not args.family or f[0] in args.family]
    if args.scenario != "cpu":
        selected = [f for f in selected if f[0] == "zelda"]
    replay, image = out / "cpu-neutral.fmr", out / "portrait.png"
    header = struct.pack("<8sQII", b"FMREPLAY", 0, args.seed, 1)
    replay.write_bytes(header + bytes(80) * args.samples)
    special = None
    sequence_path = ROOT / "tests/native/zelda_sheik_owned_pair/runtime.py"
    if args.scenario != "cpu":
        spec = importlib.util.spec_from_file_location("family_sequences", sequence_path)
        special = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(special)
        data = bytearray(special.recording(special.sequence(args.scenario == "air"), args.samples))
        struct.pack_into("<I", data, 16, args.seed)
        replay.write_bytes(data)
    portrait(image)
    pins = [Path(__file__), tool, exe, iso, replay, image,
            ROOT / "tests/native/fighter_roster_source_parity.py",
            ROOT / "tests/native/fighter_script_bundle_roundtrip.py",
            ROOT / "tools/run_fighter_parity.py",
            ROOT / "tools/compare_fighter_parity.py"]
    pins += sorted({ROOT / "examples" / (name + ".fms")
                    for row in selected for name in row[3]})
    if special:
        pins.append(sequence_path)
    receipt = dict(status="RUNNING", samples=args.samples, seed=args.seed, scenario=args.scenario,
                   inputs={str(p): sha(p) for p in pins}, cases=[], limits=[
                       "Explicit family manifest; not Workshop create-button testing.",
                       "One seed; CPU9 on Final Destination or scripted transform/special sequence on Battlefield.",
                       "No exhaustive move/lifecycle, pixels/audio, complete state or rollback claim."])

    def save():
        (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")

    save()
    for launch, mode, members, templates in selected:
        directory = out / launch
        directory.mkdir()
        project, store = directory / "project", directory / "store"
        row = dict(family=launch, mode=mode, steps=[], coverage={}, status="RUNNING")
        receipt["cases"].append(row)

        def run(stage, command):
            result = execute(command, directory / (stage + ".log"))
            row["steps"].append(result)
            save()
            if result["exit"] != 0:
                raise ValueError(stage + " failed")

        try:
            print("Preparing family " + launch, flush=True)
            for index, (member, template) in enumerate(zip(members, templates)):
                member_path = project if index == 0 else project / "partner"
                source = directory / ("owned-" + str(index) + ".fms")
                run("clone-" + str(index), [tool, "clone-stock", iso, member,
                    member_path, "Roster " + launch])
                run("parameters-" + str(index), [tool, "import-parameters", member_path,
                    ROOT / "examples" / (template + ".fms"), source])
                run("build-" + str(index), [tool, "script-build", member_path, source])
                receipt["inputs"][str(source)] = sha(source)
            (project / "family.txt").write_text(
                "mode=" + mode + "\nprimary_key=main\npartner_key=partner\npartner=partner\n",
                newline="\n")
            run("package", [tool, "--root", store, "package-project", project, image])
            found = re.search(r"Cached source fighter ([0-9a-f]{64})",
                              (directory / "package.log").read_text(errors="replace"))
            if not found:
                raise ValueError("Missing package identity")
            identity = found.group(1)
            row["package"] = identity
            for stage in ("verify-package", "accept"):
                run(stage, [tool, "--root", store, stage, identity])
            for path in [*project.rglob("*"), store / "cache" / (identity + ".fmc")]:
                if path.is_file():
                    receipt["inputs"][str(path)] = sha(path)
            print("Comparing family " + launch, flush=True)
            result = execute([sys.executable, ROOT / "tools/run_fighter_parity.py",
                "--iso", iso, "--inputs", replay, "--stock-fighter", launch,
                "--package-id", identity, "--content-root", store, "--independent",
                "--p1-type", "cpu" if special is None else "human", "--cpu-level", "9",
                "--stocks", "4", "--stage", "final-destination" if special is None else "battlefield",
                "--p2", "human:zelda" if special is None else "human:mario",
                "--out-dir", directory / "game"], directory / "compare.log", timeout=360)
            row["steps"].append(result)
            for label in ("stock", "clone"):
                log = directory / "game" / (replay.stem + "-" + label + ".log")
                text = log.read_text(errors="replace")
                diagnostics = re.findall(
                    r"^\[script\] (?:Item \d+ function|Fighter \d+ event|Source \d+).*", text, re.M)
                live = re.findall(r"^\[script\] Live fighter .*", text, re.M)
                actors = {}
                for role in ("action", "companion-action"):
                    motions = re.findall(r"^\[" + role + r"\] sample \d+ P1 .*?motion=(\d+) ", text, re.M)
                    actors[role] = dict(motions=sorted(set(map(int, motions))), transitions=len(motions))
                full = f"[replay] End after {args.samples} controller samples." in text
                row["coverage"][label] = dict(actors=actors, diagnostics=diagnostics,
                    live=live, full_replay=full, log_sha256=sha(log))
                if special:
                    row["coverage"][label]["specials"] = special.coverage(
                        log, label == "clone", args.scenario == "air")
                if not full or diagnostics:
                    raise ValueError(label + " incomplete or faulted")
                if mode == "companion" and not actors["companion-action"]["transitions"]:
                    raise ValueError(label + " companion trace absent")
                if label == "clone" and (len(live) != 2 or any("faulted=0" not in v for v in live)):
                    raise ValueError("Both independent family member reports required without faults")
            if result["exit"] != 0:
                raise ValueError("Stock/source comparison failed")
            row["status"] = "PASS"
        except Exception as error:
            row.update(status="FAIL", error=str(error))
        print(launch + " " + row["status"], flush=True)
        save()
    receipt["changed_inputs"] = [p for p, digest in receipt["inputs"].items()
                                 if not Path(p).is_file() or sha(Path(p)) != digest]
    receipt["inputs_unchanged"] = not receipt["changed_inputs"]
    receipt["status"] = "PASS" if receipt["inputs_unchanged"] and all(
        row["status"] == "PASS" for row in receipt["cases"]) else "FAIL"
    save()
    print(json.dumps({"status": receipt["status"], "receipt": str(out / "receipt.json")}), flush=True)
    return int(receipt["status"] != "PASS")


if __name__ == "__main__":
    raise SystemExit(main())
