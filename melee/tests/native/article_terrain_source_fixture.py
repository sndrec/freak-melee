"""Compile unchanged terrain bounds/lookup bodies against a frozen SDK."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

from article_resource_geometry_source_fixture import body

REPO = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--source-root", type=Path, default=REPO,
                        help="actual backend source checkout; separate from frozen headers")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--scope-label", default="pending engine acceptance")
    args = parser.parse_args()
    snapshot, root, output = args.snapshot.resolve(), args.source_root.resolve(), args.output.resolve()
    if snapshot == output or snapshot in output.parents:
        parser.error("output must remain outside the immutable snapshot")
    output.mkdir(parents=True, exist_ok=True)
    sdk, compiler = snapshot / "sdk", snapshot / "compiler/clang.exe"
    manifest_path = snapshot / "freeze.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    frozen = {n: h for n, h in manifest.items()
              if n.startswith(("sdk/", "sdk\\", "compiler/", "compiler\\"))}
    if not frozen:
        raise RuntimeError("empty frozen SDK/compiler inventory")
    for name, expected in frozen.items():
        if digest(snapshot / name) != expected:
            raise RuntimeError(f"frozen input changed: {name}")
    selected = {
        root / "src/melee/mp/mplib.c": ["mpLib_80054ED8"],
        root / "src/melee/mp/mpisland.c": ["mpIsland_8005AB54"],
        root / "src/melee/ft/native_fighter_module.c": ["stage_line_island_bounds"],
    }
    header = output / "article_terrain_originals.h"
    text = ["// Actual unchanged production bodies; hashes recorded by runner."]
    for path, names in selected.items():
        source = path.read_text(encoding="utf-8")
        for name in names:
            text.append(f"// {path}: {name}\n" + body(source, name))
    header.write_text("\n\n".join(text) + "\n", encoding="utf-8", newline="\n")
    test = REPO / "tests/native/article_terrain_source_test.c"
    inputs = [*selected, test, Path(__file__).resolve(),
              REPO / "tests/native/article_resource_geometry_source_fixture.py", header, compiler,
              sdk / "src/melee/mp/types.h", root / "src/melee/mp/types.h"]
    hashes = {str(path): digest(path) for path in inputs}
    command = [str(compiler), "-std=gnu11", "-O2", "-fuse-ld=lld", "-DMELEE_NATIVE=1",
               "-D_CRT_SECURE_NO_WARNINGS", "-I" + str(sdk / "src"),
               "-I" + str(sdk / "extern/dolphin/include"), "-I" + str(output),
               str(test), "-o", str(output / "article_terrain_source_test.exe")]
    for stage, cmd in [("compile", command), ("run", [command[-1]])]:
        result = subprocess.run(cmd, cwd=REPO, capture_output=True, text=True, timeout=60)
        (output / (stage + ".log")).write_text(result.stdout + result.stderr,
                                               encoding="utf-8", newline="\n")
        if result.returncode:
            raise RuntimeError(f"{stage} failed {result.returncode}:\n{result.stdout}{result.stderr}")
    receipt = dict(json.loads(result.stdout), input_sha256=hashes, command=command,
                   snapshot_manifest_sha256=digest(manifest_path), label=args.scope_label,
                   mp_types_identical=digest(sdk / "src/melee/mp/types.h") == digest(root / "src/melee/mp/types.h"),
                   scope="actual bounds backend + actual island lookup + actual line validity predicate; controlled valid floor graphs and ground-global accessors",
                   limits=["stage construction/malformed cyclic floor graph outside fixture", "whole article decisions and gameplay not tested"])
    for name, expected in hashes.items():
        if digest(Path(name)) != expected:
            raise RuntimeError(f"source changed while testing: {name}")
    (output / "results.json").write_text(json.dumps(receipt, indent=2) + "\n",
                                         encoding="utf-8", newline="\n")
    print(json.dumps(dict(results=str(output / "results.json"), **json.loads(result.stdout))))


if __name__ == "__main__":
    main()
