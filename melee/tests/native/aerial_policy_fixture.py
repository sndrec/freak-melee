"""Console-only actual aerial/contact policy tests against original SDK49 Peach.

Compiles live production policy/lifetime units using explicitly hashed live
headers, with an immutable original-reference snapshot and its Clang compiler.
This is not a frozen49 runtime build and does not open or install a game.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def body(text, name):
    match = re.search(r"^[^\n;{}]*\b" + name + r"\s*\([^;{}]*\)\s*\{", text, re.M)
    if not match:
        raise RuntimeError("missing original function " + name)
    end = text.index("{", match.start()) + 1
    depth = 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[match.start():end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, default=ROOT / ".local/baseline49")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    snapshot, output = args.reference.resolve(), args.output.resolve()
    if output == snapshot or snapshot in output.parents:
        parser.error("output must be outside the original snapshot")
    output.mkdir(parents=True, exist_ok=True)
    manifest = snapshot / "freeze.json"
    frozen = json.loads(manifest.read_text())
    for name, expected in frozen.items():
        if name.startswith(("sdk/", "sdk\\", "compiler/", "compiler\\")):
            if digest(snapshot / name) != expected:
                raise RuntimeError("reference snapshot changed: " + name)
    original = snapshot / "sdk/src/melee/ft/kinds/ftPeach/ftpeachfloat.c"
    text = original.read_text()
    names = ["ftPe_Float_CheckContinueInput", "checkStartFloatInput",
             "ftPe_8011BA54", "ftPe_8011BAD8"]
    extracted = "\n\n".join(body(text, name) for name in names)
    extracted = extracted.replace("ftPe_8011BA54", "original_manual").replace(
        "ftPe_8011BAD8", "original_automatic")
    golden = ROOT / "tests/native/aerial_policy_originals.h"
    golden_text = golden.read_bytes().decode("utf-8")
    provenance_end = golden_text.index("// END ORIGINAL PROVENANCE\n") + len("// END ORIGINAL PROVENANCE\n")
    if golden_text[provenance_end:] != extracted + "\n":
        raise RuntimeError("tracked original bodies differ from unchanged frozen reference")
    header = output / "aerial_policy_originals.h"
    header.write_text(extracted + "\n", encoding="utf-8", newline="\n")
    units = [ROOT / "tests/native/aerial_policy_test.c",
             ROOT / "src/melee/ft/kinds/ftCommon/ftCo_AerialAction.c",
             ROOT / "src/melee/ft/native_fighter_contact.c",
             ROOT / "src/melee/ft/native_callback_lifetime.c"]
    compiler = snapshot / "compiler/clang.exe"
    inputs = units + [original, compiler, manifest, Path(__file__).resolve(), header, golden]
    # Pin all actual live header inputs, not merely the frozen reference headers.
    inputs += sorted((ROOT / "src").rglob("*.h"))
    inputs += sorted((ROOT / "extern/dolphin/include").rglob("*.h"))
    hashes = {str(path): digest(path) for path in inputs}
    exe = output / "aerial_policy_test.exe"
    command = [str(compiler), "-std=gnu11", "-O2", "-fuse-ld=lld",
               "-ffp-contract=off", "-DMELEE_NATIVE=1", "-D_USE_MATH_DEFINES",
               "-D_CRT_SECURE_NO_WARNINGS", "-I" + str(ROOT / "src"),
               "-I" + str(ROOT / "extern/dolphin/include"), "-I" + str(output),
               *map(str, units), "-o", str(exe)]
    for label, cmd in [("compile", command), ("run", [str(exe)])]:
        result = subprocess.run(cmd, cwd=ROOT, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        (output / (label + ".log")).write_text(result.stdout, encoding="utf-8", newline="\n")
        if result.returncode:
            raise RuntimeError(f"{label} failed ({result.returncode}): {result.stdout}")
    receipt = json.loads(result.stdout)
    for name, expected in hashes.items():
        if digest(Path(name)) != expected:
            raise RuntimeError("input changed during source test: " + name)
    receipt.update(input_sha256=hashes, command=command,
                   original_functions=names, executable_sha256=digest(exe),
                   boundaries=["controlled Definition lookup and Peach float entry",
                               "actual live callback generation/list registry",
                               "actual live aerial/contact policy translation units",
                               "no full motion transition, game, script or install tested"])
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n",
                                         encoding="utf-8", newline="\n")
    print(json.dumps({"receipt": str(output / "receipt.json"),
                      **{k: v for k, v in receipt.items() if k != "input_sha256"}}))


if __name__ == "__main__":
    main()
