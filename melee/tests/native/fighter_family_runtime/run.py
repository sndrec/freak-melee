"""Extract actual family orchestration; compile a controlled-import console matrix."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

REPO = Path(__file__).resolve().parents[3]
FUNCTIONS = ["release_slot", "release_family", "prepare_family",
    "nativeFighterRuntime_ValidateCached", "nativeFighterRuntime_StageCached",
    "bind_family", "nativeFighterRuntime_ActivateStaged",
    "nativeFighterRuntime_CharacterSoundBanks", "nativeFighterRuntime_Load",
    "nativeFighterRuntime_MemberId", "nativeFighterRuntime_CpuHistorySource"]


def body(text, name):
    match = re.search(r"^[^\n;{}]*\b" + re.escape(name) + r"\s*\([^;{}]*\)\s*\{", text, re.M)
    if not match:
        raise ValueError(f"missing actual function definition: {name}")
    start = text.index("{", match.start())
    depth, end = 1, start + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[match.start():end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--compiler", default="clang")
    parser.add_argument("--snapshot", type=Path)
    parser.add_argument("--extract-only", action="store_true")
    parser.add_argument("--source", type=Path,
        default=REPO / "src/melee/ft/native_fighter_runtime.c")
    args = parser.parse_args()
    sdk = args.snapshot.resolve() / "sdk" if args.snapshot else REPO
    compiler = str(args.snapshot.resolve() / "compiler/clang.exe") if args.snapshot else args.compiler
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    text = args.source.read_text(encoding="utf-8")
    start = text.index("enum {\n    NATIVE_FIGHTER_FAMILY_CAPACITY")
    end = text.index("typedef struct NativeStockCloneBase", start)
    (output / "types.h").write_text(text[start:end], encoding="utf-8", newline="\n")
    (output / "bodies.h").write_text("\n\n".join(body(text, name) for name in FUNCTIONS),
        encoding="utf-8", newline="\n")
    hash_source = REPO / "src/melee/gm/native_content_store.c"
    (output / "hash.h").write_text(body(hash_source.read_text(encoding="utf-8"), "nativeContent_Hash"),
        encoding="utf-8", newline="\n")
    inputs = [args.source.resolve(), hash_source,
        sdk / "src/melee/gm/native_content_store.h",
        sdk / "src/melee/gm/native_fighter_package.c",
        sdk / "src/melee/ft/native_fighter_runtime.h",
        sdk / "src/melee/ft/native_fighter_module.h",
        sdk / "src/melee/pl/player.h", Path(__file__).resolve(),
        output / "types.h", output / "bodies.h", output / "hash.h"]
    fixture = REPO / "tests/native/fighter_family_runtime/test.c"
    lifetime = sdk / "src/melee/ft/native_callback_lifetime.c"
    inputs.append(lifetime)
    if fixture.exists():
        inputs.append(fixture)
    receipt = {"functions": FUNCTIONS, "controlled": ["member asset importer",
        "member table binder", "store read/default path/installed membership",
        "resource retirement and module close", "common parts bootstrap",
        "player mapping publication (recorded arguments/order)"],
        "inputs": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
        "commands": [], "results": []}
    if args.snapshot:
        manifest_path = args.snapshot.resolve() / "freeze.json"
        receipt["snapshot"] = str(args.snapshot.resolve())
        receipt["freeze_sha256"] = hashlib.sha256(manifest_path.read_bytes()).hexdigest()
        receipt["compiler_sha256"] = hashlib.sha256(Path(compiler).read_bytes()).hexdigest()
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        selected = [p for p in inputs if sdk in p.parents] + [Path(compiler)]
        receipt["verified_frozen_inputs"] = {}
        for path in selected:
            relative = path.relative_to(args.snapshot.resolve()).as_posix()
            expected = manifest.get(relative.replace("/", "\\"), manifest.get(relative))
            actual = hashlib.sha256(path.read_bytes()).hexdigest()
            if expected != actual:
                raise RuntimeError(f"frozen input differs from manifest: {relative}")
            receipt["verified_frozen_inputs"][relative] = actual
        receipt["source_correspondence"] = {
            "runtime_matches_frozen": args.source.read_bytes() ==
                (sdk / "src/melee/ft/native_fighter_runtime.c").read_bytes(),
            "hash_matches_frozen": hash_source.read_bytes() ==
                (sdk / "src/melee/gm/native_content_store.c").read_bytes()}
        frozen_text = (sdk / "src/melee/ft/native_fighter_runtime.c").read_text(encoding="utf-8")
        comparisons = {}
        for name in FUNCTIONS:
            try:
                comparisons[name] = body(text, name) == body(frozen_text, name)
            except ValueError:
                comparisons[name] = "absent from frozen source"
        receipt["source_correspondence"]["tested_functions"] = comparisons
    if args.extract_only:
        receipt["status"] = "EXTRACTED_ONLY_NOT_TESTED"
        (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
        print(output / "receipt.json")
        return
    if not fixture.exists():
        raise RuntimeError("WIP: controlled-import test.c is not implemented; use --extract-only")
    commands = [[compiler, "-std=c11", "-O2", "-fms-extensions",
        "-DMELEE_NATIVE=1", "-D_CRT_SECURE_NO_WARNINGS", "-D_WIN32_WINNT=0x0A00",
        "-DNTDDI_VERSION=0x0A000000", "-I" + str(sdk / "src"),
        "-I" + str(sdk / "extern/dolphin/include"),
        "-I" + str(sdk / "extern"), "-I" + str(output), "tests/native/fighter_family_runtime/test.c",
        str(sdk / "src/melee/gm/native_fighter_package.c"), str(lifetime), "-lbcrypt", "-o", str(output / "test.exe")],
        [str(output / "test.exe")]]
    for label, command in zip(["build", "run"], commands):
        receipt["commands"].append(command)
        result = subprocess.run(command, cwd=REPO, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        (output / (label + ".log")).write_text(result.stdout, encoding="utf-8")
        receipt["results"].append({"operation": label, "exit_code": result.returncode,
            "output": result.stdout})
        (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
        if result.returncode:
            raise RuntimeError(f"{label} failed: {output / (label + '.log')}")
    receipt["status"] = "PASS"
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(output / "receipt.json")


if __name__ == "__main__":
    main()
