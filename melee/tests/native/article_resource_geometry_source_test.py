"""Run a single source/negative matrix against frozen SDK plus owned header overlay."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
from article_resource_geometry_source_fixture import extract

REPO = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--snapshot", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    args = p.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    if snapshot == output or snapshot in output.parents:
        p.error("output must be outside snapshot")
    output.mkdir(parents=True, exist_ok=True)
    sdk, clang = snapshot / "sdk", snapshot / "compiler/clang.exe"
    manifest_path = snapshot / "freeze.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    frozen = {n:h for n,h in manifest.items() if n.startswith(("sdk\\", "compiler\\", "sdk/", "compiler/"))}
    if not frozen:
        raise RuntimeError("missing SDK/compiler manifest")
    for n,h in frozen.items():
        if digest(snapshot / n) != h:
            raise RuntimeError(f"snapshot changed: {n}")
    overlay = output / "include/melee/it"
    overlay.mkdir(parents=True, exist_ok=True)
    header = REPO / "src/melee/it/native_article_resource.h"
    backend = REPO / "src/melee/it/native_article_resource.c"
    shutil.copyfile(header, overlay / header.name)
    # Quote includes in the production unit must resolve frozen neighboring
    # headers, rather than reaching live files beside its original pathname.
    shutil.copyfile(backend, overlay / backend.name)
    originals = extract(sdk, output / "resource_geometry_originals.h")
    sources = [REPO / "tests/native/article_resource_geometry_test.c",
               overlay / backend.name,
               sdk / "src/melee/lb/native_model_joint.c",
               sdk / "src/melee/ft/native_callback_lifetime.c"]
    inputs = sources + [header, backend, overlay / header.name, clang, Path(__file__).resolve(),
                       REPO / "tests/native/article_resource_geometry_source_fixture.py"]
    inputs += [sdk / "src" / path for path in originals]
    hashes = {str(path):digest(path) for path in inputs}
    command = [str(clang), "-std=gnu11", "-O2", "-ffp-contract=off", "-fuse-ld=lld",
               "-DMELEE_NATIVE=1", "-D_CRT_SECURE_NO_WARNINGS",
               "-I"+str(output / "include"), "-I"+str(sdk / "src"),
               "-I"+str(sdk / "src/melee/it"),
               "-I"+str(sdk / "extern/dolphin/include"), "-I"+str(output),
               *map(str,sources), "-o",str(output / "geometry_test.exe")]
    for stage,cmd in [("compile",command),("run",[str(output / "geometry_test.exe")])]:
        result = subprocess.run(cmd,cwd=REPO,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        (output / (stage+".log")).write_text(result.stdout,encoding="utf-8",newline="\n")
        if result.returncode:
            raise RuntimeError(f"{stage} failed ({result.returncode}):\n{result.stdout}")
    summary = json.loads(result.stdout)
    receipt = dict(summary, input_sha256=hashes, command=command,
                   snapshot_manifest_sha256=digest(manifest_path), originals=originals,
                   scope="actual resource/joint/lifetime bodies + unchanged stock helper bodies; deterministic HSD/ECB/line-speed boundary",
                   pending=["schema/Host/parser integration", "actual collision/renderer gameplay acceptance", "whole tether solver/fighter lifecycle"])
    for name,h in hashes.items():
        if digest(Path(name)) != h:
            raise RuntimeError(f"input changed while running: {name}")
    (output / "results.json").write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf-8",newline="\n")
    print(json.dumps({"results":str(output / "results.json"),**summary}))


if __name__ == "__main__":
    main()
