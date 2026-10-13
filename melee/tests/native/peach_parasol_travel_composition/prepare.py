"""Normal frozen-tool clone/import and optional matching-SDK module compilation."""
import argparse
import hashlib
import json
import re
import struct
import subprocess
import shutil
from pathlib import Path

HERE = Path(__file__).resolve().parent


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ("root", "baseline", "output"):
        p.add_argument("--" + name, type=Path, required=True)
    p.add_argument("--iso", type=Path, default=HERE.parents[3] /
                   "Super Smash Bros. Melee (USA) (v1.02).iso")
    p.add_argument("--generate", action="store_true", help="Generate/compile against matching generic55 or newer SDK")
    a = p.parse_args()
    root, base, out = a.root.resolve(), a.baseline.resolve(), a.output.resolve()
    if out.exists():
        raise ValueError("Use a fresh output directory")
    out.mkdir(parents=True)
    tool = base / "bin/native_content_tool.exe"
    commands = []

    def run(args, log):
        args = [str(x) for x in args]
        commands.append(args)
        result = subprocess.run(args, cwd=root, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        (out / log).write_text(result.stdout)
        if result.returncode:
            raise RuntimeError(result.stdout)

    project = out / "project"
    run([tool, "clone-stock", a.iso.resolve(), "Peach", project,
         "Peach Parasol travel source draft"], "clone.log")
    run([tool, "import-parameters", project, HERE / "candidate.fms", out / "owned.fms"],
        "import.log")
    shutil.copyfile(project / "fighter_parameters.fmp", out / "fighter_parameters.fmp")
    if a.generate:
        run([tool, "script-c", out / "owned.fms", "peach_parasol_travel", out / "generated.inc"], "generate.log")
        clang = base / "compiler/clang.exe"
        sdk = base / "sdk"
        vcvars = Path("C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat")
        args = [clang, "-std=gnu11", "-O2", "-fms-runtime-lib=dll", "-DMELEE_NATIVE=1",
                "-D_DLL", "-D_MT", "-D_USE_MATH_DEFINES", "-I" + str(sdk / "src"),
                "-I" + str(sdk / "extern/dolphin/include"), "-shared", "-x", "c",
                out / "generated.inc", "-o", out / "module.dll"]
        batch = out / "compile_module.cmd"
        batch.write_text('@echo off\ncall "' + str(vcvars) + '" >nul\n' +
                         subprocess.list2cmdline([str(x) for x in args]) + "\n")
        run(["cmd", "/c", batch], "compile_module.log")
    params = {}
    for line in (project / "fighter_parameters.fmp").read_text().splitlines():
        typ, name, equals, value = line.split()
        assert equals == "="
        bits = struct.unpack("<I", struct.pack("<f", float(value)))[0] if typ == "float" else int(value, 0) & 0xffffffff
        params[name] = {"type": typ, "value": value, "bits": bits}
    raw = (project / "fighter.dat").read_bytes()
    data_size, relocs, public, external = struct.unpack_from(">4I", raw, 4)
    symbols = 32 + data_size + 4 * relocs
    strings = symbols + 8 * (public + external)
    roots = {}
    for i in range(public):
        offset, name = struct.unpack_from(">2I", raw, symbols + 8 * i)
        roots[raw[strings + name:].split(b"\0", 1)[0].decode()] = offset
    word = lambda offset: struct.unpack_from(">I", raw, 32 + offset)[0]
    fighter = word(roots["ftDataPeach"] + 4)
    slots = word(roots["ftDataPeach"] + 72)
    article_slots = {"It_Kind_Peach_Explode": 0, "It_Kind_Peach_Turnip": 1,
                     "It_Kind_Peach_Parasol": 2, "It_Kind_Peach_Toad": 3,
                     "It_Kind_Peach_ToadSpore": 4}
    active = None
    attributes = []
    for line in (HERE / "candidate.fms").read_text().splitlines():
        if line.startswith("article "):
            active = line.split()[1]
        match = re.match(r"(\s*)attribute (float|int) (\w+) (\d+)$", line)
        if not match:
            continue
        indent, typ, name, offset = match.groups()
        offset = int(offset)
        data = fighter if not indent else word(word(slots + 4 * article_slots[active]) + 4)
        qualified = name if not indent else active + "." + name
        assert params[qualified]["type"] == typ
        assert params[qualified]["bits"] == word(data + offset), qualified
        attributes.append({"name": qualified, "offset": offset,
                           "DAT_data_offset": data + offset, **params[qualified]})
    assert len(attributes) == 67
    files = [tool, base / "freeze.json", project / "fighter.dat",
             project / "fighter_parameters.fmp", HERE / "candidate.fms",
             HERE / "travel.fms", HERE / "parasol.fms", HERE / "components.json"]
    result = {
        "status": "PASS normal import and compiled draft; source comparison pending" if a.generate else "PASS tuning import only; generic55 module/source comparison pending",
        "snapshot": base.name, "normal_clone_import": True,
        "parameter_count": len(params), "parameters": params,
        "raw_dat_attribute_count": len(attributes), "raw_dat_provenance": attributes,
        "commands": commands,
        "input_hashes": {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in files},
        "no_module_generated": not a.generate, "no_game": True,
    }
    (out / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print("PASS", len(params), "owned parameters;", len(attributes),
          "actual DAT declarations byte-verified; generic55 acceptance pending")


if __name__ == "__main__":
    main()
