# Owned secondary descriptor and scene storage checkpoint

2026-10-09. This extends the checked-material/static-graph checkpoint with
immutable named imports and a real scene storage owner. Source `model_root`
metadata and static secondary operations now connect to the direct interpreter.
The initial ownership and history evidence below predates public admission;
the subsequent script-operation cohort is recorded separately. The full compact
match snapshot is still unfinished; remaining state uses the broad page oracle.

## Immutable preparation

`native_source_entity_models.c` prepares exact, owner-scoped named root records
from each Article's native32 special-attribute offsets. It uses the actual
character/blueprint import cursor, retaining cache/arena advancement rather than
copying an import cursor and overwriting later descriptors. Names are copied into
the package, not replaced with hashes. Separate source entities can share the
same physical Article without overwriting each other's root tables. A declared
NULL graph rejects; no declaration remains an empty table.

`itArticle_ImportModelRoot` contains the common substantive offset/relocation/
typed-graph import kernel. The older multi-root importer directly uses it and
retains its nullable-root semantics for historical native test exports. Ordinary
source preparation remains independent of HSD class/heap initialization.

## Bounded construction and drawing inputs

The descriptor census now rejects draw inputs that Melee itself dereferences or
asserts on: missing materials, zero texture repeats, absent images/image data,
unsupported texture formats, palettes without data, and invalid visible polygon
vertex/display inputs. Zero texture scale and fully culled polygons retain their
native behavior. The DAT importer still owns byte/index extent validation; this
trusted descriptor census does not establish arbitrary pointer validity.

`native_article_model_pool.c` combines allocation bins across alternate roots,
including lazy scale demand and a complete replacement allowance. Its caller
supplies stable captured backing and metadata. Preparation compiles real native
materials; construction calls the actual HSD loader under a complete reservation.
Matrix claims first establish JObj allocation extent, class and root identity.
Allocation exhaustion is rejected before any graph ID registration.

## Scene lifetime integration

`native_article_model_scene.c` owns one reservation and a fixed registry for the
512 source Item actors' three secondary slots. `Player_80036DA4` prepares it after
selected fighter publication and before first Fighter construction/history.
Backing is allocated once from the actual main HSD heap. Scene metadata, registry,
reservation bookkeeping and heap backing are covered by the existing broad page
capture; mutable graph storage is not placed in uncaptured package malloc.

Packages with named roots receive explicit scene leases. Each slot retains the
actual Item lifetime, published program, exact package/root table, graph pool
generation and scene epoch. Load preflights old-slot ownership and capacity,
constructs a separate candidate, then replaces/removes the old graph. An
impossible continuation failure after admitted standard construction is a scene
contract fault, not a recoverable failure: removing a candidate cannot restore
the previous descriptor-keyed global ID mapping.

The normal Destroy path revokes the registry before graph removal. Secondary draw
traversal claims lazy matrix storage and checks the retained owner afterward.
The registry retains physical/program identity across the Item's removal phase;
that phase does not forbid still-live Destroyed-hook observations. Each matrix
invocation separately captures the current removal flag and rejects a changed
flag on completion while balancing its allocation claim.
Before GObj retirement and heap reset, the scene requires drained history and an
idle callback/VM/claim boundary, removes surviving graphs, releases its reservation
and main-heap allocation, then releases package leases. It does not pretend that
logical GObj/actor retirement runs graph destructors.

## Acceptance boundary

The client, content tool, source-assets target and selected allocator/Item
interpreter targets build; eight selected CTests pass. Focused evidence is added
below. Static construction does not establish GPU rendering or whole
match behavior. Animation bundles and Chain/Gumstrings/tether collections remain
separate unimplemented ownership boundaries. Existing metadata/model0 restrictions
are intentional until the corresponding end-to-end source runtime path is proven.

| Private receipt under `A:/FreakMelee-test-archives/` | Result | Scope |
| --- | --- | --- |
| `hsd-draw-admission-oct09-final` | 11,330 checks; 43 draw-input cases; 341 shader comparisons; four actual Arrow roots | Census, native material builders/compiler and trusted drawable inputs; no GPU draw |
| `source-secondary-import-oct09-final1` | 142 checks; 16 cases; three entity imports; four pool constructions/matrix traversals | Exact named immutable imports before HSD bootstrap; actual bounded pool afterward; unchanged legacy import regressions |
| `source-article-model-scene-oct09-final` | 80 checks; seven cases; four graph constructions; two matrix traversals | Real compiler/publication, Item actor/storage/VM, scene/main heap, replacement, removal-phase inspection and teardown |

Root reverified all 364 draw-cohort and 3,171 frozen import/pool input fingerprints
unchanged. Import receipt SHA256:
`b7b6ac4860e0f67a6eb9f1059a59a60b4b258ea833d0535898352acdebc0d7fd`.
All four Arrow alternatives share one 15,320-byte pool for two live graphs and
one replacement. Tests reject exhausted construction without changing backing,
metadata or IDs, reject ordinary heap roots for reserved matrix claims, and
require graph removal before release. These byte copies check rejected-operation
preservation; they are not save/load/rollback evidence. The character cursor path
is exercised; blueprint cursor ownership is implemented and source-reviewed but
not exercised by this Arrow corpus. The actual metadata validator still rejects
the new declarations.

The scene cohort uses an explicitly staged unpublished source candidate with real
owned Link DAT imports and typed roots. It executes a minimal Article animation
body in the actual VM, then exercises the production scene owner. Private runtime
and main-heap transition tables receive controlled setup inputs; their algorithms
are unchanged. Published Item identity differs from parent Fighter identity and
the test covers that distinction, rejected borrowed slots, replacement, native
ID mapping, balanced claims and package lease release. Userdata cleanup is a
controlled callback invoking real Destroy/Detach, not the complete Item scheduler.
Same-kind duplicate runtime rows exercise the getter's first-match rule rather
than distinct-kind package deduplication. No rollback history is initialized;
captured production placement is source-reviewed, not restoration acceptance.

Root reverified all 3,055 final scene-cohort input fingerprints unchanged after
the production rebuild. Scene receipt SHA256:
`2787fbd49aa826abe3ff5f487552dcdbbbb01a0dbcc557e48d50f6cc86ffe21e`.
The final client/content/reference build and all eight selected CTests pass.
No game was launched. The next acceptance boundary is actual broad-history
restoration and direct interpreter secondary operations before public admission.

## Connected broad-history restoration

The subsequent `source-article-model-history-final` cohort uses the actual
production region builder and Windows page history, not a replacement restore
algorithm. It discovers the scene registry, reservation backing, HSD class/ID
state, actual main-heap Item userdata, actors and GObj pools in production regions.
The privately included runtime publication tables remain host state as in the
actual CMake configuration; the heap/scene bodies use the actual `.mstate` section.
Fixture controllers and expected observations stay outside captured memory.

Across eight frame executions and two restores, 275 checks cover load/replacement,
lazy scale allocation, destruction, GObj slot retirement/reuse, registry state,
package retention, allocator generations/free order, descriptor IDs and VM storage.
Replaying the same three-frame sequence matches the complete discovered-region
hash plus explicit graph/material/topology and owner observations. Scene retirement
is refused while history exists and succeeds after actual history shutdown.

Two paged and four exact regions total 83,583,012 tracked bytes in this console
setup. This is neither a compact snapshot size nor a full-match performance result.
The real Arrow root is a one-joint graph; full native Item factory/scheduler, GPU
rendering, public secondary metadata and whole-match restoration are outside this
proof. Candidate/root/userdata setup remains controlled and explicit.

Root reverified all 3,249 frozen inputs unchanged. Receipt SHA256:
`3acdd20a21e680913144cf7973ebacdd7c838b414cf77d3e908870586040525a`.
No production changes, shared rebuild or game launch were required for this
test-only cohort. Public admission still awaits connected interpreter operations;
animation bundles, collections and the complete explicit match snapshot remain open.

## Direct interpreter static-model admission

The subsequent operation batch enables exact owner-scoped `model_root`
declarations, `item.load_model`, `item.destroy_model`, draw masks, model 0–3
joint properties including raw quaternion channels, bulk transforms and world
queries. One shared grammar serves immutable import and compiler validation;
root identifiers cannot collide with source/native symbols or operator keywords.
The importer and metadata validator agree on names, offsets, ordinals, counts
and nonnull graphs before publication. Standalone compiler source lists directly
include the new grammar unit.

Each runtime secondary projection retains the actual Item, scene epoch and graph
pool generation. Empty slots return native defaults without aliasing the primary
root. Bulk transforms use native translation/rotation/scale setters in order;
nonfinite inputs leave the graph unchanged. World queries claim reserved lazy
matrix storage and balance it before publishing all three outputs. Loading a
replacement preserves the old graph on preflight capacity failure.

The rebuilt client/content/source-assets and selected allocator/Item targets
pass; eight selected CTests pass. Three focused console cohorts provide:

| Receipt under `A:/FreakMelee-test-archives/` | Result | Boundary |
| --- | --- | --- |
| `source-script-item-models-oct09-final` | 217 checks in ten groups for each of Link and Young Link; eighteen secondary constructions and two nonempty VM matrix claims total | Actual imported roots, source compiler/publication, actor/storage/VM, native HSD setters and scene owner |
| `source-secondary-import-oct09-model-api-final1` | 198 checks; sixteen cases; four graph constructions and matrix visits | Exact metadata admission/rejection and independent immutable owners; actual pool |
| `script-item-model-oct09-secondary-api-final1` | 2,594 checks; 151 native comparisons | Existing primary-model behavior; unselected secondary boundaries abort explicitly |

Root independently reverified all 3,063 connected-cohort and 3,173 import-cohort
inputs unchanged. The primary cohort's production/fixture inputs remain unchanged;
its captured Item API and fixture README were subsequently corrected to describe
the new public support, so those two documentation fingerprints differ. No code
change or additional test run is inferred from that documentation update.

Connected receipt SHA256:
`66da3bdaf0fbf949a214befee4ff8712d65c916af60117f8e640b0dd79a266c6`.
Import receipt SHA256:
`aae639fd83f10fc19ff7390895313ef3b5e923c0a93a85635bf534d99c162f24`.
Primary receipt SHA256:
`586e72092cd98ee8b92d82dc1c44df7677e71e4909aa82cc8b7a87aaefb84d91`.

The connected fixture stages real imported Arrow descriptors with controlled
runtime/heap transition inputs and Item userdata construction/cleanup. It runs
the same source separately for both owners, rejects fifteen malformed source
cases per owner without replacing accepted code, faults an actual VM call that
borrows another Item's graph, and rejects a retained generation after destroy/
reload. It compares transform fields against actual native setters. It does not
run the full native Item factory/scheduler, the draw hook, a GPU or Workshop.
The earlier actual page-history restoration proof is independent; this operation
cohort does not initialize history. No game was launched.

Static support does not activate the complete Arrow translation. Remaining
ownership/contact operations, animation bundles and resource collections are
separate work. The complete compact match snapshot remains unfinished, and the
full project goal stays active.

## Owned animation-bundle import and allocation census

The next foundation adds immutable owner-scoped `model_animation NAME OFFSET`
tables. The offset addresses three serialized pointers: joint, material and
shape animation. The importer uses the actual character or Item blueprint DAT
cursor, preserves cached graph identities and copies exact names into the
package. Distinct source entities can share immutable descriptors without
sharing their declaration tables. Valid null components remain null; names
are never published into the stock Article's hash table.

The single-bundle typed importer is also used directly by existing stock
imports. Those imports still publish their entire table only after every
component succeeds. The direct interpreter deliberately rejects animation
metadata with an explicit diagnostic: imported descriptors do not yet imply
bounded runtime storage or executable operations. Import-time lexical names
and compiler keyword/symbol admission remain separate boundaries.

A new transition-only census counts native AObj/FObj applications, including
shared descriptors applied to multiple targets, sparse positional matching,
first matching texture IDs and native instance boundaries. It uses fixed work
limits, detects cycles and validates compressed scalar operand bounds. Counts
and coverage flags are written only on success. Callback channels, referenced
objects, shapes, palette storage and unproven channel conversions remain
explicit gaps; the census is neither free-pool admission nor a complete value
proof. It does not allocate or attach native runtime objects.

Link and Young Link's real Boomerang root offsets are 68/72; their bundle
triplets begin at 76/88. Each matching root/bundle pair contains one controller
and one track with no census coverage gaps. Native scalar request/evaluation
comparisons cover CON, LIN, SPL0, SPL, SLP and KEY, including KEY waits and
terminal values without final waits. Synthetic malformed-stream and descriptor
cases are distinguished from these actual imported graphs.

The client/content/source-assets and selected runtime targets rebuild; all
eight selected allocator/Item CTests pass. The refreshed static import/pool
cohort passes 198 checks in sixteen cases, with four actual graph constructions
and four matrix visits. Root independently verified its 3,177 frozen inputs and
3,176 live source fingerprints. Receipt:
`A:/FreakMelee-test-archives/source-secondary-import-oct09-animation-foundation/receipt.json`,
SHA256 `77fd20c58dedf70d6a3166ed99bb01b021ba03e051d38e914cbe0aaad314ba63`.

The animation foundation cohort passes 421 checks in 24 cases, five entity
preparations and 40 actual native scalar comparisons. Root independently
verified all 3,177 frozen inputs and 3,176 live source fingerprints. Receipt:
`A:/FreakMelee-test-archives/source-secondary-animation-import-oct09-final/receipt.json`,
SHA256 `cf0941830ae9cc02879607d241f76a62e37303bee5e22a85685f834284bf3c8e`.
Its native evaluator uses stack FObj leaves and a recording callback. It does
not initialize HSD pools, attach controllers to live graphs, run the Item VM
or draw pipeline, or restore history. The first review run's expected rejection
of lexical names `and`/`not` was corrected in the fixture; public animation
metadata remains rejected, and no production policy was changed for that test.

### Runtime continuation

Connect exact prepared model/path/bundle selectors to native add/request/
advance/remove operations, with capacity checked before changing a live graph.
AObj/FObj already use fixed native pools; preparation must account for their
actual free capacity. Sparse bundles can retain prior controllers, so a single
bundle's demand is not the maximum retained union. Palette tables and shape
blend storage require distinct ownership; animation removal alone does not
release every texture table. Keep uncovered families rejected until their
allocation and callback contracts are implemented.

Retain the same Item/scene/graph generations across playback and replacement,
pin descriptor arenas and DAT streams until history drains, and prove the
mutable controllers through actual history restoration. Preserve the stock
Boomerang add/request sequence without an invented initial animation advance.
The native subtree rate helper changes descendant joint AObjs, excluding the
selected root and material/texture/shape controllers.

The native decoder also exposed a separate Workshop follow-up: several paths
in `native_fighter_animation_editor.c` omit KEY waits, although `FObjAnimKey`
enters the native wait-loading state. Review that reader/writer contract with
native evaluation evidence before changing it; this batch does not alter the
Workshop animation editor.

No game was launched. This foundation does not activate Boomerang behavior,
finish Arrow ownership/contact operations or resource collections, or complete
compact whole-match snapshots. The full project goal remains active.

## Public interpreter animation playback

The subsequent runtime batch opens exact `model_animation` metadata and connects
add/request/advance/remove/rate to prepared literal model/path/bundle selectors.
These are direct interpreter operations, not generated C. Named bundles remain
same-owner immutable assets. Add returns Bool and checks the actual fixed
AObj/FObj pools before mutation, conservatively without reclaim credit; vacant
slots and missing paths are no-ops. Native operation ordering is preserved with
no invented initial animation advance.

Native controller constructors now retain their immutable descriptor origins.
Transition preparation records actual applications to joint/material/texture
targets, including selected subtrees and admitted native primary Item states.
Runtime validation covers the entire model graph and controllers retained by
sparse earlier additions, exact native classes/topology, allocation lifetimes,
generations and track payloads. It does not decode compressed streams or walk
the entire pool every tick. Palette/shape/reference/callback channels and
unsupported graph classes remain rejected. End callbacks and callback reentry
are rejected before native mutation.

Packages using these operations remain leased through the scene/history owner,
including primary-only animation programs. Leasing follows authored animation
selectors rather than controller count, so an empty bundle or request/remove-only
program with no controller origins still has its owner retained. Publication rebases internal origin
and selector arrays: comparing their prepared and published addresses was a
real ownership bug, fixed by retaining the canonical published-program identity
check and comparing external asset tables/counts instead.

The actual client, content tool, source-assets target and selected runtime tests
rebuild; all eight allocator/Item CTests pass. The refreshed import foundation
passes 413 checks in 24 cases, five entity preparations and 40 native scalar
comparisons. Valid metadata now passes; lexical `and`/`not` still fail compiler
namespace validation. Root verified 3,182 frozen and 3,181 live fingerprints.
Receipt:
`A:/FreakMelee-test-archives/source-secondary-animation-import-oct09-public-closure/receipt.json`,
SHA256 `666b5cc0e454ea54211c8d8ce7605ea71868e1090c30adb8c057e768f801f004`.

The connected cohort compiles/prepares/publishes genuine source for Link and
Young Link and runs actual Item actors/VM calls against native HSD/lb operations.
Its 3,188 checks in 104 cases include 244 native graph comparisons, 16 executed
history frames and four actual Regions/Windows Pages restores. Full owner runs
each contribute 1,465 checks, 110 graph comparisons and two restores; primary-only
variants each contribute 129 checks and twelve comparisons, with four operation
selectors, no named roots/bundles and eight origins from native primary states.
They are not zero-origin fixture claims.

The sparse case applies a real trail bundle to the unchanged primary graph at
root and child. This is a controlled application, not retail Boomerang behavior:
the actual bundle has no root controller, but animates a child. The deeper
controller's exact allocation generation survives the later root Add. Rate
checks select that real animated child and confirm root exclusion. Foreign
controller/track origins, end callbacks/reentry, nonfinite inputs, pool exhaustion,
generation exhaustion and borrowed graphs preserve the old graph/output or
poison only the incorrect actor, as appropriate.

History covers actual heap Item userdata, actors, scene registry/model storage,
AObj/FObj backing and generations; hashes match after restoration and replay.
Candidate assets and test counters remain outside tracked mutable state. Direct
candidate/Item initialization and cleanup are controlled transition inputs;
production Item constructors/destructors and full scheduler launches are not
covered. The fixture uses the production zero-initialized section compiler flag.
Receipt:
`A:/FreakMelee-test-archives/source-script-item-animations-oct09-review8/receipt.json`,
SHA256 `f8a36f00a650d57d9262595faef9877a319d45761c0b82a66449bc6223afb7bd`.
Root independently verified all 3,179 frozen inputs and 3,178 unchanged live
inputs; the sole changed live input is the README's post-run results paragraph.
Code, runner, assets and rebuilt libraries still match their captured fingerprints.

No game/GPU/audio launch occurred. This batch does not claim complete Boomerang
activation, resource collections, whole-match compact snapshots or rollback
performance. Those remain active continuation work.
