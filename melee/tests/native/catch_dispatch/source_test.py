"""Actual Catch/CatchPull/CatchWait dispatch, frozen43 fallback versus authored hooks."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
UNITS = {
    "catch": ("ftCo_Catch.c", ["ftCo_Catch_CheckInput", "ftCo_800D8A38",
        "ftCo_800D8AE0", "ftCo_800D8B9C", "fn_800D8BFC", "ftCo_800D8C54",
        "ftCo_Catch_Anim", "ftCo_CatchDash_Anim", "fn_800D8E30", "fn_800D8E94"]),
    "samus": ("ftCo_0D8E.c", ["fn_800D952C"]),
    "pull": ("ftCo_CatchPull.c", ["fn_800D9CE8", "ftCo_CatchPull_Anim",
        "fn_800DA054", "fn_800DA190"]),
    "wait": ("ftCo_CatchWait.c", ["fn_800DA1D8", "fn_800DA2B0", "ftCo_CatchWait_IASA",
        "fn_800DA490", "fn_800DA4A0", "fn_800DA4C0"]),
    "capture": ("ftCo_CaptureWait.c", ["ftCo_CaptureWait_Setup", "fn_800DB6C8",
        "fn_800DB790", "fn_800DBAE4"]),
}
COMMON = Path("src/melee/ft/kinds/ftCommon")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def body(text, name):
    # Every selected function has a bool/void return type. Require that actual
    # declaration prefix, so an indented `if (checkItemThrow(...)) {` call site
    # cannot be mistaken for the definition.
    match = re.search(r"^[ \t]*(?:static[ \t]+)?(?:bool|void)[ \t]+" +
                      re.escape(name) + r"\s*\([^;{}]*\)\s*\{", text, re.M)
    if not match:
        raise RuntimeError("missing actual function " + name)
    opening = text.index("{", match.start())
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[match.start():end]


def extract(source, prefix, output):
    hashes, blocks = {}, []
    symbols = [name for _, names in UNITS.values() for name in names]
    names = {name: prefix + "_" + name for name in symbols}
    for _, (filename, selected) in UNITS.items():
        path = source / COMMON / filename
        hashes[str(path)] = sha(path)
        text = path.read_text(encoding="utf-8")
        blocks.extend(body(text, name) for name in selected)
    for old, new in names.items():
        blocks = [re.sub(r"\b" + re.escape(old) + r"\b", new, block) for block in blocks]
    parts = [block[:block.index("{")].strip() + ";" for block in blocks]
    output.write_text("// Actual full bodies; only function identifiers renamed.\n" +
                      "\n\n".join(parts + blocks) + "\n", encoding="utf-8", newline="\n")
    return hashes


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--snapshot", required=True, type=Path, help="matching44-or-later frozen SDK/compiler")
    p.add_argument("--baseline", type=Path, default=ROOT / ".local/baseline43")
    p.add_argument("--source", type=Path, default=ROOT, help="checkout containing actual modified dispatch bodies")
    p.add_argument("--output", required=True, type=Path)
    p.add_argument("--prepare-only", action="store_true", help="extract/hash inputs without compiling or executing")
    args = p.parse_args()
    snapshot, baseline, source, output = map(Path.resolve,
        (args.snapshot, args.baseline, args.source, args.output))
    if output.exists() or any(output == s or s in output.parents for s in (snapshot, baseline)):
        p.error("use a fresh output outside both immutable snapshots")
    output.mkdir(parents=True)
    inventories = []
    for snap in (snapshot, baseline):
        manifest = json.loads((snap / "freeze.json").read_text(encoding="utf-8"))
        frozen = {n: v for n, v in manifest.items() if n.replace("\\", "/").startswith(("sdk/", "compiler/"))}
        if not frozen:
            raise RuntimeError("empty frozen input inventory")
        inventories.append((snap, frozen))
    def verify():
        for snap, frozen in inventories:
            for name, expected in frozen.items():
                if sha(snap / name) != expected:
                    raise RuntimeError("frozen input changed: " + str(snap / name))
    verify()
    hashes = extract(source, "new", output / "new_bodies.h")
    hashes.update(extract(baseline / "sdk", "old", output / "old_bodies.h"))
    for path in [Path(__file__).resolve(), HERE / "dispatch_test.c", HERE / "README.md",
                 snapshot / "sdk/src/melee/ft/native_callback_lifetime.c",
                 snapshot / "sdk/src/melee/ft/inlines.h",
                 snapshot / "sdk/src/melee/ft/native_fighter_runtime.h"]:
        hashes[str(path)] = sha(path)
    receipt = dict(snapshot=str(snapshot), baseline=str(baseline),
        snapshot_manifest_sha256=sha(snapshot / "freeze.json"),
        baseline_manifest_sha256=sha(baseline / "freeze.json"), inputs=hashes,
        scope="actual common Catch/CatchPull/CatchWait/CaptureWait bodies; controlled motion, held-item throw priority, pickup callback, pummel/directional throw, article schedules, effects, world transforms and active Definition lookup",
        acceptance=False, games=False, global_build=False, commands=[])
    if not args.prepare_only:
        sdk = snapshot / "sdk"
        exe = output / "catch_dispatch.exe"
        command = [snapshot / "compiler/clang.exe", "-std=gnu11", "-O2", "-fuse-ld=lld",
            "-DMELEE_NATIVE=1", "-D_CRT_SECURE_NO_WARNINGS", "-I" + str(output),
            "-I" + str(sdk / "src"), "-I" + str(sdk / "extern/dolphin/include"),
            HERE / "dispatch_test.c", sdk / "src/melee/ft/native_callback_lifetime.c",
            "-o", exe]
        for label, cmd in [("compile", command), ("matrix", [exe])]:
            cmd = list(map(str, cmd))
            run = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=120)
            (output / (label + ".log")).write_text(run.stdout + run.stderr, encoding="utf-8", newline="\n")
            receipt["commands"].append(dict(stage=label, command=cmd, exit=run.returncode))
            if run.returncode:
                (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
                raise RuntimeError(label + " failed:\n" + (run.stdout + run.stderr)[-6000:])
            if label == "matrix":
                receipt["results"] = json.loads(run.stdout)
        cmd = [str(exe), "--legacy-frame-retirement"]
        run = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=30)
        (output / "legacy-retirement.log").write_text(run.stdout + run.stderr, encoding="utf-8", newline="\n")
        receipt["commands"].append(dict(stage="old43-retirement-rejection", command=cmd, exit=run.returncode))
        (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8", newline="\n")
        if run.returncode & 0xFFFFFFFF != 0xC0000005:
            raise RuntimeError("old43 negative control did not reject with access violation: " + str(run.returncode))
        receipt["acceptance"] = True
    verify()
    for name, expected in hashes.items():
        if sha(Path(name)) != expected:
            raise RuntimeError("source input changed during run: " + name)
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(output / "receipt.json")


if __name__ == "__main__":
    main()
