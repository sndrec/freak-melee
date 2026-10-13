"""Unregistered Link/Young Link grounded Catch timing actual-source console matrix."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
SYMBOLS = ["fn_800D8EC8", "fn_800D9228", "ftCo_800D94D8", "fn_800D949C"]


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

    stock_files = [sdk / "src/melee/ft/kinds/ftCommon/ftCo_0D8E.c"]
    text = stock_files[0].read_text(encoding="utf-8")
    # Actual local layout declaration used by the extracted functions.
    layout = text[text.index("typedef struct {"):text.index("} ftCo_LinkCatchAttrs;")+len("} ftCo_LinkCatchAttrs;")]
    original = layout + "\n\n" + "\n\n".join(body(text, name) for name in SYMBOLS)
    cleanup_sources = [
        ("src/melee/ft/kinds/ftLink/ftlinkspecials.c", ["ftLk_SpecialS_RemoveBoomerang1"]),
        ("src/melee/ft/kinds/ftLink/ftlinkspecialn.c", ["ftLk_SpecialN_ProcessFv10", "ftLk_SpecialN_ProcessFv14"]),
        ("src/melee/ft/kinds/ftCLink/inlines.h", ["checkFighter2244"]),
        ("src/melee/ft/kinds/ftCLink/ftclink.c", ["ftCl_Init_80149268", "ftCl_Init_OnDeath"]),
        ("src/melee/ft/kinds/ftLink/ftlink.c", ["ftLk_Init_BoomerangExists", "ftLk_800EAF38", "ftLk_800EAF58", "ftLk_Init_OnDeath"]),
        ("src/melee/it/kinds/itlinkhookshot.c", ["it_802A7AAC"])]
    for path, names in cleanup_sources:
        source = sdk / path
        original += "\n\n" + "\n\n".join(body(source.read_text(encoding="utf-8"), n) for n in names)
        stock_files.append(source)
    (output / "ground_originals.h").write_text(original + "\n", encoding="utf-8", newline="\n")
    inputs += stock_files
    production = [sdk / "src/melee/ft/native_fighter_parameters.c",
                  sdk / "src/melee/ft/native_callback_lifetime.c", ROOT / "tests/native/model_effect_absent_fixture.c"]
    inputs += production
    for name, donor, candidate in [("link", "Link", args.link.resolve()), ("young_link", "Young Link", args.young.resolve())]:
        project = output / name
        run(name + "_clone", [tool, "clone-stock", args.iso.resolve(), donor, project, donor + " AirCatch source fixture"])
        run(name + "_import", [tool, "import-parameters", project, candidate, project / "behavior.fms"])
        generated = project / "ground.generated.c"
        run(name + "_generate", [tool, "script-c", project / "behavior.fms", "ground_" + name, generated])
        required = sorted(set(re.findall(r"host->(\w+)\s*==\s*0", generated.read_text(encoding="utf-8"))))
        (project / "unused_host.h").write_text("\n".join(f"host.{field} = (__typeof__(host.{field}))unused_host;" for field in required) + "\n", encoding="utf-8", newline="\n")
        flags = [clang, "-std=gnu11", "-O2", "-ffp-contract=off", "-fuse-ld=lld", "-DMELEE_NATIVE=1",
                 "-D_CRT_SECURE_NO_WARNINGS", "-D_USE_MATH_DEFINES", "-I" + str(sdk / "src"),
                 "-I" + str(sdk / "extern/dolphin/include"), "-I" + str(output), "-I" + str(project)]
        run(name + "_dll", flags + ["-shared", generated, "-o", project / "ground.dll"])
        executable = project / "ground_test.exe"
        run(name + "_compile", flags + [HERE / "stock_test.c"] + production + ["-o", executable])
        summaries.append(dict(fighter=name, **json.loads(run(name + "_matrix", [executable, project / "fighter_parameters.fmp", name]))))
        inputs += [candidate] + [project / n for n in ["behavior.fms", "fighter.dat", "fighter_parameters.fmp", "ground.generated.c", "unused_host.h"]]
    verify()
    receipt = dict(snapshot=str(snapshot), summaries=summaries, commands=commands,
                   input_sha256={str(path):digest(path) for path in inputs}, original_symbols=SYMBOLS + [n for _, names in cleanup_sources for n in names],
                   scope="actual generated standing/dash timing and composed fighter removal/reset decisions versus extracted stock bodies; normal owned import; actual lifetime body linked",
                   controlled_boundaries=["ft_8008A2BC ground-action finish", "article construction/reference/signal/removal effects; article solvers and article destruction bodies are not exercised", "RightThumb semantic resolution, lb_8000B1CC world point and HSD matrix setup", "mpCheckAllRemap obstruction result", "audio and visibility groups", "active Definition lookup (NULL on stock side); exact live generation checks are production linked"],
                   limits=["no game, full constructor, resource or article solver acceptance", "transition callback removal effects already tested in shared lifetime fixtures; this source matrix uses live retained generations", "stock null tether after successful creation is undefined and excluded", "common Catch input/entry/exit and CatchPull/CatchWait notifications remain pending", "only live original/current-owner bindings are tested; immutable-author transfer/reflection composition is pending", "composed removal ORDER and ordinary handles are tested; full Bow/Arrow/Boomerang/Milk article lifecycle/resource integration remains separate accepted evidence and is not claimed here", "standalone compilation includes generated C so exact authored helpers can be invoked without exporting fixture wrappers; separate DLL compilation verifies ordinary module output"])
    (output / "acceptance.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(dict(receipt=str(output / "acceptance.json"), summaries=summaries)))


if __name__ == "__main__":
    main()
