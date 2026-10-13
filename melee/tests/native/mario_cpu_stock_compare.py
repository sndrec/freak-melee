"""Compile generated Mario callbacks against unchanged stock CPU source.

Uses an isolated project and a frozen SDK/compiler. No engine or game launches.
The stock branch bodies and engine helpers are extracted on every invocation;
only the capture return boundary is converted from void to a consumed predicate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def declaration(text, marker):
    start = text.index(marker)
    opening = text.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


def function(text, name):
    match = re.search(r"^(?:static )?(?:void|bool|f32) " + name +
                      r"\([^;]*?\)\s*\{", text, re.MULTILINE)
    if not match:
        raise AssertionError("Missing source function: " + name)
    return declaration(text, match.group(0)) + "\n"


def prepare(sdk, project, out, common_path):
    inputs = {}

    def read(path, live=False):
        file = (ROOT if live else sdk) / path
        data = file.read_bytes()
        inputs[str(file)] = hashlib.sha256(data).hexdigest()
        return data.decode()

    attack = read("src/melee/ft/ftcpuattack.c")
    command = read("src/melee/ft/ftcmdscript.c")
    random = read("src/sysdolphin/baselib/random.c")
    capture = attack[attack.index("void ftCo_800B683C"):]
    branch = re.search(r"else if \(fp->kind == Ft_Kind_Mario\)\s*\{", capture)
    if branch is None:
        raise AssertionError("Mario capture branch not found")
    body = declaration(capture, branch.group(0))
    body = body[body.index("{") + 1:-1]
    body = body.replace("#endif", "").replace("return;", "return true;")
    original = "static bool stock_capture(Fighter* fp) {\nstruct CpuFighter* temp_r31 = &fp->cpu;\n" + body + "\nreturn false;\n}\n"
    ranged = attack[attack.index("void ftCo_800B77E8"):]
    start = ranged.index("    case Ft_Kind_Mario:")
    end = ranged.index("    case ", start + 10)
    original += "static void stock_ranged(Fighter* fp) {\nFighter* target; int can_attack;\nswitch(fp->kind) {\n" + ranged[start:end] + "default: break; }\n}\n"
    original += declaration(attack, "typedef struct ftCo_CollData") + " ftCo_CollData;\n"
    for name in ("ftCo_CpuRangedAttackUnsafe", "ftCo_CpuAddAttackCandidate"):
        original += function(attack, name)
    original += function(command, "ftCo_800B4880").replace(
        "void ftCo_800B4880(", "static void original_command_script(")
    original += function(command, "ftCo_800B463C")
    original += "u32 seed = 1; u32* seed_ptr = &seed;\n" + function(random, "HSD_Randf").replace(
        "f32 HSD_Randf(", "static f32 original_randf(")
    (out / "mario_cpu_original.inc").write_text(original)

    table = declaration(read("src/melee/ft/kinds/ftMario/ftmario.c"),
                        "MotionState ftMr_Init_MotionStateTable") + ";\n"
    clone = read("src/melee/ft/kinds/ftMario/ftmario_clone.c", live=True)
    base = "\n".join(declaration(clone, marker) + ";" for marker in (
        "static const NativeFighterBone ftMarioClone_Bones",
        "static const NativeFighterArticleAsset ftMarioClone_ArticleAssets",
        "static const Fighter_DemoStrings ftMarioClone_DemoMotionNames",
        "const NativeFighterDefinition ftMarioClone_Definition"))
    callbacks = sorted(set(re.findall(r"\bft(?:Mr_[A-Za-z0-9_]+|Camera_UpdateCameraBox)\b", table + base)) -
                       {"ftMr_Init_MotionStateTable", "ftMr_MS_SelfCount"})
    callbacks = [name for name in callbacks if "_MS_" not in name and "_MF_" not in name and "_SM_" not in name]
    (out / "mario_owned_base.inc").write_text(
        "\n".join("#define " + name + " stock_slot" for name in callbacks) +
        "\n" + table + base + "\n" +
        "\n".join("#undef " + name for name in callbacks) + "\n")

    generated = read(str(project / "behavior.generated.c"), live=True)
    fields = sorted(set(re.findall(r"host->(\w+) == 0", generated)))
    (out / "mario_required_host.inc").write_text("\n".join(
        "host." + field + " = (__typeof__(host." + field + "))unused_host;"
        for field in fields) + "\n")
    (out / "mario_generated.inc").write_text(generated)
    # Follow the actual stock ftLoadCommonData descriptors, not a cached map.
    common = common_path.read_bytes()
    inputs[str(common_path)] = hashlib.sha256(common).hexdigest()
    word = lambda offset: struct.unpack_from(">I", common, offset)[0]
    ptr = lambda offset: 32 + word(offset)
    size, relocations, roots, externs = struct.unpack_from(">4I", common, 4)
    table = 32 + size + relocations * 4
    strings = table + (roots + externs) * 8
    root = None
    for i in range(roots):
        target, name_offset = struct.unpack_from(">2I", common, table + i * 8)
        if common[strings + name_offset:].split(b"\0", 1)[0] == b"ftLoadCommonData":
            root = 32 + target
            break
    if root is None:
        raise AssertionError("Stock common DAT lacks ftLoadCommonData")
    raw = ""
    for name, kind, count in (("Mario", 0, 61), ("DrMario", 21, 86)):
        descriptor = ptr(ptr(root + 16) + kind * 4)
        if word(descriptor + 8) != count:
            raise AssertionError("Unexpected stock runtime part count: " + name)
        start = ptr(descriptor + 4)
        pairs = [(part, joint) for part, joint in enumerate(common[start:start + 54]) if joint != 255]
        if len(pairs) != 51:
            raise AssertionError("Unexpected semantic rig pair count: " + name)
        omission_slot = ptr(root + 20) + kind * 4
        if word(omission_slot) and word(ptr(omission_slot) + 4):
            raise AssertionError("Unexpected stock bone omissions: " + name)
        raw += "static const unsigned raw" + name + "[51][2] = {\n" + ",\n".join(
            "{%d,%d}" % pair for pair in pairs) + "\n};\n"
    raw += declaration(read("src/melee/ft/kinds/ftDrMario/ftdrmario_clone.c", live=True),
                       "static const NativeFighterBone ftDrMarioClone_Bones") + ";\n"
    (out / "mario_raw_rigs.inc").write_text(raw)
    inputs[str(project / "fighter_parameters.fmp")] = hashlib.sha256(
        (project / "fighter_parameters.fmp").read_bytes()).hexdigest()
    (out / "source_manifest.json").write_text(json.dumps(inputs, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--project", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--independent", action="store_true")
    parser.add_argument("--common-data", type=Path, required=True,
                        help="Unmodified stock common DAT containing ftLoadCommonData")
    args = parser.parse_args()
    snapshot, project, out = (p.resolve() for p in (args.snapshot, args.project, args.out_dir))
    out.mkdir(parents=True, exist_ok=True)
    sdk = snapshot / "sdk"
    prepare(sdk, project, out, args.common_data.resolve())
    executable = out / "mario_cpu_stock_test.exe"
    command = [str(snapshot / "compiler/clang.exe"), "-fms-runtime-lib=dll", "-std=gnu11", "-O2",
               "-ffp-contract=off", "-DMELEE_NATIVE=1", "-D_USE_MATH_DEFINES",
               "-D_CRT_SECURE_NO_WARNINGS", "-D_DLL", "-D_MT",
               "-I" + str(sdk / "src"), "-I" + str(sdk / "extern/dolphin/include"),
               "-I" + str(out), str(ROOT / "tests/native/mario_cpu_stock_test.c"),
               str(sdk / "src/melee/ft/native_fighter_parameters.c"), "-o", str(executable)]
    setup = Path("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat")
    script = out / "compile.cmd"
    script.write_text("@echo off\ncall " + subprocess.list2cmdline([str(setup)]) +
                      " >nul\n" + subprocess.list2cmdline(command) + "\n")
    with (out / "build.log").open("wb") as log:
        result = subprocess.run(["cmd", "/c", str(script)], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        raise AssertionError((out / "build.log").read_text())
    result = subprocess.run([str(executable), str(project / "fighter_parameters.fmp"),
                             "1" if args.independent else "0"], cwd=ROOT, text=True, capture_output=True)
    (out / "results.log").write_text(result.stdout + result.stderr)
    print(result.stdout + result.stderr, end="")
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
