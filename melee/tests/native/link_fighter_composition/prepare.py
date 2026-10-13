"""Assemble and normal-import unregistered complete special/article/Catch candidates, with common closure gate."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
from assemble import compose

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
ACCEPTED = HERE.parent / "link_article_cleanup"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()




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
        source, components = compose(family)
        raw.write_text(source, encoding="utf-8", newline="\n")
        project = output / family
        run(family + "_clone", [tool, "clone-stock", args.iso.resolve(), donor, project, donor + " unregistered full composition"])
        run(family + "_import", [tool, "import-parameters", project, raw, project / "behavior.fms"])
        generated = project / "fighter.generated.c"
        run(family + "_generate", [tool, "script-c", project / "behavior.fms", "composed_" + family, generated])
        sdk = snapshot / "sdk"
        flags = [snapshot / "compiler/clang.exe", "-std=gnu11", "-O2", "-ffp-contract=off", "-fuse-ld=lld", "-DMELEE_NATIVE=1", "-D_CRT_SECURE_NO_WARNINGS", "-D_USE_MATH_DEFINES", "-I" + str(sdk / "src"), "-I" + str(sdk / "extern/dolphin/include")]
        run(family + "_dll", flags + ["-shared", generated, "-o", project / "fighter.dll"])
        projects.append(dict(family=family,path=str(project),source_bytes=len(source.encode("utf-8")),state_declarations=len(re.findall(r"^state ",source,re.M)),function_declarations=len(re.findall(r"^(?:export )?func ",source,re.M)),article_declarations=len(re.findall(r"^article ",source,re.M)),component_sha256={str(p):sha(p) for p in components}))
    verify()
    (output / "preparation.json").write_text(json.dumps(dict(snapshot=str(snapshot), snapshot_sha256=sha(snapshot / "freeze.json"), projects=projects, commands=commands, inputs={str(p):sha(p) for p in [*(p for p in HERE.iterdir() if p.is_file()), *(p for family in ("link", "young_link") for p in compose(family)[1])]}, acceptance=False, scope="normal import/generation/DLL compilation only; callback comparison remains pending"), indent=2) + "\n", encoding="utf-8")
    print(output / "preparation.json")


if __name__ == "__main__":
    main()
