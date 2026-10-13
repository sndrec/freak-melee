"""Run the whole Bow/Arrow source-only acceptance batch on an immutable SDK.

This builds isolated console fixtures/DLLs only. It never builds or launches
Melee, edits the supplied snapshot, or registers a fighter override.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--snapshot", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
fixture_inputs = Path(__file__).resolve().parent / "link_bow_arrow"
parser.add_argument("--arrow-link", type=Path,
                    default=fixture_inputs / "link/behavior.fms")
parser.add_argument("--arrow-young", type=Path,
                    default=fixture_inputs / "young_link/behavior.fms")
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
snapshot = args.snapshot.resolve()
out = args.output.resolve()
out.mkdir(parents=True, exist_ok=True)
sdk = snapshot / "sdk"
tool = snapshot / "bin/native_content_tool.exe"
clang = snapshot / "compiler/clang.exe"

def run(arguments):
    process = subprocess.run([str(x) for x in arguments], cwd=root, text=True,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    print(process.stdout, end="", flush=True)
    if process.returncode:
        raise RuntimeError(f"Console fixture exited {process.returncode}: {arguments[0]}")
    return process.stdout.strip()

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

manifest = json.loads((snapshot / "freeze.json").read_text(encoding="utf-8"))
snapshot_inputs = {name: value for name, value in manifest.items()
                   if name.startswith(("sdk\\", "compiler\\")) or
                   name == "bin\\native_content_tool.exe" or
                   (name.startswith("bin\\") and name.endswith(".dll"))}

def verify_snapshot():
    for name, expected in snapshot_inputs.items():
        if digest(snapshot / name) != expected:
            raise RuntimeError(f"Frozen input differs from manifest: {name}")

verify_snapshot()
script_inputs = [root / "tests/native/link_bow_article_fixture.fms",
                 root / "tests/native/link_bow_recipient_fixture.fms",
                 args.arrow_link.resolve(), args.arrow_young.resolve()]
script_inputs += [p.with_name("fighter_parameters.fmp") for p in script_inputs[2:]]
input_hashes = {str(p): digest(p) for p in script_inputs}
flags = [clang, "-std=gnu11", "-O2", "-DMELEE_NATIVE=1", "-D_CRT_SECURE_NO_WARNINGS", "-D_USE_MATH_DEFINES",
         "-fno-strict-float-cast-overflow", "-ffp-contract=off", "-fuse-ld=lld",
         "-I" + str(sdk / "src"), "-I" + str(sdk / "extern/dolphin/include")]
author = out / "bow.generated.c"
recipient = out / "recipient.generated.c"
run([tool, "script-c", root / "tests/native/link_bow_article_fixture.fms", "bow_source", author])
run([tool, "script-c", root / "tests/native/link_bow_recipient_fixture.fms", "bow_recipient", recipient])
run(flags + ["-shared", author, "-o", out / "bow.dll"])
run(flags + ["-shared", recipient, "-o", out / "recipient.dll"])
queries = out / "bow.stock_queries.c"
run([sys.executable, root / "tests/native/link_bow_stock_fixture.py",
     sdk / "src/melee/ft/kinds/ftLink/ftlinkspecialn.c", queries])
run(flags + [root / "tests/native/fighter_link_bow_script_test.c", queries,
     sdk / "src/melee/it/kinds/itlinkbow.c", sdk / "src/melee/ft/native_fighter_events.c",
     sdk / "src/melee/ft/native_callback_lifetime.c",
     root / "tests/native/model_effect_absent_fixture.c", "-o", out / "bow.test.exe"])
bow_result = run([out / "bow.test.exe", out / "bow.dll", out / "recipient.dll"])
arrow_sources = []
for family, source in (("link", args.arrow_link.resolve()), ("young", args.arrow_young.resolve())):
    generated = out / (family + ".generated.c")
    run([tool, "script-c", source, "bow_arrow_" + family, generated])
    arrow_sources.append(generated)
arrow_fixture = out / "arrow_reflection_fixture.h"
run([sys.executable, root / "tests/native/link_arrow_reflection_fixture.py",
     sdk / "src/melee/it/kinds/itlinkarrow.c", sdk / "src/melee/it/native_article_model.c",
     arrow_sources[0], arrow_fixture, "--young-generated", arrow_sources[1]])
run(flags + ["-I" + str(out), root / "tests/native/fighter_link_arrow_reflection_test.c",
             "-o", out / "arrow.test.exe"])
arrow_result = run([out / "arrow.test.exe"])
bow_counts = re.search(r"(\d+) comparisons, (\d+) failures; (\d+) restored cleanup cases; (\d+) missing-export", bow_result)
arrow_counts = re.search(r"(\d+) comparisons, (\d+) failures", arrow_result)
if not bow_counts or not arrow_counts:
    raise RuntimeError("Fixture did not report acceptance counts")
verify_snapshot()
if any(digest(Path(p)) != value for p, value in input_hashes.items()):
    raise RuntimeError("Authored input changed during the source batch")
results = {
    "snapshot": str(snapshot), "freeze_sha256": digest(snapshot / "freeze.json"),
    "scope": "original stock C versus authored C; independent recipient DLLs and real dispatch/lifetime backend; console only; no gameplay or registration",
    "bow_output": bow_result, "arrow_output": arrow_result,
    "counts": {"bow_comparisons": int(bow_counts[1]), "bow_failures": int(bow_counts[2]),
               "restored_cleanup": int(bow_counts[3]), "missing_export_policy": int(bow_counts[4]),
               "arrow_comparisons": int(arrow_counts[1]), "arrow_failures": int(arrow_counts[2])},
    "snapshot_inputs_verified_before_and_after": len(snapshot_inputs),
    "authored_input_sha256": input_hashes,
    "stock_query_sha256": digest(queries),
    "model_source_sha256": {name: digest(sdk / "src" / name) for name in (
        "melee/it/native_article_model.c", "melee/lb/native_model_joint.c",
        "melee/lb/native_model_joint.h")},
    "generated_sha256": {p.name: digest(p) for p in [author, recipient] + arrow_sources},
    "fixture_sha256": {name: digest(root / "tests/native" / name) for name in (
        "fighter_link_bow_script_test.c", "fighter_link_arrow_reflection_test.c",
        "link_bow_article_fixture.fms", "link_bow_recipient_fixture.fms",
        "link_bow_stock_fixture.py", "link_arrow_reflection_fixture.py",
        "link_bow_arrow_source_compare.py", "model_effect_absent_fixture.c")},
}
(out / "acceptance.json").write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8", newline="\n")
