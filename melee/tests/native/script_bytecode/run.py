"""Compile and execute direct FMS bytecode/VM fixtures in isolated output."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
UNITS = ["native_script_source.c", "native_script_expression.c",
         "native_script_statement.c", "native_script_program.c",
         "native_script_bytecode.c", "native_script_value.c", "native_script_vm.c"]
HEADERS = ["native_script_source.h", "native_script_expression.h", "native_script_ir.h",
           "native_script_statement.h", "native_script_program.h",
           "native_script_bytecode.h", "native_script_vm.h"]
NO_WINDOW = getattr(subprocess, "CREATE_NO_WINDOW", 0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--compiler", default="clang")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    executable = args.output / "test.exe"
    command = [args.compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-O2",
               "-I", str(ROOT/"src"), str(HERE/"test.c")]
    command += [str(ROOT/"src/melee/ft"/unit) for unit in UNITS]
    command += ["-o", str(executable)]
    build = subprocess.run(command, capture_output=True, text=True, creationflags=NO_WINDOW)
    (args.output/"build.log").write_text(build.stdout+build.stderr)
    build.check_returncode()
    run = subprocess.run([str(executable), str(HERE/"probe.fms")], capture_output=True, text=True, creationflags=NO_WINDOW)
    (args.output/"run.log").write_text(run.stdout+run.stderr)
    run.check_returncode()
    receipt = json.loads(run.stdout)
    receipt.update({"scope": "direct production source/index/typed IR/bytecode/VM units with controlled generic native operations",
                    "compiler": command, "game_acceptance": False,
                    "sources_sha256": {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                        for path in [HERE/"test.c", HERE/"probe.fms"] + [ROOT/"src/melee/ft"/unit for unit in UNITS+HEADERS]},
                    "executable_sha256": hashlib.sha256(executable.read_bytes()).hexdigest()})
    (args.output/"receipt.json").write_text(json.dumps(receipt, indent=2)+"\n")
    print(json.dumps({key: value for key, value in receipt.items() if key not in ("compiler", "sources_sha256")}, indent=2))


if __name__ == "__main__":
    main()
