"""Compare imported generated Pikachu/Pichu policy against unchanged stock C.

Uses a frozen tool/SDK and the live, separately hashed owned rig definitions.
Only standalone source compilation occurs; this driver never launches a game.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

from fox_falco_cpu_stock_compare import declaration, function, select_native

ROOT = Path(__file__).resolve().parents[2]


def prepare(snapshot, project, out, family, allow_candidate):
    sdk = snapshot / "sdk"
    hashes = {}

    def read(path):
        data = path.read_bytes()
        hashes[str(path)] = hashlib.sha256(data).hexdigest()
        return data.decode()

    cpu = select_native(read(sdk / "src/melee/ft/ftcpuattack.c"), False)
    common = select_native(read(sdk / "src/melee/ft/kinds/ftCommon/ftCo_0A01.c"), False)
    command = read(sdk / "src/melee/ft/ftcmdscript.c")
    random = read(sdk / "src/sysdolphin/baselib/random.c")
    inlines = read(sdk / "src/melee/ft/inlines.h")
    assets = read(sdk / "src/melee/ft/ftcharacter_assets.h")
    capacity = re.search(r"FT_CHARACTER_EXT_ATTR_CAPACITY\s*=\s*(0x[0-9A-Fa-f]+|\d+)", assets)[1]
    (out / "policy_capacity.inc").write_text("enum { FT_CHARACTER_EXT_ATTR_CAPACITY = " + capacity + " };\n")
    macros = inlines[inlines.index("#define COPY_ATTRS"):inlines.index("#ifdef M2C")]
    (out / "policy_inline.inc").write_text(macros + function(inlines, "Fighter_SetEffectHitlagCallbacks"), newline="\n")
    original = declaration(cpu, "typedef struct ftCo_CollData") + " ftCo_CollData;\n"
    for name in ("ftCo_CpuRangedAttackUnsafe", "ftCo_CpuAddAttackCandidate"):
        original += function(cpu, name)
    for name in ("ftCo_800B463C", "ftCo_800B46B8"):
        original += function(command, name)
    original += "u32 seed = 1; u32* seed_ptr = &seed;\n" + function(random, "HSD_Randf").replace(
        "f32 HSD_Randf(", "static f32 original_randf(")
    for name in ("ftCo_CpuStartUpSpecial", "ftCo_800A92CC", "ftCo_800A0CB0"):
        original += function(common, name)
    initialize = declaration(common, "void ftCo_800A101C")
    original += "static void stock_initialize(Fighter* arg0) { struct CpuFighter* temp_r30 = &arg0->cpu;\n" + declaration(
        initialize, "switch (arg0->kind)") + "\n}\n"
    ranged = cpu[cpu.index("void ftCo_800B77E8"):]
    start, end = ranged.index("    case Ft_Kind_Pikachu:"), ranged.index("    case Ft_Kind_Koopa:")
    original += "static void stock_ranged(Fighter* fp) { Fighter* target; int can_attack;\nswitch(fp->kind) {\n" + ranged[start:end] + "default: break; } }\n"
    for stem, short in (("Pikachu", "Pk"), ("Pichu", "Pc")):
        text = read(sdk / f"src/melee/ft/kinds/ft{stem}/ft{stem.lower()}.c")
        for suffix in ("OnDeath", "OnItemPickup", "OnItemDrop", "OnItemInvisible", "OnItemVisible",
                       "OnKnockbackEnter", "OnKnockbackExit"):
            original += function(text, f"ft{short}_Init_{suffix}")
        if short == "Pk":
            for suffix in ("UnkMotionStates1", "UnkMotionStates2", "LoadSpecialAttrs"):
                original += function(text, f"ftPk_Init_{suffix}")
    hi = read(sdk / "src/melee/ft/kinds/ftPikachu/ftpikachuspecialhi.c")
    for name in ("ftPk_SpecialHiEnd_Coll", "ftPk_SpecialAirHiEnd_Coll"):
        original += function(hi, name)
    jab = select_native(read(sdk / "src/melee/ft/kinds/ftCommon/ftCo_Attack1.c"), False)
    original += function(jab, "onPkPc21EC") + (declaration(jab, "static MotionFlags getMotionFlags") + "\n").replace(
        "getMotionFlags(", "stock_jab_flags(", 1)
    (out / "policy_original.inc").write_text(original, newline="\n")

    stem, short = ("Pikachu", "Pk") if family == "pikachu" else ("Pichu", "Pc")
    table_name = f"ft{short}_Init_MotionStateTable"
    text = read(sdk / f"src/melee/ft/kinds/ft{stem}/ft{family}.c")
    table = declaration(text, "MotionState " + table_name) + ";\n"
    clone = read(ROOT / f"src/melee/ft/kinds/ft{stem}/ft{family}_clone.c")
    base = "\n".join(declaration(clone, marker) + ";" for marker in (
        f"static const NativeFighterBone ft{stem}Clone_Bones",
        f"static const NativeFighterArticleAsset ft{stem}Clone_ArticleAssets",
        f"static const Fighter_DemoStrings ft{stem}Clone_DemoMotionNames",
        f"const NativeFighterDefinition ft{stem}Clone_Definition"))
    names = sorted(set(re.findall(r"\bft(?:Pk_[A-Za-z0-9_]+|Pc_[A-Za-z0-9_]+|Camera_UpdateCameraBox)\b", table + base)))
    names = [name for name in names if name != table_name and not any(
        token in name for token in ("_MS_", "_MF_", "_SM_"))]
    (out / "policy_base.inc").write_text(
        "\n".join("#define " + name + " stock_slot" for name in names) + "\n" + table + base + "\n" +
        "\n".join("#undef " + name for name in names) +
        f"\n#define TEST_BASE ft{stem}Clone_Definition\n", newline="\n")
    # Final acceptance compiles actual shipped independence. Prepromotion
    # scratch declarations require explicit --candidate; no silent rewriting.
    source = read(project / "behavior.fms")
    if not re.search(r"^\s*behavior independent\s*$", source, re.M):
        if not allow_candidate:
            raise ValueError("Shipped independent source required; use --candidate only for a pending isolated candidate")
        source = "fighter:\n    behavior independent\n" + source
    candidate = out / "independent.fms"
    candidate.write_text(source, newline="\n")
    (out / "fighter_parameters.fmp").write_bytes((project / "fighter_parameters.fmp").read_bytes())
    generated = out / "policy_generated.inc"
    result = subprocess.run([str(snapshot / "bin/native_content_tool.exe"), "script-c", str(candidate),
                             family + "_clone", str(generated)], capture_output=True, text=True, cwd=ROOT)
    (out / "generation.log").write_text(result.stdout + result.stderr)
    if result.returncode:
        raise AssertionError(result.stdout + result.stderr)
    code = read(generated)
    fields = sorted(set(re.findall(r"host->(\w+) == 0", code)))
    (out / "policy_required_host.inc").write_text("\n".join(
        "host." + field + " = (__typeof__(host." + field + "))unused_host;" for field in fields) + "\n")
    for path in (candidate, project / "fighter_parameters.fmp", project / "module.dll",
                 ROOT / "examples" / (family + "_wip.fms"), snapshot / "freeze.json",
                 snapshot / "bin/native_content_tool.exe", snapshot / "compiler/clang.exe",
                 ROOT / "tests/native/pikachu_pichu_policy_stock_test.c", Path(__file__),
                 ROOT / "tests/native/fox_falco_cpu_stock_compare.py"):
        if path.exists():
            hashes[str(path)] = hashlib.sha256(path.read_bytes()).hexdigest()
    (out / "source_manifest.json").write_text(json.dumps(hashes, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--project", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--family", choices=("pikachu", "pichu"), required=True)
    parser.add_argument("--prepare-only", action="store_true")
    parser.add_argument("--candidate", action="store_true", help="Explicit fixture-only independence for unpromoted source")
    args = parser.parse_args()
    snapshot, project, out = (p.resolve() for p in (args.snapshot, args.project, args.out_dir))
    out.mkdir(parents=True, exist_ok=True)
    prepare(snapshot, project, out, args.family, args.candidate)
    if args.prepare_only:
        return 0
    sdk = snapshot / "sdk"
    executable = out / "pikachu_pichu_policy_stock_test.exe"
    command = [str(snapshot / "compiler/clang.exe"), "-fms-runtime-lib=dll", "-std=gnu11", "-O2",
               "-ffp-contract=off", "-DMELEE_NATIVE=1", "-D_USE_MATH_DEFINES",
               "-D_CRT_SECURE_NO_WARNINGS", "-D_DLL", "-D_MT",
               "-I" + str(sdk / "src"), "-I" + str(sdk / "extern/dolphin/include"),
               "-I" + str(out), str(ROOT / "tests/native/pikachu_pichu_policy_stock_test.c"),
               str(sdk / "src/melee/ft/native_fighter_parameters.c"), "-o", str(executable)]
    setup = Path("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat")
    script = out / "compile.cmd"
    script.write_text("@echo off\ncall " + subprocess.list2cmdline([str(setup), "-vcvars_ver=14.44"]) +
                      " >nul\n" + subprocess.list2cmdline(command) + "\n")
    with (out / "build.log").open("wb") as log:
        result = subprocess.run(["cmd", "/c", str(script)], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        raise AssertionError((out / "build.log").read_text())
    result = subprocess.run([str(executable), str(project / "fighter_parameters.fmp"), args.family],
                            cwd=ROOT, text=True, capture_output=True)
    (out / "results.log").write_text(result.stdout + result.stderr)
    print(result.stdout + result.stderr, end="")
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
