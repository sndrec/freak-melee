"""Compare native grapple initialization with independent GameCube word decoding."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
BASELINE = "1ba857e70"


def body(text, name):
    match = re.search(r"^[\w \t*]+\b" + re.escape(name) + r"\s*\([^;{}]*\)\s*\{", text, re.M)
    if not match:
        raise RuntimeError("missing function " + name)
    at = text.index("{", match.start()) + 1
    depth = 1
    while depth:
        depth += (text[at] == "{") - (text[at] == "}")
        at += 1
    return text[match.start():at]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    paths = [ROOT / "src/melee/it/kinds/itsamusgrapple.c",
             ROOT / "src/melee/ft/ftcoll.c", ROOT / "src/melee/lb/lbcollision.c"]
    current, collisions, history = [path.read_text() for path in paths]
    original = subprocess.run(["git", "show", BASELINE + ":src/melee/it/kinds/itsamusgrapple.c"],
        cwd=ROOT, check=True, capture_output=True, text=True, creationflags=subprocess.CREATE_NO_WINDOW).stdout
    initializer = body(original, "it_802B7160")
    declaration = re.search(r"const itSamusGrapple_Hitbox it_803B8660 = \{.*?^\};", original, re.M | re.S).group()
    if declaration not in current:
        raise RuntimeError("stock descriptor changed")
    # Verify all three creation sites now initialize the whole descriptor;
    # extracting their unrelated resource/solver code is outside this fixture.
    if len(re.findall(r"hitbox_data = \(itSamusGrapple_HitboxData\) \{\s*\.create_hitbox = it_803B8660,\s*\};", current)) != 3:
        raise RuntimeError("not all three grapple hitbox call sites initialize trailing flags")
    native = args.output / "native_initializer.inc"
    native.write_text("\n\n".join([
        body(history, "lbColl_80008440"), body(history, "lbColl_CopyHitCapsule"),
        body(collisions, "ftColl_800768A0"), declaration,
        initializer.replace("void it_802B7160(", "static void original_initializer(", 1),
        body(current, "it_802B7160")]) + "\n", encoding="utf-8")
    (args.output / "original_initializer.c").write_text(initializer + "\n", encoding="utf-8")
    inputs = paths + [HERE / "test.c", HERE / "run.py", HERE / "README.md", args.compiler.resolve(), native]
    inputs += list((ROOT / "src").rglob("*.h")) + list((ROOT / "extern/dolphin/include").rglob("*.h"))
    sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    pins = {str(path.resolve()): sha(path) for path in inputs}
    (args.output / "input_sha256.json").write_text(json.dumps(pins, indent=2) + "\n")
    commands = []

    def run(stage, command):
        result = subprocess.run(list(map(str, command)), cwd=ROOT, capture_output=True,
            text=True, timeout=180, creationflags=subprocess.CREATE_NO_WINDOW)
        (args.output / (stage + ".log")).write_text(result.stdout + result.stderr)
        commands.append({"stage": stage, "command": list(map(str, command)), "exit": result.returncode})
        (args.output / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        if result.returncode:
            raise RuntimeError(stage + ": " + (result.stdout + result.stderr)[-7000:])
        return result

    exe = args.output / "test.exe"
    flags = ["-std=gnu11", "-O2", "-ffp-contract=off", "-DMELEE_NATIVE=1",
             "-D_CRT_SECURE_NO_WARNINGS", "-D_USE_MATH_DEFINES",
             "-I" + str(ROOT / "src"), "-I" + str(ROOT / "extern/dolphin/include"),
             "-I" + str(args.output)]
    run("strict", [args.compiler, *flags, "-Wall", "-Wextra", "-Werror", "-Wno-array-parameter",
                   "-Wno-unused-variable", "-fsyntax-only", HERE / "test.c"])
    run("compile", [args.compiler, *flags, HERE / "test.c", "-o", exe])
    summary = json.loads(run("fixture", [exe]).stdout)
    changed = [path for path, digest in pins.items() if sha(Path(path)) != digest]
    if changed:
        raise RuntimeError("inputs changed: " + str(changed))
    receipt = {"status": "PASS", "summary": summary, "input_sha256": pins,
        "original_commit": BASELINE, "original_initializer_sha256": hashlib.sha256(initializer.encode()).hexdigest(),
        "commands": commands, "initialized_call_sites": 3,
        "actual": ["current and original grapple initializer bodies and stock descriptor",
                   "unchanged common group reset/copy and victim-history routines",
                   "independent GameCube command-word decoding; all capsule bytes and cleanup flag compared"],
        "controlled": ["damage and angle common helpers observe calls and assign scalar results",
                       "common bone conversion and valid preconstructed rig/joint storage"],
        "limits": ["not a console/Dolphin runtime or script-VM test",
                   "no live article graph construction, resource binding, damage scaling/staleness or angle side effects",
                   "three caller initializations checked statically; retail uninitialized trailing flag cannot have deterministic parity"]}
    (args.output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(summary))


if __name__ == "__main__":
    main()
