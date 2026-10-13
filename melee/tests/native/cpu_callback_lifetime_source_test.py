"""Extract actual CPU callback continuations and prove removal/generation guards."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

REPO = Path(__file__).resolve().parents[2]
FUNCTIONS = ["ftCo_CpuClearTargetAndFinish", "ftCo_800B630C",
             "ftCo_CpuTapRAndWaitFiveFrames", "ftCo_800B658C",
             "ftCo_CpuSetNeutralStick", "ftCo_800B9020", "ftCo_800B9CBC"]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def end_brace(text, opening):
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return end


def body(text, name):
    match = re.search(r"^[^\n;{}]*\b" + name + r"\s*\([^;{}]*\)\s*\{", text, re.M)
    if not match:
        raise RuntimeError(f"missing actual function: {name}")
    return text[match.start():end_brace(text, text.index("{", match.start()))]


def extract(cpu, common, output, planner_name="ftCo_CpuPlanAttack"):
    originals = [body(cpu, name) for name in FUNCTIONS]
    outer = body(common, planner_name)
    start = outer.index("if (data->x18 != 3)")
    closing = end_brace(outer, outer.index("{", start))
    tail = re.match(r"\s*else\s*\{", outer[closing:])
    if tail is None:
        raise RuntimeError("outer candidate else continuation absent")
    end = end_brace(outer, closing + tail.end() - 1)
    fragment = outer[start:end]
    originals.append(f"// Exact candidate if/else from {planner_name}; surrounding selection excluded.\n"
                     "static bool cpu_outer_candidate_fragment(Fighter* fp) {\n"
                     "    struct CpuFighter* data = &fp->cpu;\n" + fragment +
                     "\n    return false;\n}\n")
    originals.append(body(common, "ftCo_800ABA34"))
    charge = body(common, "ftCo_800ACD5C")
    # Exact startup prefix, ending before the independent charge-decision
    # callback/raw character dispatch. Those later policies are out of scope.
    stop = charge.index("#ifdef MELEE_NATIVE\n    const NativeFighterDefinition* authored") if \
        "#ifdef MELEE_NATIVE\n    const NativeFighterDefinition* authored" in charge else \
        charge.index("    if (fp->kind == Ft_Kind_Donkey)")
    originals.append(charge[:stop].replace("ftCo_800ACD5C", "cpu_pending_charge_prefix", 1) + "}\n")
    output.write_text("// Actual unchanged source bodies and explicitly scoped outer block.\n" +
                      "\n\n".join(originals) + "\n", encoding="utf-8", newline="\n")


def run_logged(command, output, name):
    result = subprocess.run(command, cwd=REPO, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    (output / (name + ".log")).write_text(result.stdout, encoding="utf-8", newline="\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--source", type=Path, default=REPO / "src/melee/ft/ftcpuattack.c")
    parser.add_argument("--common", type=Path, default=REPO / "src/melee/ft/kinds/ftCommon/ftCo_0A01.c")
    parser.add_argument("--extract-only", action="store_true")
    parser.add_argument("--reject-revision", default="4df2ecc99")
    args = parser.parse_args()
    source, common, output = args.source.resolve(), args.common.resolve(), args.output.resolve()
    if args.extract_only:
        extract(source.read_text(encoding="utf-8"), common.read_text(encoding="utf-8"), output)
        return
    if args.snapshot is None:
        parser.error("--snapshot is required")
    snapshot = args.snapshot.resolve()
    if output == snapshot or snapshot in output.parents:
        parser.error("output must be outside snapshot")
    output.mkdir(parents=True, exist_ok=True)
    sdk, compiler = snapshot / "sdk", snapshot / "compiler/clang.exe"
    manifest_path = snapshot / "freeze.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    frozen = {n: h for n, h in manifest.items()
              if n.startswith(("sdk\\", "compiler\\", "sdk/", "compiler/"))}
    if not frozen:
        raise RuntimeError("empty SDK/compiler freeze inventory")
    for name, expected in frozen.items():
        if digest(snapshot / name) != expected:
            raise RuntimeError(f"frozen input changed: {name}")
    header = output / "cpu_callback_originals.h"
    extract(source.read_text(encoding="utf-8"), common.read_text(encoding="utf-8"), header)
    inputs = [source, common, compiler, Path(__file__).resolve(),
              REPO / "tests/native/cpu_callback_lifetime_test.c",
              sdk / "src/melee/ft/native_callback_lifetime.c"]
    hashes = {str(p): digest(p) for p in inputs}
    base = [str(compiler), "-std=gnu11", "-O2", "-fuse-ld=lld", "-DMELEE_NATIVE=1",
            "-D_CRT_SECURE_NO_WARNINGS", "-I" + str(sdk / "src"),
            "-I" + str(sdk / "extern/dolphin/include"), "-I" + str(output),
            str(inputs[-2]), str(inputs[-1])]
    command = base + ["-o", str(output / "cpu_callback_lifetime_test.exe")]
    result = run_logged(command, output, "compile")
    if result.returncode:
        raise RuntimeError(f"compile failed: {result.stdout}")
    result = run_logged([command[-1]], output, "run")
    if result.returncode:
        raise RuntimeError(f"matrix failed ({result.returncode}): {result.stdout}")
    receipt = json.loads(result.stdout)
    accepted_header_hash = digest(header)
    old = subprocess.check_output(["git", "show", args.reject_revision + ":src/melee/ft/ftcpuattack.c"],
                                   cwd=REPO, text=True)
    old_common = subprocess.check_output(["git", "show", args.reject_revision +
                                         ":src/melee/ft/kinds/ftCommon/ftCo_0A01.c"], cwd=REPO, text=True)
    (output / "rejected_cpu_source.c").write_text(old, encoding="utf-8", newline="\n")
    (output / "rejected_common_source.c").write_text(old_common, encoding="utf-8", newline="\n")
    extract(old, old_common, header, planner_name="ftCo_800ADE48")
    rejection_header_hash = digest(header)
    old_command = base + ["-o", str(output / "cpu_callback_lifetime_rejected.exe")]
    old_compile = run_logged(old_command, output, "rejected_compile")
    if old_compile.returncode:
        raise RuntimeError(f"old source must compile before rejection: {old_compile.stdout}")
    rejected = run_logged([old_command[-1], "--reject-travel-removal"], output, "rejected_run")
    if rejected.returncode & 0xffffffff != 0xc0000005:
        raise RuntimeError(f"expected old source access violation, got {rejected.returncode}: {rejected.stdout}")
    extract(source.read_text(encoding="utf-8"), common.read_text(encoding="utf-8"), header)
    if digest(header) != accepted_header_hash:
        raise RuntimeError("accepted extraction changed")
    for name, expected in hashes.items():
        if digest(Path(name)) != expected:
            raise RuntimeError(f"source changed during batch: {name}")
    receipt.update(input_sha256=hashes, command=command,
        snapshot_manifest_sha256=digest(manifest_path), originals=FUNCTIONS,
        extracted_header_sha256=accepted_header_hash,
        outer_scope="exact ftCo_CpuPlanAttack candidate if/else only; earlier/later CPU selection excluded",
        rejected_revision=args.reject_revision, rejected_exit_hex="0xc0000005",
        rejected_cpu_sha256=digest(output / "rejected_cpu_source.c"),
        rejected_common_sha256=digest(output / "rejected_common_source.c"),
        rejected_header_sha256=rejection_header_hash,
        boundaries=["controlled Definition lookup and engine command/classifier helpers",
                    "actual frozen callback lifetime/live-list resolver", "no authored CPU policy or gameplay acceptance"])
    (output / "results.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({"results": str(output / "results.json"), **receipt}))


if __name__ == "__main__":
    main()
