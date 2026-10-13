"""Unregistered tether states4/5: actual stock/generated console comparison."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import re


ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent

SYMBOLS = ['itLinkHookshot_ApplyOwnerScale', 'it_802A2474', 'it_802A24A0', 'link_fighter_compare', 'it_802A2568_inline', 'it_link_get_joint', 'it_link_get_joint_c', 'it_link_lerp', 'it_link_attr_math', 'it_802A2568', 'it_802A2B10', 'it_802A2BA4', 'fn_802A2E4C', 'itLinkhookshot_UnkMotion0_Phys', 'it_802A3C98', 'it_802A40D0', 'it_802A49B0', 'it_802A6944', 'it_802A6A78_get_next', 'it_802A6A78_get_pos', 'it_802A6A78_normalize_diff_rev', 'it_802A6A78_normalize_diff', 'it_802A6A78', 'it_802A6F80', 'it_802A7168', 'itLinkHookshot_Logic20_PickedUp_inline', 'itLinkHookshot_Logic20_PickedUp', 'it_802A7AAC', 'it_802A7AF0', 'it_802A2EE4_inline_alt_mtx_first', 'it_802A7B34_6944_inline', 'it_802A7B34', 'it_802A7D40', 'it_802A2EE4_inline', 'fn_802A33A0_GetFighter', 'fn_802A33A0', 'itLinkhookshot_UnkMotion4_Phys', 'it_802A44CC', 'it_802A5E28', 'it_802A77DC', 'it_802A3500', 'itLinkhookshot_UnkMotion5_Phys', 'it_802A678C', 'it_802A7840']


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
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--snapshot", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--link", type=Path, default=HERE / "link.fixture.fms")
    p.add_argument("--young", type=Path, default=HERE / "young_link.fixture.fms")
    p.add_argument("--iso", type=Path, default=ROOT.parent / "Super Smash Bros. Melee (USA) (v1.02).iso")
    p.add_argument("--prepared", action="store_true", help="reuse only already imported source-identical scratch projects")
    args = p.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    if output == snapshot or snapshot in output.parents:
        p.error("output must not modify the immutable snapshot")
    output.mkdir(parents=True, exist_ok=True)
    sdk, tool, clang = snapshot / "sdk", snapshot / "bin/native_content_tool.exe", snapshot / "compiler/clang.exe"
    manifest_file = snapshot / "freeze.json"
    manifest = json.loads(manifest_file.read_text(encoding="utf-8"))
    frozen = {n: h for n, h in manifest.items() if n.startswith(("sdk/", "sdk\\", "compiler/", "compiler\\"))
              or n in ("bin/native_content_tool.exe", "bin\\native_content_tool.exe")}
    if not frozen:
        raise RuntimeError("empty frozen input inventory")

    def verify():
        for name, expected in frozen.items():
            if digest(snapshot / name) != expected:
                raise RuntimeError(f"frozen input changed: {name}")

    verify()
    commands = []

    def run(stage, cmd):
        cmd = list(map(str, cmd))
        proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=120)
        (output / (stage + ".log")).write_text(proc.stdout + proc.stderr, encoding="utf-8", newline="\n")
        commands.append(dict(stage=stage, command=cmd, exit=proc.returncode))
        if proc.returncode:
            (output / "commands.json").write_text(json.dumps(commands, indent=2), encoding="utf-8")
            raise RuntimeError(f"{stage} failed {proc.returncode}:\n{(proc.stdout + proc.stderr)[-6000:]}")
        return proc.stdout

    flags = [clang, "-std=gnu11", "-O2", "-ffp-contract=off", "-fuse-ld=lld", "-DMELEE_NATIVE=1",
             "-D_CRT_SECURE_NO_WARNINGS", "-D_USE_MATH_DEFINES", "-I" + str(sdk / "src"),
             "-I" + str(sdk / "extern/dolphin/include"), "-I" + str(output)]
    projects = []
    for name, donor, candidate in [
            ("link", "Link", args.link.resolve()),
            ("young_link", "Young Link", args.young.resolve())]:
        project = output / name
        if not args.prepared:
            if project.exists():
                raise RuntimeError("Fresh project required; use a new output or --prepared")
            run(name + "_clone", [tool, "clone-stock", args.iso.resolve(), donor, project, donor + " deployed retraction source fixture"])
            run(name + "_import", [tool, "import-parameters", project, candidate, project / "behavior.fms"])
        source_receipt = project / "fixture_source.json"
        identity = dict(candidate_sha256=digest(candidate), tool_sha256=digest(tool),
                        snapshot_sha256=digest(manifest_file),
                        project_sha256={n:digest(project / n) for n in
                            ["fighter.dat", "fighter_parameters.fmp", "behavior.fms"]})
        if args.prepared:
            if not source_receipt.exists() or json.loads(source_receipt.read_text()) != identity:
                raise RuntimeError("prepared project does not match candidate, snapshot and imported source inputs")
        else:
            source_receipt.write_text(json.dumps(identity, indent=2) + "\n", encoding="utf-8", newline="\n")
        generated = project / "behavior.generated.c"
        run(name + "_generate", [tool, "script-c", project / "behavior.fms", "deployed_tether_" + name, generated])
        if re.search(r"^\s*attribute\s", (project / "behavior.fms").read_text(encoding="utf-8"), re.M):
            raise RuntimeError("DAT attribute draft survived owned import")
        run(name + "_module", flags + ["-shared", generated, "-o", project / "deployed.dll"])
        projects.append(project)

    stock_file = sdk / "src/melee/it/kinds/itlinkhookshot.c"
    stock = stock_file.read_text(encoding="utf-8")
    pieces = sorted((stock.index(body(stock, symbol)), body(stock, symbol)) for symbol in SYMBOLS)
    (output / "link_tether_deployed_originals.h").write_text("\n\n".join(b for _, b in pieces) + "\n", encoding="utf-8", newline="\n")
    imports_file = sdk / "src/melee/it/itarticle_data.c"
    imports = imports_file.read_text(encoding="utf-8")
    (output / "link_tether_model_import.h").write_text("\n\n".join(body(imports, name)
        for name in ["fail", "resolve", "itArticle_ImportModelRoots"]) + "\n", encoding="utf-8", newline="\n")
    sources = [HERE / "stock_test.c",
               sdk / "src/melee/it/native_article_resource.c", sdk / "src/melee/lb/native_model_joint.c",
               sdk / "src/melee/ft/native_callback_lifetime.c", ROOT / "tests/native/model_effect_absent_fixture.c",
               sdk / "src/melee/ft/native_fighter_parameters.c",
               sdk / "src/sysdolphin/baselib/dat.c"]
    sources += [sdk / "src/sysdolphin/baselib" / ("dat_" + n + ".c")
                for n in ["import", "material", "texture", "animation", "model", "geometry", "scene"]]
    run("fixture_compile", flags + sources + ["-o", output / "deployed_stock_test.exe"])
    summaries = []
    for name, project in zip(["link", "young_link"], projects):
        summary = json.loads(run(name + "_matrix", [output / "deployed_stock_test.exe", project / "deployed.dll", project / "fighter.dat", name, project / "fighter_parameters.fmp"]))
        summaries.append(dict(fighter=name, **summary))
    verify()
    inputs = [Path(__file__).resolve(), HERE / "stock_test.c", stock_file,
              imports_file, args.link.resolve(), args.young.resolve(), manifest_file]
    inputs += sources[1:]
    for project in projects:
        inputs += [project / name for name in ["fighter.dat", "fighter_parameters.fmp", "behavior.fms", "behavior.generated.c"]]
    receipt = dict(summaries=summaries, snapshot=str(snapshot), commands=commands,
                   import_boundary="normal frozen content-tool import", fresh_import=not args.prepared,
                   input_sha256={str(path): digest(path) for path in inputs},
                   scope="whole state4/state5 deferred update callbacks, retraction solvers and selected transitions vs actual generated DLLs; production resource/joint/lifetime/model-root import linked",
                   controlled_boundaries=["creator lookup and active Definition lookup", "HSD GObj/ItemLink allocation and model construction/free", "matrix and vector math including lbVector_Normalize", "ECB initialization and stage responses", "root item creation, pickup/scale, destruction and signal dispatch", "hitbox-clear call sequencing only; capsules start inactive", "unused Host boundaries trap rather than simulate untested article/fighter states"],
                   original_symbols=SYMBOLS,
                   limits=["this matrix starts with at least one deployed node; no all-undeployed search or stock UB is executed", "resource ownership clears retired reserved parts instead of preserving stock stale pointers", "constructor/held acceptance remains its separate tracked fixture", "Float spool scale is compiled but not exercised by this slice"],
                   pending=["article states1/2/3/6/7/8 and spool animation", "AirCatch360/361 fighter decisions beyond successful compilation", "ground Catch/CatchPull/CatchWait notifications", "full fighter damage/death composition", "gameplay and activation"])
    (output / "acceptance.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(dict(receipt=str(output / "acceptance.json"), summaries=summaries)))


if __name__ == "__main__":
    main()
