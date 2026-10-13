"""Batched compiler/generated-module checks with actual item pause/resume bodies."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

REPO = Path(__file__).resolve().parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def originals(source, output):
    text = source.read_text(encoding="utf-8")
    functions = []
    for name in ("it_8026B724", "it_8026B73C"):
        match = re.search(r"^void " + name + r"\([^;{}]*\)\s*\{", text, re.M)
        if match is None:
            raise RuntimeError(f"missing actual body: {name}")
        start = text.index("{", match.start())
        depth, end = 1, start + 1
        while depth:
            depth += (text[end] == "{") - (text[end] == "}")
            end += 1
        functions.append(text[match.start():end])
    output.write_text("// Unchanged actual item engine bodies.\n" +
                      "\n\n".join(functions) + "\n", encoding="utf-8", newline="\n")


def run(command, output, name):
    result = subprocess.run(command, cwd=REPO, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    (output / (name + ".log")).write_text(result.stdout, encoding="utf-8", newline="\n")
    if result.returncode:
        raise RuntimeError(f"{name} failed ({result.returncode}):\n{result.stdout}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--sdk", type=Path, required=True,
                        help="Matching SDK root containing src and extern; current checkout is allowed.")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--generator", type=Path,
                        help="Already-built script_item_pause_compiler_test executable, if available.")
    args = parser.parse_args()
    compiler, sdk, output = args.compiler.resolve(), args.sdk.resolve(), args.output.resolve()
    if output == sdk or sdk in output.parents and sdk != REPO:
        parser.error("output must not mutate a frozen SDK")
    output.mkdir(parents=True, exist_ok=True)
    cmake = REPO / "CMakeLists.txt"
    match = re.search(r"set\(NATIVE_FIGHTER_SCRIPT_SOURCES\s+([^)]*)\)",
                      cmake.read_text(encoding="utf-8"))
    if not match:
        raise RuntimeError("missing compiler source inventory")
    source_names = re.findall(r"tools/[a-zA-Z0-9_/.]+\.c", match[1])
    compiler_sources = [REPO / name for name in source_names]
    compiler_sources.append(REPO / "src/melee/ft/native_fighter_parameters.c")
    compiler_fixture = REPO / "tests/native/script_item_pause_compiler_test.c"
    module_fixture = REPO / "tests/native/script_item_pause_module_test.c"
    original_source = REPO / "src/melee/it/it_26B1.c"
    # Record the SDK headers actually supplied; compile source and SDK may differ.
    inputs = [compiler, cmake, compiler_fixture, module_fixture, original_source,
              Path(__file__).resolve(), *compiler_sources,
              *sorted((sdk / "src").rglob("*.h")),
              *sorted((sdk / "extern/dolphin/include").rglob("*.h")),
              *sorted((REPO / "tools").glob("native_fighter*.h"))]
    if args.generator:
        inputs.append(args.generator.resolve())
    hashes = {str(path): digest(path) for path in inputs}
    commands = []
    common = [str(compiler), "-std=gnu11", "-O2", "-fuse-ld=lld",
              "-DMELEE_NATIVE=1", "-D_CRT_SECURE_NO_WARNINGS",
              "-I" + str(sdk / "src"), "-I" + str(sdk / "extern/dolphin/include"),
              "-I" + str(output)]
    generator = args.generator.resolve() if args.generator else output / "script_item_pause_compiler_test.exe"
    if not args.generator:
        command = [*common, str(compiler_fixture), *map(str, compiler_sources),
                   "-o", str(generator)]
        commands.append(command); run(command, output, "compile_generator")
    action = output / "script_item_pause_action_fixture.h"
    callback = output / "script_item_pause_callback_fixture.h"
    command = [str(generator), str(action), str(callback)]
    commands.append(command)
    summaries = {"compiler": json.loads(run(command, output, "compiler_suite"))}
    originals(original_source, output / "script_item_pause_originals.h")
    for mode in ("action", "callback"):
        executable = output / ("script_item_pause_" + mode + "_test.exe")
        command = [*common, "-DPAUSE_QUERY_CALLBACKS=" + str(int(mode == "callback")),
                   str(module_fixture), "-o", str(executable)]
        commands.append(command); run(command, output, "compile_" + mode)
        command = [str(executable)]; commands.append(command)
        summaries[mode] = json.loads(run(command, output, "run_" + mode))
    for name, expected in hashes.items():
        if digest(Path(name)) != expected:
            raise RuntimeError(f"input changed during test: {name}")
    generated = [action, callback, output / "script_item_pause_originals.h"]
    receipt = dict(status="accepted bounded console fixture", summaries=summaries,
        commands=commands, input_sha256=hashes,
        generated_sha256={str(path):digest(path) for path in generated},
        scope="actual generated action/callback modules and unchanged item pause/resume bodies; creator lookup is an explicit controlled validated-creator boundary",
        pending=["whole engine item scheduling", "gameplay"])
    (output / "results.json").write_text(json.dumps(receipt, indent=2) + "\n",
                                        encoding="utf-8", newline="\n")
    print(json.dumps({"results":str(output / "results.json"), "summaries":summaries}))


if __name__ == "__main__":
    main()
