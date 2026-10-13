"""Extract actual live position/registry bodies and compile a console-only fixture."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SOURCES = {"src/melee/ft/native_fighter_target.c": ["nativeFighterTarget_Position"],
           "src/melee/ft/ftlib.c": ["ftLib_8008741C"]}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def body(text, name):
    match = re.search(r"^[^\n;{}]*\b" + re.escape(name) + r"\s*\([^;{}]*\)\s*\{", text, re.M)
    if not match:
        raise ValueError("Missing actual definition: " + name)
    depth, end = 1, match.end()
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[match.start():end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    manifest = snapshot / "freeze.json"
    compiler = snapshot / "compiler/clang.exe"
    records = json.loads(manifest.read_text(encoding="utf-8"))
    hashes = {key.replace("\\", "/"): value for key, value in records.items()}
    if sha(compiler) != hashes.get("compiler/clang.exe"):
        raise ValueError("Compiler differs from frozen input manifest")
    extracted = output / "bodies.h"
    extracted.write_text("\n\n".join(body((ROOT / path).read_text(encoding="utf-8"), name)
        for path, names in reversed(list(SOURCES.items())) for name in names) + "\n", encoding="utf-8", newline="\n")
    command = [str(compiler), "-std=c11", "-O2", "-fms-extensions", "-DMELEE_NATIVE=1",
        "-D_CRT_SECURE_NO_WARNINGS", "-I"+str(ROOT/"src"), "-I"+str(ROOT/"extern/dolphin/include"),
        "-I"+str(output), "-MD", "-MF", str(output/"dependencies.d"), str(HERE/"test.c"), "-o", str(output/"test.exe")]
    inputs = [HERE/"test.c", Path(__file__).resolve(), extracted] + [ROOT/path for path in SOURCES] + [
        ROOT/"src/melee/ft/native_fighter_target.h", ROOT/"src/melee/ft/types.h", ROOT/"src/melee/ft/inlines.h",
        ROOT/"src/sysdolphin/baselib/gobj.h", ROOT/"src/sysdolphin/baselib/gobjplink.h"]
    receipt = {"status": "WIP", "scope": "actual live production position/registry bodies and live headers, immutable compiler only",
        "functions": SOURCES, "inputs": {str(p): sha(p) for p in inputs}, "freeze_sha256": sha(manifest),
        "compiler_sha256": sha(compiler), "commands": [], "results": []}
    for label, cmd in [("build", command), ("run", [str(output/"test.exe")])]:
        result = subprocess.run(cmd, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                creationflags=getattr(subprocess,"CREATE_NO_WINDOW",0))
        (output/(label+".log")).write_text(result.stdout, encoding="utf-8", newline="\n")
        receipt["commands"].append(cmd)
        receipt["results"].append({"operation": label, "exit_code": result.returncode, "output": result.stdout})
        receipt["status"] = "WIP" if result.returncode == 0 else "FAIL"
        (output/"receipt.json").write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf-8")
        if result.returncode:
            raise RuntimeError(str(output/(label+".log")))
    receipt["status"] = "PASS"
    (output/"receipt.json").write_text(json.dumps(receipt,indent=2)+"\n",encoding="utf-8")
    print(output/"receipt.json")


if __name__ == "__main__":
    main()
