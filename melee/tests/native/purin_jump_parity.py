"""Focused independent Purin jumps and one accepted costume/Rollout regression.

Coordinate the runtime slot before running. Matches stay hidden, muted and fast,
with drawing enabled. --prepare-only creates the isolated package and recordings
without launching any game. Coverage checks reject missed jumps/turns/pickups.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from fighter_script_bundle_roundtrip import portrait, run
from purin_costume_parity import disc_files, independent_costume

ROOT = Path(__file__).resolve().parents[2]
INPUTS = ROOT / "tests/native/parity_inputs"
JUMP, A = 1 << 10, 1 << 8


def recording(left, worn, samples=900):
    template = (INPUTS / "drmario-special-neutral-ground-inputs.fmr").read_bytes()
    if template[:8] != b"FMREPLAY" or len(template) < 104:
        raise AssertionError("Unexpected recording format")
    first = 260 if worn else 180
    ground = first - 30
    frames = bytearray()
    for tick in range(samples):
        frame = bytearray(template[24:104])
        for port in range(4):
            struct.pack_into("<HbbbbBB", frame, 8 + port * 16, 0, 0, 0, 0, 0, 0, 0)
        buttons, x = 0, 0
        if left and ground - 10 <= tick < ground - 7:
            x = -40  # A short walk-turn avoids long TurnRun near the ledge.
        if worn and tick == 120:
            buttons = A
        if tick == ground:
            buttons = JUMP
        for jump in range(5):
            start = first + 45 * jump
            if start <= tick < start + 9:
                # Each jump reverses the facing established by the previous one.
                x = (-1 if not left else 1) * (-1 if jump & 1 else 1) * 80
            if tick == start:
                buttons = JUMP
        struct.pack_into("<Hbb", frame, 8, buttons, x, 0)
        frames.extend(frame)
    return template[:24] + frames


def coverage(log, worn):
    text = log.read_text(errors="replace")
    states = [(int(m[1]), int(m[2]), m[3]) for m in re.finditer(
        r"^\[fighter\] sample (\d+) P1 motion=(\d+) (.*)$", text, re.M)]
    first_samples = []
    for motion in range(341, 346):
        rows = [row for row in states if row[1] == motion]
        if not rows:
            raise AssertionError(f"{log}: missing aerial jump motion {motion}")
        facings = {re.search(r"\bfacing=([0-9a-f]+)\b", row[2])[1] for row in rows}
        if not {"3f800000", "bf800000"}.issubset(facings):
            raise AssertionError(f"{log}: motion {motion} did not turn during its lifetime")
        first_samples.append(rows[0][0])
    if first_samples != sorted(set(first_samples)):
        raise AssertionError(f"{log}: aerial jump sequence was not ordered")
    if worn and not re.search(r"^\[item\] sample 2[6-9]\d I\d+ kind=31 state=2 .*?owner=P1 ", text, re.M):
        raise AssertionError(f"{log}: Bunny Hood was not actually worn during jumping")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--tool", type=Path, default=ROOT / "build/native/native_content_tool.exe")
    parser.add_argument("--exe", type=Path, default=ROOT / "build/native/melee_native.exe")
    parser.add_argument("--case", action="append", choices=("jumps-right", "jumps-left", "jumps-bunny", "rollout-blue"))
    parser.add_argument("--prepare-only", action="store_true")
    args = parser.parse_args()
    batch, iso = args.out_dir.resolve(), args.iso.resolve()
    batch.mkdir(parents=True, exist_ok=False)
    tool, template = str(args.tool.resolve()), ROOT / "examples/purin_wip.fms"
    if "behavior independent" not in template.read_text():
        raise AssertionError("Purin template is not independent")
    project, store, owned, image = [batch / name for name in ("project", "store", "owned.fms", "portrait.png")]
    portrait(image)
    run(tool, "clone-stock", iso, "Jigglypuff", project, "Independent Purin jump comparison")
    run(tool, "import-parameters", project, template, owned)
    run(tool, "script-build", project, owned)
    output = run(tool, "--root", store, "package-project", project, image)
    package = re.search(r"Cached source fighter ([0-9a-f]{64})", output)[1]
    run(tool, "--root", store, "verify-package", package)
    run(tool, "--root", store, "accept", package)
    with iso.open("rb") as stream:
        offset, size = disc_files(stream)["PlPrBu.dat"]
        stream.seek(offset)
        data = stream.read(size)
    if len(data) != size:
        raise AssertionError("Truncated stock blue costume")
    costume = batch / "blue-costume.dat"
    costume.write_bytes(independent_costume(data, "Bu"))
    output = run(tool, "--root", store, "import-custom-costume", costume, package, "Purin blue jump regression", "2")
    skin = re.search(r"ID: ([a-f0-9]{64})", output)[1]
    cases = []
    for name, left, worn in (("jumps-right", False, False), ("jumps-left", True, False), ("jumps-bunny", False, True)):
        inputs = batch / (name + ".fmr")
        inputs.write_bytes(recording(left, worn))
        cases.append((name, inputs, worn, False))
    cases.append(("rollout-blue", INPUTS / "purin-rollout-short-ground-right-inputs.fmr", False, True))
    selected = [row for row in cases if not args.case or row[0] in args.case]
    manifest = dict(package=package, skin=skin, project=str(project), store=str(store),
        template_sha256=hashlib.sha256(template.read_bytes()).hexdigest(),
        files={str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in
               (Path(__file__), ROOT / "tools/run_fighter_parity.py", ROOT / "tools/compare_fighter_parity.py",
                ROOT / "tests/native/fighter_script_bundle_roundtrip.py", ROOT / "tests/native/purin_costume_parity.py",
                args.exe.resolve(), args.tool.resolve(), owned, costume,
                *[path for path in project.rglob("*") if path.is_file()], *[row[1] for row in selected])})
    (batch / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (batch / "package.txt").write_text(package + "\n")
    if args.prepare_only:
        print("Prepared isolated independent package/recordings: " + str(batch))
        return 0
    results = []
    for name, inputs, worn, blue in selected:
        directory = batch / name
        command = [sys.executable, str(ROOT / "tools/run_fighter_parity.py"),
            "--iso", str(iso), "--exe", str(args.exe.resolve()), "--inputs", str(inputs),
            "--stock-fighter", "jigglypuff", "--package-id", package, "--content-root", str(store),
            "--out-dir", str(directory), "--stocks", "2", "--independent"]
        for motion in ((346, 350, 362) if blue else range(341, 346)):
            command += ["--expect-motion", str(motion)]
        if worn:
            command += ["--fixture-item", "31,100,-60,3", "--expect-item-kind", "31"]
        if blue:
            command += ["--stock-color", "2", "--clone-skin", skin, "--expect-attachment", "--expect-damage-port", "2"]
        log = batch / (name + "-result.log")
        with log.open("wb") as stream:
            result = subprocess.run(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT,
                                    creationflags=subprocess.CREATE_NO_WINDOW)
        code = result.returncode
        if code == 0:
            try:
                clone_log = directory / (inputs.stem + "-clone.log")
                actors = re.findall(r"Live fighter (\d+) kind (\d+): (\d+) completed / (\d+) requested interpreted ticks, state=([0-9a-f]+), faulted=(\d+)", clone_log.read_text(errors="replace"))
                # completed counts only an optional global `on tick` hook.
                # Purin uses motion/event callbacks and has no global tick body.
                if len(actors) != 1 or actors[0][5] != "0" or int(actors[0][3]) <= 0:
                    raise AssertionError(f"{clone_log}: expected one healthy ordinary interpreted fighter")
                if not blue:
                    for label in ("stock", "clone"):
                        coverage(directory / (inputs.stem + "-" + label + ".log"), worn)
            except AssertionError as error:
                with log.open("a") as stream:
                    stream.write(str(error) + "\n")
                code = 1
        results.append(dict(case=name, exit_code=code, samples=(len(inputs.read_bytes()) - 24) // 80,
                            input=str(inputs), log=str(log)))
        (batch / "results.json").write_text(json.dumps(results, indent=2) + "\n")
        print(f"Purin {name}: exit {code}", flush=True)
    if any(hashlib.sha256(Path(path).read_bytes()).hexdigest() != digest for path, digest in manifest["files"].items()):
        raise AssertionError("Input changed during the batch")
    return int(any(row["exit_code"] for row in results))


if __name__ == "__main__":
    raise SystemExit(main())
