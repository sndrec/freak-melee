"""Compare generated PK Flash with unchanged stock C, using an explicit SDK.

The project must contain normal import-parameters output, behavior.generated.c,
and a module compiled with the same SDK. This command never invokes script-build,
the global build, or the game. Original bodies may come from an older immutable
reference snapshot; their full files and every compilation input are hashed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

from ness_pkfire_stock_compare import root_offset

ROOT = Path(__file__).resolve().parents[2]


def prepare(args):
    out = args.out_dir.resolve()
    out.mkdir(parents=True, exist_ok=True)
    dat = args.stock_dat.read_bytes()
    root = root_offset(dat, "ftDataNess")
    attrs = 32 + struct.unpack_from(">I", dat, root + 4)[0]
    table = 32 + struct.unpack_from(">I", dat, root + 72)[0]
    words = []
    for slot, count in ((2, 11), (8, 5)):
        descriptor = 32 + struct.unpack_from(">I", dat, table + slot * 4)[0]
        tuning = 32 + struct.unpack_from(">I", dat, descriptor + 4)[0]
        words.append(struct.unpack_from(">" + "I" * count, dat, tuning))
    raw = [(offset, struct.unpack_from(">I", dat, attrs + offset)[0])
           for offset in (0, 4, 8, 12, 20, 28)]
    imported = {}
    for match in re.finditer(r"(?m)^(int|float) (\S+) = (\S+)$", (args.project / "fighter_parameters.fmp").read_text()):
        kind, name, value = match.groups()
        imported[name] = (int(value, 0) & 0xFFFFFFFF if kind == "int" else
                          struct.unpack("<I", struct.pack("<f", float(value)))[0])
    fighter_names = ("flash_ground_loop_delay", "flash_air_loop_delay", "flash_gravity_delay",
                     "flash_release_delay", "flash_fall_acceleration", "flash_landing_lag")
    article_names = (("lifetime", "max_charge", "initial_scale", "final_scale", "launch_angle_degrees",
                      "launch_speed", "control_acceleration", "gravity", "max_horizontal_speed",
                      "terminal_speed", "explosion_delay"),
                     ("max_charge", "initial_scale", "final_scale", "base_damage", "charge_damage_multiplier"))
    verified = {name: bits for name, (_, bits) in zip(fighter_names, raw)}
    for prefix, names, values in zip(("It_Kind_Ness_PKFlush", "It_Kind_Ness_PKFlush_Explode"), article_names, words):
        verified.update({prefix + "." + name: bits for name, bits in zip(names, values)})
    for name, expected in verified.items():
        if imported.get(name) != expected:
            raise RuntimeError("Imported tuning differs from original DAT: " + name)
    (out / "import_receipt.json").write_text(json.dumps({"verified_words": len(verified), "raw_bits": verified}, indent=2) + "\n")
    (out / "ness_flash_raw.generated.h").write_text(
        "static const struct { unsigned offset; uint32_t bits; } raw_attrs[] = {\n" +
        "".join("{%d,UINT32_C(0x%08x)},\n" % row for row in raw) + "};\n" +
        "".join("static const uint32_t %s[] = {%s};\n" %
                (name, ",".join("UINT32_C(0x%08x)" % word for word in values))
                for name, values in zip(("flash_raw", "explosion_raw"), words)))
    generated = (args.project / "behavior.generated.c").read_text()
    required = sorted(set(re.findall(r"host->(\w+) == 0", generated)))
    (out / "ness_flash_required_host.generated.h").write_text("".join(
        "host.%s=(__typeof__(host.%s))unexpected;\n" % (name, name) for name in required))
    reference = args.reference_root or args.sdk_root
    return [ROOT / "tests/native/ness_flash_stock_test.c",
            args.sdk_root / "src/melee/ft/native_fighter_parameters.c",
            reference / "src/melee/ft/kinds/ftNess/ftnessspecialn.c",
            reference / "src/melee/it/kinds/itnesspkflash.c",
            reference / "src/melee/it/kinds/itnesspkflashexplode.c"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-root", type=Path, required=True)
    parser.add_argument("--reference-root", type=Path)
    parser.add_argument("--project", type=Path, required=True)
    parser.add_argument("--stock-dat", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--compiler", type=Path,
                        default=Path("C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe"))
    parser.add_argument("--prepare-only", action="store_true")
    args = parser.parse_args()
    for name in ("sdk_root", "reference_root", "project", "stock_dat", "out_dir", "compiler"):
        if getattr(args, name) is not None:
            setattr(args, name, getattr(args, name).resolve())
    sources = prepare(args)
    flags = ["-std=gnu11", "-O2", "-ffp-contract=off", "-ffunction-sections", "-fdata-sections",
             "-DMELEE_NATIVE=1", "-D_CRT_SECURE_NO_WARNINGS", "-D_USE_MATH_DEFINES",
             "-D_DLL", "-D_MT", "-Xclang", "--dependent-lib=msvcrt",
             "-I" + str(args.sdk_root / "src"),
             "-I" + str(args.sdk_root / "extern/dolphin/include"), "-I" + str(args.out_dir)]
    manifest = [*sources, args.stock_dat, args.project / "behavior.fms",
                args.project / "fighter_parameters.fmp", args.project / "behavior.generated.c",
                args.sdk_root / "src/melee/ft/native_fighter_module.h",
                args.sdk_root / "src/melee/ft/native_fighter_runtime.h",
                ROOT / "examples/ness_wip.fms",
                ROOT / "src/melee/ft/kinds/ftNess/ftness_clone.c", Path(__file__)]
    module = args.project / "module.dll"
    if module.exists():
        manifest.append(module)
    (args.out_dir / "source_manifest.json").write_text(json.dumps(
        {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in manifest}, indent=2) + "\n")
    commands, objects = [], []
    for index, source in enumerate(sources):
        obj = args.out_dir / (str(index) + ".obj")
        objects.append(str(obj))
        commands.append([str(args.compiler), *flags, "-c", str(source), "-o", str(obj)])
    exe = args.out_dir / "ness_flash_stock_test.exe"
    commands.append([str(args.compiler), "-fuse-ld=lld", *objects, "-o", str(exe), "-Wl,/OPT:REF"])
    (args.out_dir / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    if args.prepare_only:
        print("Prepared unchanged-source PK Flash comparison inputs; no compilation/game.")
        return 0
    if not module.is_file():
        parser.error("Missing SDK-matched module.dll")
    for index, command in enumerate(commands):
        result = subprocess.run(command, capture_output=True, text=True)
        (args.out_dir / (str(index) + ".compile.log")).write_text(result.stdout + result.stderr)
        if result.returncode:
            print(result.stdout + result.stderr)
            return result.returncode
    result = subprocess.run([str(exe), str(module), str(args.project / "fighter_parameters.fmp")],
                            capture_output=True, text=True)
    (args.out_dir / "comparison.log").write_text(result.stdout + result.stderr)
    print(result.stdout + result.stderr)
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
