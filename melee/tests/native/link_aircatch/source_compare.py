"""Unregistered Link/Young Link AirCatch360/361 actual-source console matrix."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
SYMBOLS = ["ftCo_800C3B10", "ftCo_800C3BE8", "ftCo_800C3CC0", "ftCo_AirCatch_Anim",
           "ftCo_AirCatchHit_Anim", "ftCo_AirCatch_IASA", "ftCo_AirCatchHit_IASA",
           "ftCo_AirCatch_Phys", "ftCo_AirCatchHit_Phys", "ftCo_AirCatch_Coll"]


def body(text, name):
    match = re.search(r"^[A-Za-z_][A-Za-z0-9_ \t*]*[ \t*]" + re.escape(name) + r"\s*\([^;{}]*\)\s*\{", text, re.M)
    if not match:
        raise ValueError(f"missing actual body {name}")
    opening = text.index("{", match.start())
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[match.start():end]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--link", type=Path, default=HERE / "link.fixture.fms")
    parser.add_argument("--young", type=Path, default=HERE / "young_link.fixture.fms")
    parser.add_argument("--iso", type=Path, default=ROOT.parent / "Super Smash Bros. Melee (USA) (v1.02).iso")
    args = parser.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    if snapshot == output or snapshot in output.parents:
        parser.error("output cannot modify the frozen snapshot")
    if output.exists():
        parser.error("use a fresh output directory")
    output.mkdir(parents=True)
    sdk, tool, clang = snapshot / "sdk", snapshot / "bin/native_content_tool.exe", snapshot / "compiler/clang.exe"
    manifest_path = snapshot / "freeze.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    frozen = {n:h for n,h in manifest.items() if n.startswith(("sdk/", "sdk\\", "compiler/", "compiler\\"))
              or n.replace("\\", "/") == "bin/native_content_tool.exe"}
    if not frozen:
        raise RuntimeError("empty frozen inventory")

    def verify():
        for name, expected in frozen.items():
            if digest(snapshot / name) != expected:
                raise RuntimeError(f"frozen input changed: {name}")

    verify()
    commands, inputs, summaries = [], [Path(__file__).resolve(), HERE / "stock_test.c", manifest_path], []

    def run(stage, cmd):
        cmd = list(map(str, cmd))
        result = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=120)
        (output / (stage + ".log")).write_text(result.stdout + result.stderr, encoding="utf-8", newline="\n")
        commands.append(dict(stage=stage, command=cmd, exit=result.returncode))
        (output / "commands.json").write_text(json.dumps(commands, indent=2), encoding="utf-8")
        if result.returncode:
            raise RuntimeError(f"{stage} failed:\n{(result.stdout + result.stderr)[-7000:]}")
        return result.stdout

    stock_files = [sdk / "src/melee/ft/kinds/ftCommon/ftCo_AirCatch.c",
                   sdk / "src/melee/ft/kinds/ftCommon/ftCo_FallSpecial.c",
                   sdk / "src/melee/ft/ft_081B.c"]
    text = stock_files[0].read_text(encoding="utf-8")
    # Entry appears below the predicate; retain an actual forward declaration.
    original = "void ftCo_800C3BE8(Fighter_GObj*);\n" + "\n\n".join(body(text, name) for name in SYMBOLS)
    original += "\n" + body(stock_files[1].read_text(encoding="utf-8"), "ftCo_800968C8")
    original += "\n" + body(stock_files[2].read_text(encoding="utf-8"), "ftCo_AirCatchHit_Coll")
    (output / "aircatch_originals.h").write_text(original + "\n", encoding="utf-8", newline="\n")
    inputs += stock_files
    production = [sdk / "src/melee/ft/native_fighter_parameters.c",
                  sdk / "src/melee/ft/native_callback_lifetime.c", ROOT / "tests/native/model_effect_absent_fixture.c"]
    inputs += production
    for name, donor, candidate in [("link", "Link", args.link.resolve()), ("young_link", "Young Link", args.young.resolve())]:
        project = output / name
        run(name + "_clone", [tool, "clone-stock", args.iso.resolve(), donor, project, donor + " AirCatch source fixture"])
        run(name + "_import", [tool, "import-parameters", project, candidate, project / "behavior.fms"])
        generated = project / "aircatch.generated.c"
        run(name + "_generate", [tool, "script-c", project / "behavior.fms", "aircatch_" + name, generated])
        required = sorted(set(re.findall(r"host->(\w+)\s*==\s*0", generated.read_text(encoding="utf-8"))))
        (project / "unused_host.h").write_text("\n".join(f"host.{field} = (__typeof__(host.{field}))unused_host;" for field in required) + "\n", encoding="utf-8", newline="\n")
        flags = [clang, "-std=gnu11", "-O2", "-ffp-contract=off", "-fuse-ld=lld", "-DMELEE_NATIVE=1",
                 "-D_CRT_SECURE_NO_WARNINGS", "-D_USE_MATH_DEFINES", "-I" + str(sdk / "src"),
                 "-I" + str(sdk / "extern/dolphin/include"), "-I" + str(output), "-I" + str(project)]
        run(name + "_dll", flags + ["-shared", generated, "-o", project / "aircatch.dll"])
        executable = project / "aircatch_test.exe"
        run(name + "_compile", flags + [HERE / "stock_test.c"] + production + ["-o", executable])
        summaries.append(dict(fighter=name, **json.loads(run(name + "_matrix", [executable, project / "fighter_parameters.fmp", name]))))
        inputs += [candidate] + [project / n for n in ["behavior.fms", "fighter.dat", "fighter_parameters.fmp", "aircatch.generated.c", "unused_host.h"]]
    verify()
    receipt = dict(snapshot=str(snapshot), summaries=summaries, commands=commands,
                   input_sha256={str(path):digest(path) for path in inputs}, original_symbols=SYMBOLS + ["ftCo_800968C8", "ftCo_AirCatchHit_Coll"],
                   scope="actual generated full-candidate fighter callbacks and helpers versus actual original AirCatch360/361 bodies; normal owned import; actual lifetime body linked",
                   controlled_boundaries=["motion/freefall/landing transitions", "article constructor, reference lookup, signal effects and removal; no article solver is exercised", "thumb semantic resolution and world-position/matrix setup", "fastfall/fall/drift/clamp common operations", "ECB collision responses and grounded-hit auxiliary update", "active Definition lookup (NULL on stock side)"],
                   limits=["no game, full constructor, resource or article solver acceptance", "transition callback removal effects already tested in shared lifetime fixtures; this source matrix uses live retained generations", "null stock tether after successful creation timeline is undefined and excluded", "full AirCatch reel states and ground Catch composition remain unaccepted", "standalone compilation includes generated C so exact authored helpers can be invoked without exporting fixture wrappers; separate DLL compilation verifies ordinary module output"])
    (output / "acceptance.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(dict(receipt=str(output / "acceptance.json"), summaries=summaries)))


if __name__ == "__main__":
    main()
