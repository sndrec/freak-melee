"""Finish stock/source matches and admit actual owned result actors as a batch.

The offline airborne fixture places the losing slot below the blast zone. The
ordinary death, match-end, results and script teardown paths then run normally.
Result creation/scene transitions are checked separately from gameplay traces;
this does not compare rendered pixels, audio or all presentation branches.
"""
import argparse
import json
from pathlib import Path
import re
import struct
import sys

from fighter_roster_source_parity import FIGHTERS, ROOT, execute, sha
from fighter_family_source_parity import FAMILIES
from fighter_script_bundle_roundtrip import portrait


BASES = tuple((launch, "single", (stock,), (template,))
              for stock, launch, template in FIGHTERS) + FAMILIES
OUTCOMES = (("win1", 0, 0x200), ("win2", 1, 0x800),
            ("win3", 2, 0x400), ("lose", 4, 0))
SAMPLES = 900


def recording(path, button):
    data = bytearray(struct.pack("<8sQII", b"FMREPLAY", 0, 3, 1))
    for sample in range(SAMPLES):
        pads = bytearray(64)
        # After the KO, held B/X/Y select the three ordinary victory variants.
        if sample >= 200:
            struct.pack_into("<H", pads, 0, button)
        data.extend(bytes(8) + pads + bytes(8))
    path.write_bytes(data)


def coverage(path, source, mode, variant):
    text = path.read_text(errors="replace")
    scenes = re.findall(r"^\[scene\] (enter|exit) kind=(\d+) sample=(\d+)(?: terminal=(\d+))?", text, re.M)
    results = re.findall(r"^\[results\] sample (\d+) P(\d+) variant=(\d+) models=(\d+)$", text, re.M)
    errors = []
    expected_models = 2 if mode == "companion" else 1
    if not any(port == "1" and int(value) == variant and int(models) == expected_models
               for _, port, value, models in results):
        errors.append("Expected P1 result variant/models absent")
    if not any(port == "2" and models == "1" for _, port, _, models in results):
        errors.append("Reference opponent result model absent")
    if [int(kind) for direction, kind, _, _ in scenes if direction == "enter"] != [2, 5]:
        errors.append("Expected versus -> results transition absent")
    if not any(direction == "exit" and kind == "2" and terminal == "0"
               for direction, kind, _, terminal in scenes):
        errors.append("Versus scene did not finish normally")
    faults = re.findall(r"^\[script\] (?:Item \d+ function|Fighter \d+ event|Source \d+).*", text, re.M)
    if faults or "Owned results unavailable:" in text or "[replay] Nonfinite" in text:
        errors.append("Source fault, unavailable result or nonfinite pose")
    marker = re.search(r"^\[scene\] enter kind=5 .*", text, re.M)
    before = text[:marker.start()] if marker else text
    after = text[marker.end():] if marker else ""
    reports = [list(map(int, re.findall(r"^\[script\] Live fighter .*?faulted=(\d+)", part, re.M)))
               for part in (before, after)]
    if source and (list(map(len, reports)) != [1 if mode == "single" else 2, expected_models]
                   or any(any(values) for values in reports)):
        errors.append("Expected fault-free gameplay/result script owners absent")
    if f"[replay] End after {SAMPLES} controller samples." not in text:
        errors.append("Replay incomplete")
    return dict(scenes=scenes, results=results, reports=reports, faults=faults,
                log_sha256=sha(path)), errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--fighter", choices=[row[0] for row in BASES], action="append")
    parser.add_argument("--outcome", choices=[row[0] for row in OUTCOMES], action="append")
    args = parser.parse_args()
    out, iso = args.out_dir.resolve(), args.iso.resolve()
    if out.drive.upper() != "A:" or out.exists() or not iso.is_file():
        parser.error("Need an ISO and fresh private A: output directory")
    out.mkdir(parents=True)
    selected = [row for row in BASES if not args.fighter or row[0] in args.fighter]
    outcomes = [row for row in OUTCOMES if not args.outcome or row[0] in args.outcome]
    tool, exe = (ROOT / "build/native" / name for name in
                 ("native_content_tool.exe", "melee_native.exe"))
    image, store = out / "portrait.png", out / "store"
    portrait(image)
    for name, _, button in outcomes:
        recording(out / (name + ".fmr"), button)
    pins = [Path(__file__), tool, exe, iso, image,
            ROOT / "tests/native/fighter_roster_source_parity.py",
            ROOT / "tests/native/fighter_family_source_parity.py",
            ROOT / "tests/native/fighter_script_bundle_roundtrip.py",
            ROOT / "tools/run_fighter_parity.py", ROOT / "tools/compare_fighter_parity.py",
            ROOT / "src/melee/gm/gm_1798.c", ROOT / "src/melee/gm/gm_1A3F.c"]
    pins += [ROOT / "src/melee/gm/gmvs.c", ROOT / "src/melee/gr/grdatfiles.c",
             ROOT / "src/melee/it/iteffect.c"]
    pins += [out / (row[0] + ".fmr") for row in outcomes]
    pins += sorted({ROOT / "examples" / (name + ".fms") for row in selected for name in row[3]})
    receipt = dict(status="RUNNING", inputs={str(p): sha(p) for p in pins}, fighters=[], cases=[],
                   limits=["Offline placement of losing slot below blast zone, normal death thereafter.",
                           "Explicit family manifest, not live Workshop create-button acceptance.",
                           "Trace/actor admission, not rendered pixels/audio or full rollback acceptance."])

    def save():
        (out / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")

    prepared = {}
    save()
    for launch, mode, members, templates in selected:
        directory = out / launch
        directory.mkdir()
        project = directory / "project"
        row = dict(fighter=launch, mode=mode, status="RUNNING", steps=[])
        receipt["fighters"].append(row)
        print("Preparing " + launch, flush=True)
        try:
            for index, (member, template) in enumerate(zip(members, templates)):
                target = project if index == 0 else project / "partner"
                source = directory / ("owned-" + str(index) + ".fms")
                for stage, command in (
                    ("clone", [tool, "clone-stock", iso, member, target, "Results " + launch]),
                    ("parameters", [tool, "import-parameters", target, ROOT / "examples" / (template + ".fms"), source]),
                    ("build", [tool, "script-build", target, source])):
                    result = execute(command, directory / (stage + "-" + str(index) + ".log"))
                    row["steps"].append(result)
                    save()
                    if result["exit"] != 0:
                        raise ValueError(stage + " failed")
                receipt["inputs"][str(source)] = sha(source)
            if mode != "single":
                (project / "family.txt").write_text(
                    "mode=" + mode + "\nprimary_key=main\npartner_key=partner\npartner=partner\n", newline="\n")
            result = execute([tool, "--root", store, "package-project", project, image], directory / "package.log")
            row["steps"].append(result)
            found = re.search(r"Cached source fighter ([0-9a-f]{64})", (directory / "package.log").read_text(errors="replace"))
            if result["exit"] != 0 or not found:
                raise ValueError("Package failed")
            identity = found.group(1)
            for stage in ("verify-package", "accept"):
                result = execute([tool, "--root", store, stage, identity], directory / (stage + ".log"))
                row["steps"].append(result)
                if result["exit"] != 0:
                    raise ValueError(stage + " failed")
            for path in [*project.rglob("*"), store / "cache" / (identity + ".fmc")]:
                if path.is_file():
                    receipt["inputs"][str(path)] = sha(path)
            prepared[launch] = identity
            row.update(status="PASS", package=identity)
        except Exception as error:
            row.update(status="FAIL", error=str(error))
        save()
    for launch, mode, _, _ in selected:
        for name, variant, _ in outcomes:
            row = dict(fighter=launch, outcome=name, status="RUNNING", coverage={}, errors=[])
            receipt["cases"].append(row)
            save()
            if launch not in prepared:
                row.update(status="FAIL", errors=["Required package preparation failed"])
                save()
                continue
            directory = out / launch / name
            directory.mkdir()
            print("Comparing " + launch + " " + name, flush=True)
            result = execute([sys.executable, ROOT / "tools/run_fighter_parity.py",
                "--iso", iso, "--inputs", out / (name + ".fmr"), "--stock-fighter", launch,
                "--package-id", prepared[launch], "--content-root", store, "--independent",
                "--stocks", "1", "--p2", "human:luigi", "--fixture-airborne",
                ("1" if name == "lose" else "2") + ",180,0,-1000", "--out-dir", directory], directory / "compare.log")
            row["comparison"] = result
            if result["exit"] != 0:
                row["errors"].append("Stock/source comparison failed")
            for label in ("stock", "clone"):
                log = directory / (name + "-" + label + ".log")
                if not log.is_file():
                    row["errors"].append(label + " log absent")
                    continue
                facts, errors = coverage(log, label == "clone", mode, variant)
                row["coverage"][label] = facts
                row["errors"] += [label + ": " + error for error in errors]
            if len(row["coverage"]) == 2:
                for field in ("scenes", "results"):
                    if row["coverage"]["stock"][field] != row["coverage"]["clone"][field]:
                        row["errors"].append("Different " + field + " trajectory")
            row["status"] = "FAIL" if row["errors"] else "PASS"
            print(launch + " " + name + " " + row["status"], flush=True)
            save()
    receipt["changed_inputs"] = [path for path, digest in receipt["inputs"].items()
                                 if not Path(path).is_file() or sha(Path(path)) != digest]
    receipt["inputs_unchanged"] = not receipt["changed_inputs"]
    receipt["status"] = "PASS" if receipt["inputs_unchanged"] and all(
        row["status"] == "PASS" for row in receipt["fighters"] + receipt["cases"]) else "FAIL"
    save()
    print(json.dumps(dict(status=receipt["status"], receipt=str(out / "receipt.json"))), flush=True)
    return int(receipt["status"] != "PASS")


if __name__ == "__main__":
    raise SystemExit(main())
