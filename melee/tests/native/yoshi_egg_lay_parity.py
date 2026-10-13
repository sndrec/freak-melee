"""Freeze the Egg Lay toolchain and compare complete authored Yoshi behavior."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tarfile

from fighter_script_bundle_roundtrip import bundled_id, portrait, run

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x4E082EBA


def freeze(batch, source, iso, working_sdk=False):
    text = source.read_text()
    for move in ("EggLay", "EggThrow", "GroundPound", "EggRoll"):
        if not re.search(r"^move " + move + r":", text, re.MULTILINE):
            raise AssertionError("Acceptance source must author " + move)
    if not re.search(r"^article It_Kind_Yoshi_EggLay states 3:", text, re.MULTILINE):
        raise AssertionError("Acceptance source must author the Egg Lay article")
    batch.mkdir(parents=True, exist_ok=False)
    binary = batch / "bin"
    binary.mkdir()
    for name in ("melee_native.exe", "native_content_tool.exe", "dxcompiler.dll",
                 "dxil.dll", "libpng16.dll", "nod.dll", "SDL3.dll",
                 "webgpu_dawn.dll", "zlib1.dll"):
        shutil.copy2(ROOT / "build/native" / name, binary / name)
    for name in ("ui", "assets"):
        shutil.copytree(ROOT / "build/native" / name, binary / name)
    scripts = batch / "tools"
    scripts.mkdir()
    for name in ("run_fighter_parity.py", "compare_fighter_parity.py"):
        shutil.copy2(ROOT / "tools" / name, scripts / name)
    sdk = batch / "sdk"
    if working_sdk:
        shutil.copytree(ROOT / "src", sdk / "src")
        shutil.copytree(ROOT / "extern/dolphin/include", sdk / "extern/dolphin/include")
        (batch / "sdk-working.diff").write_bytes(subprocess.check_output(["git", "diff", "--", "src"], cwd=ROOT))
    else:
        archive = subprocess.check_output(["git", "archive", "HEAD", "src", "extern/dolphin/include"], cwd=ROOT)
        with tarfile.open(fileobj=io.BytesIO(archive)) as files:
            files.extractall(sdk, filter="data")
    (batch / "sdk-commit.txt").write_text(subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True))
    compiler = batch / "compiler"
    compiler.mkdir()
    llvm = Path("C:/Users/loser/scoop/apps/llvm/current/bin")
    for name in ("clang.exe", "lld-link.exe"):
        shutil.copy2(llvm / name, compiler / name)
    shutil.copytree(llvm.parent / "lib/clang", batch / "lib/clang")
    tool = binary / "native_content_tool.exe"
    project = batch / "project"
    run(tool, "clone-stock", iso, "Yoshi", project, "Yoshi Egg Lay comparison")
    owned = project / "behavior.fms"
    run(tool, "import-parameters", project, source, owned)
    compile_module(batch)
    publish(batch, iso)
    manifest = {}
    for folder in (binary, sdk, compiler, batch / "lib", scripts):
        for path in sorted(folder.rglob("*")):
            if path.is_file():
                manifest[str(path.relative_to(batch))] = hashlib.sha256(path.read_bytes()).hexdigest()
    (batch / "freeze.json").write_text(json.dumps(manifest, indent=2) + "\n")


def compile_module(batch):
    project = batch / "project"
    run(batch / "bin/native_content_tool.exe", "script-c", project / "behavior.fms",
        "yoshi_clone", project / "behavior.generated.c")
    generated = (project / "behavior.generated.c").read_text()
    for assignment in ("generated.special_n = authored_ground_n;",
                       "generated.special_air_n = authored_air_n;"):
        if assignment not in generated:
            raise AssertionError("Missing authored Egg Lay startup")
    for motion in (347, 348, 349, 350, 352, 353, 354, 355):
        if "authored_states[" + str(motion) + " - base->motion_state_first].anim_cb" not in generated:
            raise AssertionError("Missing authored Egg Lay state " + str(motion))
    command = 'call "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\n'
    command += '"' + str(batch / "compiler/clang.exe") + '" -std=gnu11 -O2 -shared -fuse-ld=lld-link '
    command += '-DMELEE_NATIVE=1 -D_CRT_SECURE_NO_WARNINGS -D_DLL -D_MT '
    command += '-I"' + str(batch / "sdk/src") + '" -I"' + str(batch / "sdk/extern/dolphin/include") + '" '
    command += '-o "' + str(project / "module.dll") + '" "' + str(project / "behavior.generated.c") + '"'
    (batch / "compile.cmd").write_text("@echo off\n" + command + "\n")
    with (batch / "compile.log").open("wb") as output:
        result = subprocess.run(["cmd", "/c", str(batch / "compile.cmd")], stdout=output, stderr=subprocess.STDOUT)
    if result.returncode:
        raise AssertionError((batch / "compile.log").read_text())


def publish(batch, iso):
    image = batch / "portrait.png"
    portrait(image)
    tool, store = batch / "bin/native_content_tool.exe", batch / "store"
    package = bundled_id(run(tool, "--root", store, "bundle-project", batch / "project", image, iso))
    run(tool, "--root", store, "accept", package, iso)
    (batch / "package.txt").write_text(package + "\n")


def verify_freeze(batch):
    manifest = json.loads((batch / "freeze.json").read_text())
    for name, digest in manifest.items():
        if hashlib.sha256((batch / name).read_bytes()).hexdigest() != digest:
            raise AssertionError("Frozen file changed: " + name)


def recording(case):
    data = bytearray((ROOT / "tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr").read_bytes())
    struct.pack_into("<I", data, 16, SEED)
    frames = bytearray()
    for tick in range(case.get("samples", 650)):
        sample = bytearray(data[24:104])
        for offset in (8, 24, 40, 56):
            struct.pack_into("<Hbb", sample, offset, 0, 0, 0)
        if "p3" in case:
            sample[50] = 0  # PADStatus.err: the two-player template disconnects P3.
        for port, start, end, buttons, x, y in case.get("inputs", []):
            if start <= tick < end:
                struct.pack_into("<Hbb", sample, 8 + (port - 1) * 16, buttons, x, y)
        frames.extend(sample)
    return data[:24] + frames


def compare(batch, iso, case):
    name = case["name"]
    folder = batch / name
    folder.mkdir(exist_ok=True)
    inputs = folder / (name + ".fmr")
    inputs.write_bytes(recording(case))
    command = [str(batch / "bin/melee_native.exe"), "--iso", str(iso),
               "--replay-inputs", str(inputs), "--match", "--stage", case.get("stage", "final-destination"),
               "--mode", "stock", "--stocks", "4", "--time", "1", "--items", "off",
               "--hidden", "--mute", "--fast", "--trace-actions", "--trace-fighter-state",
               "--p2", case.get("p2", "human:zelda")]
    if "p3" in case:
        command += ["--p3", case["p3"]]
    for value in case.get("airborne", []):
        command += ["--fixture-airborne", value]
    for value in case.get("items", []):
        command += ["--fixture-item", value]
    for value in case.get("capturable_items", []):
        command += ["--fixture-capturable-item", value]
    logs = []
    for variant in ("stock", "clone"):
        log = folder / (name + "-" + variant + ".log")
        controller = case.get("p1_type", "human")
        suffix = ":" + str(case.get("cpu_level", 9)) if controller == "cpu" else ""
        player = ["--p1", controller + ":yoshi" + suffix] if variant == "stock" else [
            "--p1", controller + ":custom" + suffix, "--p1-fighter", (batch / "package.txt").read_text().strip(),
            "--content-root", str(batch / "store")]
        with log.open("wb") as output:
            result = subprocess.run(command + player, cwd=batch, stdout=output,
                                    stderr=subprocess.STDOUT, timeout=90)
        if result.returncode:
            return dict(case=name, exit=result.returncode, error="Game failed: " + str(log))
        logs.append(log)
    command = [sys.executable, str(batch / "tools/compare_fighter_parity.py"),
               *map(str, logs), "--ports", "1", "2"]
    if "p3" in case:
        command += ["3"]
    for value in case.get("motions", []):
        command += ["--expect-motion", str(value)]
    for port, motion in case.get("port_motions", []):
        command += ["--expect-port-motion", str(port), str(motion)]
    for value in case.get("item_kinds", []):
        command += ["--expect-item-kind", str(value)]
    for kind, previous, current in case.get("item_owner_transfers", []):
        command += ["--expect-item-owner-transfer", str(kind), str(previous), str(current)]
    for value in case.get("damage_ports", []):
        command += ["--expect-damage-port", str(value)]
    for kind, port in case.get("removed_on_damage", []):
        command += ["--expect-item-removed-on-damage", str(kind), str(port)]
    code = subprocess.run(command, cwd=ROOT, check=False).returncode
    for log in logs:
        trace = log.read_text()
        for port in case.get("cpu_recovery_ports", []):
            if re.search(r"\[cpu-recovery\] sample=\d+ P" + str(port) + r"\b", trace) is None:
                print(name + ": no CPU recovery call for P" + str(port), flush=True)
                code = 1
        if case.get("capture_from_air"):
            caught = re.findall(r"\[action\] sample (\d+) P1 .*?motion=213 ", trace)
            airborne = set(map(int, re.findall(
                r"\[fighter\] sample (\d+) P2 .*? ga=1 ", trace)))
            if not any(int(sample) - 1 in airborne for sample in caught):
                print(name + ": no airborne victim immediately before catch", flush=True)
                code = 1
        if case.get("enclosure_damage"):
            damage = re.findall(r"\[damage\] sample (\d+) P2 ([\d.]+) -> ([\d.]+)", trace)
            if not any(float(before) > 0 and float(after) > float(before)
                       for _, before, after in damage):
                print(name + ": no damage after enclosure entry", flush=True)
                code = 1
        if "escape_before" in case:
            escaped = re.search(r"\[action\] sample (\d+) P2 .*?motion=29 .*", trace[trace.find("motion=277 "):])
            if escaped is None or int(escaped[1]) >= case["escape_before"]:
                print(name + ": no early enclosure escape", flush=True)
                code = 1
        if "facing" in case:
            entry = re.search(r"\[fighter\] sample \d+ P1 motion=(?:346|351) .*? facing=([0-9a-f]+)", trace)
            if entry is None or entry[1] != ("bf800000" if case["facing"] < 0 else "3f800000"):
                print(name + ": incorrect entry facing", flush=True)
                code = 1
        for kind, state in case.get("item_states", []):
            if re.search(r"\[item\] sample \d+ I\d+ kind=" + str(kind) + r" state=" + str(state) + r"\b", trace) is None:
                print(name + ": item state not exercised: " + str((kind, state)), flush=True)
                code = 1
        for kind in case.get("expired_items", []):
            samples = re.findall(r"\[item\] sample (\d+) I\d+ kind=" + str(kind) + r"\b", trace)
            if not samples or int(samples[-1]) >= case.get("samples", 650) - 1:
                print(name + ": item did not expire: " + str(kind), flush=True)
                code = 1
        for kind in case.get("absent_items", []):
            if re.search(r"\[item\] sample \d+ I\d+ kind=" + str(kind) + r"\b", trace):
                print(name + ": unexpected item kind: " + str(kind), flush=True)
                code = 1
        for kind, port in case.get("captured_item_removed", []):
            # Capture links intentionally retain NULL owner. This verifies the
            # actual captured item rather than ordinary held-item ownership.
            samples = set(map(int, re.findall(r"\[item\] sample (\d+) I\d+ kind=" + str(kind) + r"\b", trace)))
            damages = set(map(int, re.findall(r"\[damage\] sample (\d+) P" + str(port) + r"\b", trace)))
            if not any(sample - 1 in samples and sample not in samples for sample in damages):
                print(name + ": captured item did not disappear on holder damage", flush=True)
                code = 1
    return dict(case=name, exit=code, samples=case.get("samples", 650),
                assertions=case)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iso", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--sdk-working-tree", action="store_true",
                        help="Use only while the build owner holds matching SDK writes")
    parser.add_argument("--cases", type=Path)
    parser.add_argument("--recompile", action="store_true")
    args = parser.parse_args()
    batch, iso = args.out_dir.resolve(), args.iso.resolve()
    if args.source:
        freeze(batch, args.source.resolve(), iso, args.sdk_working_tree)
    else:
        verify_freeze(batch)
    if args.recompile:
        compile_module(batch)
        publish(batch, iso)
    if args.cases:
        cases = json.loads(args.cases.read_text())
        results = []
        for case in cases:
            print("Running " + case["name"], flush=True)
            results.append(compare(batch, iso, case))
            (batch / (args.cases.stem + "-results.json")).write_text(json.dumps(results, indent=2) + "\n")
        verify_freeze(batch)
        return int(any(row["exit"] for row in results))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
