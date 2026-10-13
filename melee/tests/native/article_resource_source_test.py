"""Build/run the resource backend console matrix against an immutable SDK.

No global build, game launch, authored article activation or snapshot mutation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from article_resource_fixture import extract

REPO = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    snapshot = args.snapshot.resolve()
    output = args.output.resolve()
    if output == snapshot or snapshot in output.parents:
        parser.error("output must be outside the immutable snapshot")
    output.mkdir(parents=True, exist_ok=True)
    sdk = snapshot / "sdk"
    clang = snapshot / "compiler/clang.exe"
    manifest = json.loads((snapshot / "freeze.json").read_text(encoding="utf-8"))
    frozen = {name: value for name, value in manifest.items()
              if name.startswith(("sdk\\", "compiler\\", "sdk/", "compiler/"))}
    if not frozen:
        raise RuntimeError("snapshot has no compiler/SDK manifest entries")
    for name, expected in frozen.items():
        if digest(snapshot / name) != expected:
            raise RuntimeError(f"snapshot input changed: {name}")
    sources = [REPO / "tests/native/article_resource_test.c",
               REPO / "src/melee/it/native_article_resource.c",
               sdk / "src/melee/lb/native_model_joint.c",
               sdk / "src/melee/ft/native_callback_lifetime.c"]
    runtime_source = REPO / "src/melee/ft/native_fighter_runtime.c"
    inputs = sources + [REPO / "src/melee/it/native_article_resource.h", clang,
                       runtime_source, REPO / "tests/native/article_resource_fixture.py"]
    hashes = {str(path): digest(path) for path in inputs}
    extract(runtime_source, output / "article_resource_fixture.h")
    command = [str(clang), "-std=gnu11", "-O2", "-fuse-ld=lld",
               "-DMELEE_NATIVE=1", "-D_CRT_SECURE_NO_WARNINGS",
               "-I" + str(sdk / "src"), "-I" + str(sdk / "extern/dolphin/include"),
               "-I" + str(REPO / "src"), "-I" + str(output),
               *map(str, sources), "-o", str(output / "resource_test.exe")]
    compile_result = subprocess.run(command, cwd=REPO, text=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (output / "compile.log").write_text(compile_result.stdout, encoding="utf-8", newline="\n")
    if compile_result.returncode:
        raise RuntimeError(compile_result.stdout)
    run = subprocess.run([str(output / "resource_test.exe")], cwd=REPO, text=True,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (output / "run.log").write_text(run.stdout, encoding="utf-8", newline="\n")
    if run.returncode:
        raise RuntimeError(f"resource fixture failed ({run.returncode}): {run.stdout}")
    result = dict(json.loads(run.stdout), source_sha256=hashes,
                  snapshot=str(snapshot), snapshot_manifest_sha256=digest(snapshot / "freeze.json"),
                  command=command,
                  scope="production resource/shared joint/lifetime bodies; fake HSD allocation, draw, ECB and creator boundary",
                  pending=["engine heap/renderer gameplay acceptance", "active-pool rollback restore",
                           "immutable resource declaration/parser", "authored tether simulation"])
    for name, expected in hashes.items():
        if digest(Path(name)) != expected:
            raise RuntimeError(f"source changed during fixture: {name}")
    (output / "results.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({"results": str(output / "results.json"),
                      **{key: result[key] for key in ("checks", "allocation_failures", "maximum_live_nodes", "borrower_cases")}}))


if __name__ == "__main__":
    main()
