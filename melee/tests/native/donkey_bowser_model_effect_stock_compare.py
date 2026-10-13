"""One console batch comparing full-example authored effects to original C.

All writes stay under a fresh --output directory. Input project assets are
copied before parameter import, which writes an adjacent owned FMP. No game,
renderer, global build, registration or input-project modification occurs.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--sdk", type=Path, required=True)
p.add_argument("--tool", type=Path, required=True)
p.add_argument("--clang", type=Path, required=True)
p.add_argument("--output", type=Path, required=True)
p.add_argument("--donkey-project", type=Path, default=root / ".local/donkey-owned-final-heavy/project")
p.add_argument("--bowser-project", type=Path, default=root / ".local/bowser-capture-final/project")
a = p.parse_args()
sdk, tool, clang, out = (x.resolve() for x in (a.sdk, a.tool, a.clang, a.output))
if out.exists():
    raise ValueError("Use a fresh output directory; accepted evidence is immutable")
out.mkdir(parents=True)

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def run(arguments):
    result = subprocess.run([str(x) for x in arguments], cwd=root,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    print(result.stdout, end="", flush=True)
    if result.returncode:
        raise RuntimeError(f"Console process exited {result.returncode}: {arguments[0]}")
    return result.stdout.strip()

# Cover the actual compiler, all included SDK headers, linked production units,
# original stock source and source-only fixture files before and after the run.
production = ["melee/ef/eflib.c", "sysdolphin/baselib/jobj.c",
    "melee/lb/native_model_joint.c", "melee/ef/native_model_effect.c",
    "melee/ft/native_callback_lifetime.c", "melee/ft/native_fighter_parameters.c"]
inputs = [tool, clang] + [sdk / "src" / name for name in production]
inputs += list((sdk / "src").rglob("*.h"))
inputs += list((sdk / "extern/dolphin/include").rglob("*.h"))
inputs += [root / "tests/native" / name for name in (
    "donkey_bowser_model_effect_stock_compare.py", "donkey_bowser_model_effect_stock_fixture.py",
    "donkey_bowser_model_effect_stock_test.c")]
families = [("donkey", "donkey_wip.fms", a.donkey_project.resolve()),
    ("bowser", "koopa_wip.fms", a.bowser_project.resolve())]
for _, template, project in families:
    inputs.append(root / "examples" / template)
    inputs += [path for path in project.rglob("*") if path.is_file() and
        path.suffix.lower() not in (".dll", ".lib", ".exe", ".c")]
input_hashes = {str(path): digest(path) for path in inputs}
flags = [clang, "-std=gnu11", "-O2", "-DMELEE_NATIVE=1",
    "-D_CRT_SECURE_NO_WARNINGS", "-D_USE_MATH_DEFINES",
    "-fno-strict-float-cast-overflow", "-ffp-contract=off", "-fuse-ld=lld",
    "-I" + str(sdk / "src"), "-I" + str(sdk / "extern/dolphin/include")]
module_inputs = []
generated_files = []
for family, template, project in families:
    copied = out / family
    # Exclude derived behavior and parameters so the current full template is
    # imported anew. Do not write into the source project even temporarily.
    shutil.copytree(project, copied, ignore=shutil.ignore_patterns(
        "*.dll", "*.lib", "*.exe", "*.c", "behavior.fms", "fighter_parameters.fmp"))
    behavior = copied / "behavior.fms"
    run([tool, "import-parameters", copied, root / "examples" / template, behavior])
    generated = out / (family + ".generated.c")
    module = out / (family + ".dll")
    run([tool, "script-c", behavior, "effect_stock_" + family, generated])
    run(flags + ["-shared", generated, "-o", module])
    generated_files.append(generated)
    module_inputs.extend((module, copied / "fighter_parameters.fmp"))
fixture = out / "donkey_bowser_model_effect_stock_fixture.h"
run([sys.executable, root / "tests/native/donkey_bowser_model_effect_stock_fixture.py",
    sdk / "src", fixture])
test = out / "model_effect_stock_test.exe"
run(flags + ["-I" + str(out), root / "tests/native/donkey_bowser_model_effect_stock_test.c",
    sdk / "src/melee/lb/native_model_joint.c",
    sdk / "src/melee/ef/native_model_effect.c",
    sdk / "src/melee/ft/native_callback_lifetime.c",
    sdk / "src/melee/ft/native_fighter_parameters.c", "-o", test])
result = run([test] + module_inputs)
counts = re.search(r"(\d+) comparisons, 0 failures; (\d+) production-registry/lifetime rejection cases", result)
if not counts:
    raise RuntimeError("Source fixture did not report acceptance counts")
for path, expected in input_hashes.items():
    if digest(Path(path)) != expected:
        raise RuntimeError(f"Input changed during the batch: {path}")
evidence = {
    "scope": "original unchanged stock effect callbacks versus actual full-example generated DLL callbacks; real shared HSD channels, registry, creator identity and lifetime backend",
    "boundaries": ["synthetic common/base motion rows and active Definition lookup",
        "fake EF allocation/tagged spawn and deferred GObj free boundary",
        "fixture recipe classifier admits the two recipes; actual efSync provenance is covered separately",
        "no efLib_Update animation scheduling or renderer",
        "effect-specific rollback restore not tested; existing general rollback checks remain separate"],
    "output": result, "comparisons": int(counts[1]), "failures": 0,
    "lifetime_rejections": int(counts[2]), "input_sha256": input_hashes,
    "generated_sha256": {path.name: digest(path) for path in generated_files},
    "extracted_fixture_sha256": digest(fixture),
    "imported_source_sha256": {str(path.relative_to(out)): digest(path)
        for family, _, _ in families for path in (
            out / family / "behavior.fms", out / family / "fighter_parameters.fmp")},
}
(out / "acceptance.json").write_text(json.dumps(evidence, indent=2) + "\n",
    encoding="utf-8", newline="\n")
