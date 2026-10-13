"""Bounded resource declaration/type/code emitter matrix; never launches Melee."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

REPO = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    if snapshot == output or snapshot in output.parents:
        parser.error("output must be outside immutable snapshot")
    output.mkdir(parents=True, exist_ok=True)
    sdk = snapshot / "sdk"
    manifest_path = snapshot / "freeze.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    for name, expected in manifest.items():
        if name.startswith(("sdk/", "sdk\\", "compiler/", "compiler\\")):
            if digest(snapshot / name) != expected:
                raise RuntimeError(f"changed frozen input: {name}")
    inputs = [REPO / "tools/native_fighter_article_resources.c",
              REPO / "tools/native_fighter_article_resources.h",
              REPO / "src/melee/it/native_article_resource.h",
              REPO / "tests/native/article_resource_compiler_test.c",
              sdk / "tools/native_fighter_script_types.c",
              sdk / "tools/native_fighter_script_code.c", Path(__file__).resolve()]
    hashes = {str(path): digest(path) for path in inputs}
    overlay = output / "include/melee/it"
    overlay.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(inputs[2], overlay / inputs[2].name)
    # Byte-identical component copy uses the frozen neighboring tool headers.
    for path in inputs[:2]:
        shutil.copyfile(path, output / path.name)
    source = inputs[5].read_text(encoding="utf-8")
    start = source.index("bool nativeScriptCode_ParseArguments(")
    opening = source.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    (output / "article_resource_compiler_arguments.h").write_text(
        source[start:end] + "\n", encoding="utf-8", newline="\n")
    clang = snapshot / "compiler/clang.exe"
    command = [str(clang), "-std=gnu11", "-O2", "-fuse-ld=lld",
               "-DMELEE_NATIVE=1", "-D_CRT_SECURE_NO_WARNINGS",
               "-I" + str(output), "-I" + str(output / "include"),
               "-I" + str(sdk / "tools"), "-I" + str(sdk / "src"),
               "-I" + str(sdk / "extern/dolphin/include"),
               str(inputs[3]), str(output / inputs[0].name), str(inputs[4]),
               "-o", str(output / "compiler_test.exe")]
    for name, cmd in [("compile", command), ("run", [str(output / "compiler_test.exe")])]:
        result = subprocess.run(cmd, cwd=REPO, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        (output / (name + ".log")).write_text(result.stdout, encoding="utf-8", newline="\n")
        if result.returncode:
            raise RuntimeError(f"{name} failed ({result.returncode}):\n{result.stdout}")
    receipt = dict(json.loads(result.stdout), input_sha256=hashes, command=command,
                   snapshot_manifest_sha256=digest(manifest_path),
                   scope="actual declaration/type/resource emitter and argument parser; literal expression and state lookup boundaries",
                   pending=["whole compiler/schema/Host integration", "authored article gameplay"])
    for name, expected in hashes.items():
        if digest(Path(name)) != expected:
            raise RuntimeError(f"changed test input: {name}")
    (output / "results.json").write_text(json.dumps(receipt, indent=2) + "\n",
                                         encoding="utf-8", newline="\n")
    print(json.dumps({"results": str(output / "results.json"), "checks": receipt["checks"]}))


if __name__ == "__main__":
    main()
