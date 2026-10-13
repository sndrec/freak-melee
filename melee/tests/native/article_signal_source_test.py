"""One actual signal-dispatch source matrix; no game or production mutation."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

REPO = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def extract(source, output):
    text = source.read_text(encoding="utf-8")
    functions = ["article_signal_author", "signal_article"]
    bodies = []
    for name in functions:
        match = re.search(r"^[^\n;{}]*\b" + name + r"\s*\([^;{}]*\)\s*\{", text, re.M)
        if not match:
            raise RuntimeError(f"missing original body: {name}")
        opening = text.index("{", match.start())
        depth, end = 1, opening + 1
        while depth:
            depth += (text[end] == "{") - (text[end] == "}")
            end += 1
        bodies.append(text[match.start():end])
    output.write_text("// Unchanged current module bodies.\n" + "\n\n".join(bodies) + "\n",
                      encoding="utf-8", newline="\n")
    return functions


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path,
                        default=REPO / "src/melee/ft/native_fighter_module.c")
    parser.add_argument("--snapshot", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--extract-only", action="store_true")
    args = parser.parse_args()
    source, output = args.source.resolve(), args.output.resolve()
    if args.extract_only:
        extract(source, output)
        return
    if args.snapshot is None:
        parser.error("--snapshot is required for compilation")
    snapshot = args.snapshot.resolve()
    if output == snapshot or snapshot in output.parents:
        parser.error("output must be outside snapshot")
    output.mkdir(parents=True, exist_ok=True)
    sdk, compiler = snapshot / "sdk", snapshot / "compiler/clang.exe"
    manifest_path = snapshot / "freeze.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    frozen = {n:h for n,h in manifest.items()
              if n.startswith(("sdk\\", "compiler\\", "sdk/", "compiler/"))}
    if not frozen:
        raise RuntimeError("snapshot has no SDK/compiler manifest")
    for name, expected in frozen.items():
        if digest(snapshot / name) != expected:
            raise RuntimeError(f"snapshot changed: {name}")
    originals = extract(source, output / "article_signal_originals.h")
    sources = [REPO / "tests/native/article_signal_test.c",
               sdk / "src/melee/ft/native_callback_lifetime.c"]
    inputs = sources + [source, compiler, Path(__file__).resolve(),
                        output / "article_signal_originals.h"]
    hashes = {str(path):digest(path) for path in inputs}
    command = [str(compiler), "-std=gnu11", "-O2", "-fuse-ld=lld", "-DMELEE_NATIVE=1",
               "-D_CRT_SECURE_NO_WARNINGS", "-I"+str(sdk / "src"),
               "-I"+str(sdk / "extern/dolphin/include"), "-I"+str(output),
               *map(str,sources), "-o",str(output / "article_signal_test.exe")]
    for stage, cmd in [("compile",command), ("run",[str(output / "article_signal_test.exe")])]:
        result = subprocess.run(cmd,cwd=REPO,text=True,stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        (output / (stage+".log")).write_text(result.stdout,encoding="utf-8",newline="\n")
        if result.returncode:
            raise RuntimeError(f"{stage} failed ({result.returncode}):\n{result.stdout}")
    receipt = dict(json.loads(result.stdout), input_sha256=hashes, command=command,
        originals=originals, snapshot_manifest_sha256=digest(manifest_path),
        scope="unchanged actual module signal bodies + frozen live-list generation/depth backend; runtime Definition lookup and authored callback boundaries",
        pending=["whole authored move decisions and generated caller lowering", "gameplay"])
    for name, expected in hashes.items():
        if digest(Path(name)) != expected:
            raise RuntimeError(f"input changed while running: {name}")
    (output / "results.json").write_text(json.dumps(receipt,indent=2)+"\n",
                                         encoding="utf-8",newline="\n")
    print(json.dumps({"results":str(output / "results.json"),
                      **json.loads(result.stdout)}))


if __name__ == "__main__":
    main()
