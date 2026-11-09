#!/usr/bin/env python3
"""Audit the Windows native simulation's compiled math dependencies."""

import argparse
from pathlib import Path
import re
import subprocess


# These Aurora objects compute display data. The matrix and pad libraries are
# audited as simulation dependencies, even though they also serve the renderer.
DISPLAY_OBJECTS = {
    "rmlui.cpp.obj",
    "WebGPURenderInterface.cpp.obj",
    "shader_info.cpp.obj",
    "GXDraw.cpp.obj",
    "GXLighting.cpp.obj",
}
TRANSCENDENTALS = {
    name + suffix
    for name in (
        "sin", "cos", "tan", "sincos", "asin", "acos", "atan", "atan2",
        "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
        "log", "log2", "log10", "log1p", "exp", "exp2", "exp10", "expm1",
        "pow", "hypot", "cbrt", "erf", "erfc", "tgamma", "lgamma",
    )
    for suffix in ("", "f", "l")
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--llvm-nm", default="llvm-nm")
    parser.add_argument("--llvm-readobj", default="llvm-readobj")
    args = parser.parse_args()
    build = args.build.resolve()
    files = [
        build / "melee_runtime.lib",
        build / "melee_math.lib",
        build / "CMakeFiles/melee_native.dir/src/melee/gm/gmmain.c.obj",
        build / "rmlui.lib",
        *sorted((build / "aurora").rglob("*.lib")),
    ]
    if not (build / "aurora/aurora_mtx.lib").is_file():
        parser.error("Build melee_native and its Aurora libraries before auditing.")
    result = subprocess.run(
        [args.llvm_nm, "--undefined-only", "--print-file-name", *map(str, files)],
        check=True, capture_output=True, text=True,
    )
    failures = []
    presentation = set()
    for line in result.stdout.splitlines():
        match = re.search(r"\sU\s+(\S+)$", line)
        if not match:
            continue
        symbol = match[1].removeprefix("__imp_").lstrip("_")
        if symbol not in TRANSCENDENTALS:
            continue
        origin = line[:match.start()].rstrip(": ")
        library, _, member = origin.rpartition(":")
        is_display = (
            Path(library).name == "imgui.lib"
            or (Path(library).name == "rmlui.lib"
                and member in {"Math.cpp.obj", "PropertyParserColour.cpp.obj"})
            or (Path(library).name in {"aurora_core.lib", "aurora_gx.lib"}
                and member in DISPLAY_OBJECTS)
        )
        if is_display:
            presentation.add(symbol)
        else:
            failures.append(line)
    if failures:
        parser.exit(1, "Unexpected native math imports:\n" + "\n".join(failures) + "\n")
    executable = subprocess.run(
        [args.llvm_readobj, "--coff-imports", str(build / "melee_native.exe")],
        check=True, capture_output=True, text=True,
    )
    imports = {
        match.lstrip("_") for match in re.findall(r"^\s+Symbol: (\S+)", executable.stdout, re.M)
    } & TRANSCENDENTALS
    if imports - presentation:
        parser.exit(1, "Unattributed executable math imports: " +
                    ", ".join(sorted(imports - presentation)) + "\n")
    print(f"Audited {len(files)} native objects and archives. No simulation CRT transcendentals.")
    if imports:
        print("Display imports in melee_native.exe:", ", ".join(sorted(imports)))


if __name__ == "__main__":
    main()
