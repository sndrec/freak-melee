"""Unregistered Samus AirCatch and common Catch source matrix; controlled article boundary."""
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
COMMON_SYMBOLS = {
    "ftCo_Catch.c": ["ftCo_Catch_CheckInput", "ftCo_800D8A38", "ftCo_800D8AE0", "ftCo_800D8C54", "ftCo_Catch_Anim", "ftCo_CatchDash_Anim", "fn_800D8E30", "fn_800D8E94"],
    "ftCo_0D8E.c": ["fn_800D952C"],
    "ftCo_0D95.c": ["fn_800D9C64"],
    "ftCo_CatchPull.c": ["fn_800D9CE8", "ftCo_CatchPull_Anim", "fn_800DA190"],
    "ftCo_CatchWait.c": ["ftCo_CatchWait_IASA"],
}


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
    parser.add_argument("--iso", type=Path, default=ROOT.parent / "Super Smash Bros. Melee (USA) (v1.02).iso")
    args = parser.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    if snapshot == output or snapshot in output.parents or HERE == output or HERE in output.parents:
        parser.error("output cannot modify the frozen snapshot or source namespace")
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
    commands, inputs, summaries = [], [Path(__file__).resolve(), HERE / "aircatch_test.c", HERE / "catch_test.inc.c", manifest_path], []

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
    ground_source=sdk/"src/melee/ft/kinds/ftCommon/ftCo_0D95.c"
    ground_text = ground_source.read_text(encoding="utf-8")
    constant = re.search(r"^static const Vec3 lbl_803B7510\s*=\s*\{[^}]*\};", ground_text, re.M).group()
    (output/"ground_originals.h").write_text(constant + "\n\n" + "\n\n".join(body(ground_text,n) for n in ["fn_800D9558","fn_800D9930"])+"\n",encoding="utf-8",newline="\n")
    inputs += stock_files+[ground_source]
    common = []
    for filename, names in COMMON_SYMBOLS.items():
        path = sdk / "src/melee/ft/kinds/ftCommon" / filename
        inputs.append(path)
        common += [body(path.read_text(encoding="utf-8"), name) for name in names]
    article_source = sdk / "src/melee/it/kinds/itsamusgrapple.c"
    inputs.append(article_source)
    common += [body(article_source.read_text(encoding="utf-8"), "it_802BAC3C")]
    declarations = [block[:block.index("{")].strip() + ";" for block in common]
    (output / "catch_originals.h").write_text("\n\n".join(declarations + common) + "\n", encoding="utf-8", newline="\n")
    inputs += [output / name for name in ["aircatch_originals.h", "ground_originals.h", "catch_originals.h"]]
    production = [sdk / "src/melee/ft/native_fighter_parameters.c",
                  sdk / "src/melee/ft/native_callback_lifetime.c", ROOT / "tests/native/model_effect_absent_fixture.c"]
    inputs += production
    # Declaration order is explicit; accepted source is read, never modified.
    source_inputs=[ROOT / "examples/samus_wip.fms", HERE / "fighter.component.fms", HERE / "article_boundary.fixture.fms", HERE / "ground.component.fms"]
    chunks=[]
    for path in source_inputs:
        text=path.read_text(encoding="utf-8");starts=list(re.finditer(r"^(?:article|func|export func|action|move|state|attribute|parameter)\s",text,re.M))
        chunks += [text[m.start():starts[i+1].start() if i+1<len(starts) else len(text)] for i,m in enumerate(starts)]
    def group(chunk):
        first=chunk.splitlines()[0]
        return 0 if first.startswith(("state ","attribute ","parameter ")) else 1 if first.startswith(("func ","export func ")) else 2 if first.startswith("article ") else 3
    candidate=output/"samus.candidate.fms"
    candidate.write_text("# TEST ONLY; unregistered Samus with controlled grapple article boundary.\n"+"\n".join(sorted(chunks,key=group)),encoding="utf-8",newline="\n")
    inputs += source_inputs
    for name, donor, candidate in [("samus", "Samus", candidate)]:
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
        run(name + "_compile", flags + [HERE / "aircatch_test.c"] + production + ["-o", executable])
        summaries.append(dict(fighter=name, **json.loads(run(name + "_matrix", [executable, project / "fighter_parameters.fmp", name]))))
        inputs += [candidate] + [project / n for n in ["behavior.fms", "fighter.dat", "fighter_parameters.fmp", "aircatch.generated.c", "unused_host.h"]]
    verify()
    receipt = dict(snapshot=str(snapshot), snapshot_sha256=digest(manifest_path), summaries=summaries, commands=commands,
                   input_sha256={str(path):digest(path) for path in inputs}, original_symbols=SYMBOLS + ["ftCo_800968C8", "ftCo_AirCatchHit_Coll"],
                   common_symbols=COMMON_SYMBOLS,
                   scope="actual generated composed Samus fighter policies versus actual AirCatch357/358, standing/dash frame, common Catch input/entry/animation/exit, CatchPull anchor/ready/follow and CatchWait release bodies; normal owned import; production lifetime body linked",
                   controlled_boundaries=["motion/freefall/landing transitions", "article constructor, reference lookup, signal effects and removal; no article solver is exercised", "raw part51 and endpoint world-position/matrix setup", "spark particle dispatch and dyadic RNG samples with exact draw order", "fastfall/fall/drift/clamp common operations", "ECB collision responses and grounded-hit auxiliary update", "active Definition lookup (NULL on stock side)", "held-item throw priority, capture callback registration, transition to CatchWait, pummel/directional throw outcomes; native Link policy branches excluded by controlled Samus-only guards"],
                   limits=["no game, full constructor, resource or article solver acceptance", "transition callback removal effects already tested in shared lifetime fixtures; this source matrix uses live retained generations", "null stock tether after successful creation timeline is undefined and excluded", "missing CatchPull item has undefined native endpoint dereference; safe authored ready/fallback scope is classified separately", "Samus native follow retains a no-op fn_800DA190; actual body is executed and leaves no events while authored follow leaves NULL", "grapple constructor/nine article modes, victim CaptureWait transition bodies, throw attachment and full lifecycle remain pending", "existing specials/combat articles compile unchanged in the full module; this matrix does not rerun their previously accepted decisions", "standalone compilation includes generated C so exact authored helpers can be invoked without exporting fixture wrappers; separate DLL compilation verifies ordinary module output"])
    (output / "acceptance.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(dict(receipt=str(output / "acceptance.json"), summaries=summaries)))


if __name__ == "__main__":
    main()
