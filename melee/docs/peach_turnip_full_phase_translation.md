# Peach Turnip complete post-construction phases

This separate **unregistered source candidate** passes **4,509 actual-original/
generated pairs and 92,827 checks** against matching frozen34. It preserves the
accepted constructor candidate (`66abe5eac`) and available-phase subset
(`b41ffa76d`), which remain unchanged. It is not a complete Peach fighter,
default article replacement, or gameplay acceptance.

`tests/native/peach_turnip_full_phases.fms` adds thrown and state-2/3 collision
callbacks to the previously compared phase behavior. Constructor and pickup
initialization stay in their accepted separate candidate. Parasol/Toad cleanup
and retained-held-item restoration remain pending; this file does not install
the full fighter resource lifecycle.

Thrown preserves **state 2, flags 6 → face appearance → hitbox-0 damage →
remembered absolute scale**. The scale operation is the real
`it_80274484(gobj, gobj->hsd_obj, item.throw_scale)`: assign `Item.scl`, set all
root scale channels, refresh hit geometry, ECB geometry and model geometry in
the original order. Dropped preserves state 3/flags 6 and face appearance,
without scale restoration. `item.set_scale` is not substituted.

Both collision states read the complete Int mask once. The named owned Int
parameters are `surface_response_mask = 14` and `retire_contact_mask = 3`.
The first branch passes the unchanged mask to `it_80276FC4`, then enters state 1
and returns false. Only when the first mask does not match does the second
branch return true. These low-bit response masks are distinct from the
environment `Collision.floor_mask/wall_mask` constants. The comparison uses
the corrected mask-carrying native types, not historical `_Bool` normalization.

## Actual-source comparison boundaries

The new extractor compiles 23 unchanged production bodies, including Turnip
Thrown/collision and the real absolute-scale/stage-response implementations.
The actual HSD root scale inline and real falling/reflection/expiry math
execute. The actual callback lifetime registry executes; creator resolution
remains a controlled generation-aware fixture dependency.

Geometry refresh, hitbox damage, stage sweep, surface preparation, landing
predicate, ground transition, response and surface damage are **controlled
dependencies**. Their ordered calls and complete arguments are compared;
their internal geometry, damage scaling and collision-solving algorithms are
not validated here. In particular, the fixture's landing predicate can return
either value independently of its mask. Both branches of the actual shared
response body execute, and the Turnip state transition still occurs after the
response regardless of that result.

The executed matrix has 4,509 original/generated pairs: the prior 329 animation,
physics, drop/contact/reflection, interaction and destruction pairs; 84 thrown
combinations across seven remembered scales, three faces and four damages;
and 4,096 collision pairs across both states, every low-byte mask, four
high-bit prefixes and both landing-predicate results. Signed-zero scale is
compared by representation; no digest or state field is masked.

## Matching generation and console recipe

Peach has no clone-project registration. The existing console import entry
`tests/native/peach_turnip_composition_import.c` is compiled afresh against
frozen34's real parameter importer, parameter parser, article bindings and DAT
reader. It reads the same retail `ftDataPeach` DAT pinned by the accepted
constructor comparison. This performs normal owned-parameter conversion;
neither a fabricated clone project nor private parameter-storage mutation is
used. The first raw `script-c` attempt correctly rejected unresolved article
parameter declarations; generating the imported source succeeds.

Reproduce from the repository root with the receipt's console command files:

```powershell
cmd /c .local\peach-turnip-full-phases34\compile_import.cmd
.local/peach-turnip-full-phases34/import.exe .local/peach-turnip-composition30/retail-peach.dat tests/native/peach_turnip_full_phases.fms .local/peach-turnip-full-phases34/owned.fms .local/peach-turnip-full-phases34/fighter_parameters.fmp
.local/baseline34/bin/native_content_tool.exe script-c .local/peach-turnip-full-phases34/owned.fms peach_turnip_full_phases .local/peach-turnip-full-phases34/full_generated.inc
python tests/native/peach_turnip_full_phase_fixture.py --root .local/baseline34/sdk --output .local/peach-turnip-full-phases34 --generated .local/peach-turnip-full-phases34/full_generated.inc --script tests/native/peach_turnip_full_phases.fms --parameters .local/peach-turnip-full-phases34/fighter_parameters.fmp
cmd /c .local\peach-turnip-full-phases34\compile.cmd
.local/peach-turnip-full-phases34/full-phase-test.exe
```

The extractor emits `turnip_full_phase_actual.inc`, `full_phase_parameters.inc`
and `full_phase_source.json`. The actual generated parameter-binding export
loads the two imported owned mask values using the compiler's emitted schema;
the extractor checks their type, qualified name and value against the source
declarations. The test
does not overwrite private parameter storage.

Compile `tests/native/peach_turnip_full_phase_test.c` and frozen34's
`src/melee/ft/native_callback_lifetime.c` with frozen34 clang after `vcvars64.bat`,
using its SDK `src`/Dolphin includes and the generated include directory. Flags:
`-std=gnu11 -O2 -fms-runtime-lib=dll -DMELEE_NATIVE=1 -D_USE_MATH_DEFINES
-D_CRT_SECURE_NO_WARNINGS -D_DLL -D_MT`.

The direct stage-response Host field uses exact Dolphin `s32`, including its
Windows `long` typedef; the fixture binds the real function without a pointer
cast. The instance-scale Host accepts `(Item_GObj*, HSD_JObj*, float)` directly.

Evidence is `.local/peach-turnip-full-phases34/{result.json,full_phase_source.json,
import.log,generate.log,compile.log,run.log}`. The receipt hashes the frozen
manifest/tool/compiler, actual source/header inputs, original/generated bodies,
imported values, candidate, fixture and executable. The compile emits one
unused-result warning in generated `item.interact` code, with no test failure.
There are no gameplay tests, package/default activation, full resource-chain
comparison, removed/recycled creator cases, or callback-invalidation cases in
these phase results.

No global build or game process is required. A future CMake target can import,
generate the three includes/receipt, then compile the console test with the
real callback registry. Root owns that integration. Earlier subset/constructor
evidence remains separate and unchanged.
