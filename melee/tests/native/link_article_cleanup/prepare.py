"""Assemble and normal-import complete unregistered cleanup article candidates."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def assemble(family):
    components = ["bow", "boomerang", "tether"]
    if family == "young_link":
        components.append("milk")
    components.append("cleanup")
    source = "\n".join((HERE / (n + ".component.fms")).read_text(encoding="utf-8") for n in components)
    if family == "link":
        # No stock Link Milk constructor/asset exists; its reachable slot is empty.
        source = source.replace('    if state.milk_handle != -1:\n        if articles.ref(state.milk_handle, It_Kind_CLink_Milk).exists:\n            articles.signal(state.milk_handle, It_Kind_CLink_Milk, "remove", 0)\n        state.milk_handle = -1\n', "")
    matches = list(re.finditer(r"^(?:article|func|export func|action|move|state|attribute|parameter)\s", source, re.M))
    groups = [[], [], [], []]
    for i, match in enumerate(matches):
        block = source[match.start():matches[i + 1].start() if i + 1 < len(matches) else len(source)]
        first = block.splitlines()[0]
        group = 0 if first.startswith(("state ", "attribute ", "parameter ")) else 1 if first.startswith(("func ", "export func ")) else 2 if first.startswith("article ") else 3
        groups[group].append(block)
    source = "# Unregistered full article cleanup source candidate.\n\n" + "\n".join(b for g in groups for b in g)
    if family == "young_link":
        source = source.replace("It_Kind_Link_", "It_Kind_CLink_")
        source = source.replace("tether_extend_sound = 160073", "tether_extend_sound = 70073")
        source = source.replace("tether_retract_sound = 160076", "tether_retract_sound = 70076")
    return source


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--snapshot", required=True, type=Path)
    p.add_argument("--output", required=True, type=Path)
    p.add_argument("--iso", type=Path, default=ROOT.parent / "Super Smash Bros. Melee (USA) (v1.02).iso")
    args = p.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    if output.exists() or output == snapshot or snapshot in output.parents:
        p.error("use a fresh output outside the immutable snapshot")
    output.mkdir(parents=True)
    manifest = json.loads((snapshot / "freeze.json").read_text(encoding="utf-8"))
    frozen = {n:v for n,v in manifest.items() if n.startswith(("sdk/", "sdk\\", "compiler/", "compiler\\")) or n.replace("\\", "/") == "bin/native_content_tool.exe"}
    def verify():
        if not frozen: raise RuntimeError("empty frozen input inventory")
        for name, expected in frozen.items():
            if sha(snapshot / name) != expected: raise RuntimeError("frozen input changed: " + name)
    verify()
    commands, projects = [], []
    tool = snapshot / "bin/native_content_tool.exe"
    def run(stage, cmd):
        cmd = list(map(str, cmd))
        proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=120)
        (output / (stage + ".log")).write_text(proc.stdout + proc.stderr, encoding="utf-8", newline="\n")
        commands.append(dict(stage=stage, command=cmd, exit=proc.returncode))
        (output / "commands.json").write_text(json.dumps(commands, indent=2) + "\n", encoding="utf-8")
        if proc.returncode: raise RuntimeError(stage + " failed:\n" + (proc.stdout + proc.stderr)[-6000:])
    for family, donor in [("link", "Link"), ("young_link", "Young Link")]:
        raw = output / (family + ".candidate.fms")
        raw.write_text(assemble(family), encoding="utf-8", newline="\n")
        project = output / family
        run(family + "_clone", [tool, "clone-stock", args.iso.resolve(), donor, project, donor + " cleanup composition source fixture"])
        run(family + "_import", [tool, "import-parameters", project, raw, project / "behavior.fms"])
        generated = project / "cleanup.generated.c"
        run(family + "_generate", [tool, "script-c", project / "behavior.fms", "cleanup_" + family, generated])
        sdk = snapshot / "sdk"
        flags = [snapshot / "compiler/clang.exe", "-std=gnu11", "-O2", "-ffp-contract=off", "-fuse-ld=lld", "-DMELEE_NATIVE=1", "-D_CRT_SECURE_NO_WARNINGS", "-D_USE_MATH_DEFINES", "-I" + str(sdk / "src"), "-I" + str(sdk / "extern/dolphin/include")]
        run(family + "_dll", flags + ["-shared", generated, "-o", project / "cleanup.dll"])
        projects.append(str(project))
    verify()
    (output / "preparation.json").write_text(json.dumps(dict(snapshot=str(snapshot), snapshot_sha256=sha(snapshot / "freeze.json"), projects=projects, commands=commands, inputs={str(p):sha(p) for p in HERE.iterdir() if p.is_file()}, acceptance=False, scope="normal import/generation/DLL compilation only; callback comparison remains pending"), indent=2) + "\n", encoding="utf-8")
    print(output / "preparation.json")


if __name__ == "__main__":
    main()
