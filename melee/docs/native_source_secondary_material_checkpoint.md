# Owned static secondary-model construction checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-09. This closes checked material compilation and actual static Arrow
graph construction evidence. It does not enable source secondary-model metadata,
Item slot publication, rendering or whole-match rollback.

## Actual implementation

`native_article_model_admission.c` prepares an immutable imported descriptor at
an explicit transition. It checks the initialized effective JObj/DObj/PObj/MObj/
TObj classes, object sizes, allocation/load/release/draw methods and actual toon
environment. Missing class-name registry entries follow the real default-class
selection; conflicting entries or changed default methods reject preparation.
No global texture or class defaults are replaced to make an asset pass.

The descriptor census retains MaterialEnvironment until preparation succeeds.
Preparation claims its complete construction budget, creates actual standalone
native materials/textures, runs the checked native compiler, then removes them.
It never registers a JObj ID or changes a live Item slot. Failure preserves its
output; capacity rejection precedes construction. Successful preparation closes
that environment gate, with current-environment revalidation required before
later graph construction. Descriptors must remain immutable and pinned.

HSD_TExpCompile now takes an explicit Abort/Checked policy. Both execute the
same native optimizer, scheduler, constant assignment and descriptor conversion.
Checked reports invalid/cyclic graphs, stage limits, register packing and final
descriptor allocation failure without publishing partial output. Its input
validation uses a fixed 512-node table and iterative colored traversal; ordinary
Abort compilation avoids that extra input walk. All real callers use the changed
API directly. There is no separate approximate compiler or compatibility adapter.

HSD_MObjCompileTev preserves its existing shader if checked candidate compilation
fails. Ordinary compilation retains the native early removal order. Checked
construction still requires a prior allocation budget: expression/material
constructors retain native allocation assertions. Expected exhaustion must fail
at reservation Begin, not be described as recoverable loader failure.

The census additionally rejects IK and skin references outside the owned graph.
Lazy matrix scale-vector allocation remains a separate claim at actual traversal.

## Evidence

Receipts are under `A:/FreakMelee-test-archives/`:

| Cohort | Result | Scope |
| --- | --- | --- |
| `texp-checked-oct09-final` | 10,341 checks; 128 pre-edit compiler pairs; ten real packing failures; 3,800 allocations/frees | Actual compiler/DAG kernels; fixed test allocator and constructed expression inputs |
| `texp-material-reference-oct09-review2` | 341 materials, including all four imported Arrow roots; original/current numerical fingerprint `b2f9eb28f6234797` | Native expression builders/compiler, ordinary/reserved compilation and recompilation; comparison excludes addresses |
| `source-secondary-models-oct09-final1` | 565 checks; 24 actual HSD graphs; 16 matrix visits; four replacement commits; four exhaustion preflights; eight ID cases; three environment rejections | Actual current native library and copied owned Link/Young Link Arrow DATs at special-attribute offsets 36/40 |

Root reverified all 3,055 graph-cohort, 224 checked-kernel and 227 full-material
recorded input fingerprints unchanged. Graph receipt SHA256:
`3fec605e06c7ae66b72cf0bd8c199773e9b74c0ac7cc788a2a66cb9657a8e82a`.
Client, content tool and actual source-assets runtime target rebuild successfully;
all eight selected heap/allocator/DAT/pool/Item interpreter CTests pass.

Every actual Arrow root has one joint/material/texture. Two-construction plus
two-lazy-scale quotas require 5,752 bytes for offset 36 and 5,912 for offset 40.
Claims cause no ordinary heap growth. Every reserved bin and actual graph-class,
ID and Vec live count returns to zero. Destroying the older duplicate preserves
the replacement's ID. Destroying the newest first does not resurrect the older
mapping; all recoverable failure conditions must precede JObj registration.

## Remaining acceptance

These are console graph/material/allocator tests, not a game/render test. The
replacement sequence is fixture orchestration of actual native primitives, not
an implemented source Item replacement API. No snapshot completeness, page
rewind or timing acceptance follows from the local allocation checks.

Before source metadata is admitted, connect indexed package-owned model rows,
scene-owned reservation backing, Item slot identity/lifetime, guarded matrix/draw
claims, removal and captured backing/metadata/ID globals. Validate drawable inputs
too: current DAT imports permit zero texture repeat counts and absent image
descriptors, which actual texture matrix/setup code asserts on. The four tested
Arrow roots have valid values; material preflight alone is not general drawable
admission. Active toon/shadow and custom-class cases remain unsupported here.
Boomerang animation allocation and the separate Chain/Gumstrings/tether resource
collections remain open. The full project goal remains active.
