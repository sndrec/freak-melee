"""Compile actual fighter relation/registry/collision bodies with controlled pose boundary."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

REPO = Path(__file__).resolve().parents[3]
SOURCES = {
    "src/melee/ft/native_fighter_relations.c": ["nativeFighterRelations_Link",
        "nativeFighterRelations_LinkedId", "nativeFighterRelations_CopyCollision"],
    "src/melee/ft/ftlib.c": ["ftLib_8008741C", "ftLib_80087460"],
    "src/melee/mp/mpcoll.c": ["mpCopyCollData"],
    "src/melee/ft/native_fighter_target.c": ["nativeFighterTarget_BonePosition", "nativeFighterTarget_Id"],
    "src/melee/ft/fighter.c": ["Fighter_UnkSetFlag_8006CFBC", "Fighter_8006CFE0", "setBit",
        "Fighter_UnkRecursiveFunc_8006D044", "Fighter_8006D10C_Inline2",
        "Fighter_8006D10C_Inline1", "Fighter_8006D10C"],
    "src/melee/ft/kinds/ftPopo/ftpopospecials.c": ["setRefGObjFlagAndClear", "ftPp_SpecialS_8011F68C"],
    "src/melee/ft/kinds/ftNana/ftnanaspecials.c": ["ftNn_Init_801238E4"],
}


def body(text, name):
    match = re.search(r"^[^\n;{}]*\b" + re.escape(name) + r"\s*\([^;{}]*\)\s*\{", text, re.M)
    if match is None:
        raise RuntimeError(f"missing actual definition: {name}")
    start = text.index("{", match.start())
    depth, end = 1, start + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[match.start():end]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def writer_inventory():
    """Audit the bounded current identifier->field assignment grammar; reject new shapes."""
    entries, hashes = [], {}
    trivia = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
    compact = lambda value: re.sub(r"\s+", "", value)
    for path in sorted((REPO / "src").rglob("*")):
        if path.suffix not in {".c", ".cpp", ".cc", ".cxx", ".h", ".hpp"}:
            continue
        text = path.read_text(encoding="utf-8", errors="strict")
        if "x1A5C" not in text:
            continue
        clean = trivia.sub(lambda m: "".join("\n" if c == "\n" else " " for c in m.group()), text)
        for field in re.finditer(r"\bx1A5C\b", clean):
            suffix = clean[field.end():]
            if re.match(r"\s*(?:\+\+|--|[-+*/%&|^]=)", suffix):
                raise RuntimeError(f"unreviewed compound hitlag-link writer: {path}:{text.count(chr(10), 0, field.start()) + 1}")
            if not re.match(r"\s*=(?!=)", suffix):
                continue
            receiver = re.search(r"\b([A-Za-z_]\w*)\s*->\s*$", clean[:field.start()])
            if receiver is None:
                raise RuntimeError(f"unreviewed hitlag-link receiver shape: {path}:{field.start()}")
            prefix = clean[:receiver.start()].rstrip()
            if prefix.endswith((".", "->", "]", ")")):
                raise RuntimeError(f"unreviewed compound hitlag-link receiver: {path}:{field.start()}")
            name = receiver.group(1)
            end = clean.index(";", field.end()) + 1
            following = re.match(r"\s*" + re.escape(name) + r"\s*->\s*native_hitlag_link_generation\s*=([^;]*);", clean[end:])
            if following is None:
                raise RuntimeError(f"missing immediately following same-receiver generation stamp: {path}:{field.start()}")
            stamp = compact(following.group(1))
            expected = "nativeFighterTarget_Id(" + name + "->x1A5C)"
            policy = "live registry ID"
            if stamp != expected:
                approved = path == REPO / "src/melee/ft/native_fighter_relations.c" and name == "fighter" and stamp == "target"
                function = compact(body(clean, "nativeFighterRelations_Link")) if approved else ""
                if not approved or compact(clean[receiver.start():end]) != "fighter->x1A5C=endpoint;" or \
                    "nativeCallbackSource_Snapshot(source,NativeCallbackSource_Fighter,&generation)" not in function or \
                    "endpoint=target!=0?ftLib_8008741C(target):NULL;" not in function or \
                    "if(target!=0&&(endpoint==NULL||endpoint==source))returnfalse;" not in function:
                    raise RuntimeError(f"unreviewed generation stamp policy: {path}:{field.start()}: {stamp}")
                policy = "explicit full target ID after actual lookup/self rejection"
            relative = path.relative_to(REPO).as_posix()
            hashes[relative] = sha(path)
            entries.append({"path": relative, "line": text.count("\n", 0, field.start()) + 1,
                "receiver": name, "assignment": text[receiver.start():end].strip(), "policy": policy})
    if not entries:
        raise RuntimeError("empty hitlag-link writer inventory")
    return {"writer_count": len(entries), "files": hashes, "sites": entries,
            "scope": "all src C/C++/header direct identifier->x1A5C assignments; comments/strings/comparisons excluded"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True,
                        help="immutable compiler; current live headers/bodies are recorded separately")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    if output == snapshot or snapshot in output.parents:
        parser.error("output must be outside snapshot")
    output.mkdir(parents=True, exist_ok=False)
    compiler = snapshot / "compiler/clang.exe"
    manifest_path = snapshot / "freeze.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if sha(compiler) != manifest.get("compiler\\clang.exe", manifest.get("compiler/clang.exe")):
        raise RuntimeError("compiler differs from immutable manifest")
    extracted = output / "bodies.h"
    extracted.write_text("\n\n".join(body((REPO / path).read_text(encoding="utf-8"), name)
        for path, names in SOURCES.items() for name in names) + "\n", encoding="utf-8", newline="\n")
    fixture = Path(__file__).with_name("test.c")
    lifetime = REPO / "src/melee/ft/native_callback_lifetime.c"
    inputs = [REPO / path for path in SOURCES] + [fixture, Path(__file__).resolve(), lifetime,
        REPO / "src/melee/ft/types.h", REPO / "src/melee/ft/fighter.h", REPO / "src/melee/lb/types.h",
        REPO / "src/melee/ft/native_fighter_relations.h",
        REPO / "src/melee/ft/native_fighter_target.h",
        REPO / "src/melee/ft/native_callback_lifetime.h", extracted]
    # Compile the current include tree; key inputs are hashed, without claiming frozen SDK correspondence.
    command = [str(compiler), "-std=c11", "-O2", "-fms-extensions", "-DMELEE_NATIVE=1",
        "-D_CRT_SECURE_NO_WARNINGS", "-I" + str(REPO / "src"),
        "-I" + str(REPO / "extern/dolphin/include"), "-I" + str(REPO / "extern"),
        "-I" + str(output), "-MD", "-MF", str(output / "dependencies.d"),
        str(fixture), str(lifetime), "-o", str(output / "test.exe")]
    receipt = {"scope": "actual current production bodies/live headers, frozen compiler only",
        "functions": SOURCES, "inputs": {str(p): sha(p) for p in inputs},
        "compiler": str(compiler), "compiler_sha256": sha(compiler),
        "freeze_sha256": sha(manifest_path), "commands": [], "results": []}
    receipt["hitlag_link_writers"] = writer_inventory()
    receipt["body_correspondence_to_snapshot"] = {}
    for path, names in SOURCES.items():
        frozen = snapshot / "sdk" / path
        old = frozen.read_text(encoding="utf-8") if frozen.exists() else ""
        live = (REPO / path).read_text(encoding="utf-8")
        for name in names:
            try:
                receipt["body_correspondence_to_snapshot"][name] = body(live, name) == body(old, name)
            except RuntimeError:
                receipt["body_correspondence_to_snapshot"][name] = "absent from snapshot"
    for label, cmd in [("build", command), ("run", [str(output / "test.exe")])]:
        receipt["commands"].append(cmd)
        result = subprocess.run(cmd, cwd=REPO, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        (output / (label + ".log")).write_text(result.stdout, encoding="utf-8", newline="\n")
        receipt["results"].append({"operation": label, "exit_code": result.returncode,
                                   "output": result.stdout})
        receipt["status"] = "WIP" if result.returncode == 0 else "FAIL"
        (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
        if result.returncode:
            raise RuntimeError(f"{label} failed: {output / (label + '.log')}")
    receipt["status"] = "PASS"
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(output / "receipt.json")


if __name__ == "__main__":
    main()
