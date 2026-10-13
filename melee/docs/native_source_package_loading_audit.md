# Direct source package loading audit

> Historical pre-migration callback inventory. Ordinary Workshop/package loading now uses owned source candidates and interpreted programs, including paired members. See [interpreter status](native_interpreter_transition.md), [Workshop status](native_character_workshop_progress.md) and current source before reusing the old migration tasks below.

Initial source audit, 2026-10-07. The tables below record the pre-migration
architecture and explain the migration boundaries; they are not the current
package protocol. The ordinary format now carries owned asset facts and FMS,
and Workshop validation/build uses the source candidate instead of C/DLL output.
See `native_source_workshop_preview.md`. Runtime registry publication is being
migrated separately. The data-only candidate prepares complete Mario source with
actual copied assets and typed parameters; see `native_source_package_candidate.md`
and `native_source_asset_records.md`. Preparation and format/editor fixtures are
not live-game acceptance.

## The current dependency chain

| Stage | Actual route and constraint |
| --- | --- |
| Editable project | `nativeFighterProject_Open` reads DAT assets and optional `module.dll`, `behavior.fms`, parameters and CPU text. The manifest carries base/root/action/demo facts; package member metadata currently retains keys rather than that full asset schema. See `src/melee/ft/native_fighter_project.c:525` and its header. |
| Behavior build | Workshop `nativeShellWorkshopEditor_BuildScript` saves parameters, combines source, then runs `native_content_tool.exe script-build`. The tool emits C and invokes Clang to create `module.dll.new`, then replaces the module/source pair. See `src/melee/ui/native_shell_workshop_editor_script.cpp:340`, `tools/native_content_tool.c:1640,1716`. |
| Package build | `nativeFighterProject_Build` / family Build supply seven chunks: fighter DAT, animation DAT, costume DAT, DLL, source, parameters and CPU profile. `native_fighter_package.c:82` rejects source/parameters when DLL size is zero and requires DLLs for paired members. Both builder and parser enforce it. Removing the compiler command alone therefore cannot produce an ordinary source package. |
| Asset preparation | `nativeFighterRuntime.prepare_member` opens fighter DAT, identifies a compiled-in donor Definition by root, optionally loads the cached DLL, and selects the DLL/requested/compiled-in `code_key` Definition. It validates native callback-bearing records, imports character/animation/costume arenas, resolves article assets/graphs and copies the entire native MotionState table. See `native_fighter_runtime.c:625–1079`. |
| Staging and activation | `StageCached` prepares separate slots/families. `bind_member` mutates engine asset/callback/CPU tables; `ActivateStaged` swaps slot arrays only after active families are absent. See `native_fighter_runtime.c:1184,1211,1316`. Neither stage compiles behavior source or installs an interpreter bank. |
| Direct source route | `nativeScriptFighterLaunch_Load` remains a diagnostic source-file launch with common motions only. It builds a bank, prepares an immutable program, checks publication eligibility and publishes it. It does not load ordinary project/package assets. See `native_script_fighter_launch.c`. |
| Program publication | `PrepareProgram` validates a supplied candidate Definition and caller-pinned entity assets without changing the installed program. `CanPublish` rejects active fighter/Item references and non-idle interpreter scratch; `PublishProgram` copies/rebinds the prepared records. Article/Item owner schemas, state/hook signatures and parameters are supported. Effect owners and additional Fighter event contracts remain unsupported. See `native_script_fighter_program.c`. |
| Preview | Workshop builds/cache-validates a candidate before `nativeHome_RequestPreview`. Preview keeps the previous content ID and, if preparation fails before practice loads, restarts the previous preview once. See `native_shell_workshop.cpp:546–706`, `native_home.c:1140,1490`. Preserve this behavior and strengthen preparation; do not discard the working bundle to discover compiler errors. |

The existing staged asset machinery is substantive import/ownership code worth
retaining. Its old behavior selection and callback publication must be changed
at those call sites. Calling the C generator from a new “source loader,” or
manufacturing a DLL-shaped compatibility Definition, would preserve the wrong
dependency chain.

## Connected entity checkpoint

The direct runtime now owns interpreted Item storage, snapshots and actual native
Item scheduler dispatch. Source factories select immutable Article declarations
at compile time, and retain creator/owner/holder identities across native calls.
Workshop holdables use a shared native protocol with interpreted authored hooks.
These are actual runtime components, not DLL-shaped adapters. Their focused
fixtures separately identify controlled asset/model/platform leaves.

This does not yet activate ordinary Workshop packages through the interpreter.
The package loader must prepare resident assets, parameters, motion tables and
all dependent program records together, then publish only after the previous
content owners have retired. Completing this entity suite alone does not prove
full Mario/Rook behavior, effect scheduling or complete rollback serialization.

Holdable billboard textures also need inclusion in that transaction:
`nativeHoldableVisual_Prepare` currently decodes the ordinary Definition's item
array into its per-fighter cache. Prepared source recipes pin PNG bytes but do
not prepare or publish that cache yet. Update the visual preparation call sites
to consume the candidate recipe data directly; do not manufacture a callback
Definition just to reuse the old input shape. Its current 24-visual capacity
also differs from the new entity limits and needs an explicit bounded contract.

## What Mario actually requires

The census uses the current production Source parser, ProgramIndex and event
signature resolver, not a token-name list. It indexes 487 source nodes, 76
callbacks and 48 owned value declarations. This establishes syntax/declaration
and event signatures only: no callback body, package metadata or game acceptance
is inferred.

| Authored content | Count / concrete contract |
| --- | --- |
| Fighter/moves | One independent fighter; four moves; one action section. The actual donor motion layout comes from `ftMarioClone_Definition` and `ftMario/forward.h`: ten extra states starting at `ftCo_MS_Count`, including appeal states and eight ground/air special entries. Copy numeric metadata and flags, never infer a two-states-per-move layout. |
| Values | Six ordinary parameter defaults; six fighter state values; 31 root DAT Attribute imports. Fire owns one additional Bool state and four float imports at authored offsets 0,4,8,16. Root importer already supplies 31; those four article imports remain explicitly unresolved. |
| Cape | `article It_Kind_Mario_Cape states 2`, two animation states and three logic callbacks. Resolve its asset through the actual slot/kind mapping, validate animation IDs against the imported article table, and install entity-owned event/storage lifetime. |
| Fire | `article It_Kind_Mario_Fire states 1`, animation/physics/collision plus seven logic callbacks. Its projectile declaration binds named speed/angle/lifetime parameters, appearance -1, state 0 and auto-lifetime. This needs typed metadata and article import/publication, not an ignored generic Metadata line. |
| Tornado effect | `model_effect TornadoSwirl 0x47c` with one update callback. Validate the gfx recipe, retained creator identity and joint path ownership, then install effect-owned lifetime/dispatch. Current native policy stores an `EF_UpdateFn`. |

Mario's actual immutable donor asset facts are in
`src/melee/ft/kinds/ftMario/ftmario_clone.c`: fighter root `ftDataMario`, costume
joint/matanim roots, 303 action descriptors, 16 demos, 61 costume joints, semantic
bone mapping, effect bank 1, sound-bank/channel data and article mask `0x005`.
Fire maps to asset slot 0. Cape maps to slot 2 with a kind read from decoded
fighter attribute offset 20; validate that resolved kind agrees with the authored
article declaration. These are load-time asset facts, not permission to retain
Mario's native special callbacks or its `source_kind` gameplay fallback.

The following table records the original audit's barriers. The subsequent
connected Item, Effect and Fighter ABI slices now close these Mario categories;
the real candidate prepares all 77 current source bodies. Ordinary publication
and full gameplay comparisons remain separate acceptance gates.

| Mario callback group | Current scheduler category |
| --- | --- |
| 14 fighter callbacks | Names belong to the current zero-argument Void fighter hook/instance classes. This is an individual class match, not complete program acceptance. |
| 34 move phases | Current motion scheduler class, subject to the actual owned layout and duplicate-target checks. |
| 5 callbacks | ABI closure: Bool `cpu_capture_decision`; one-argument `item_pickup_ext`, `item_drop_ext`, `item_pickup`, `item_drop`. |
| 7 callbacks | Missing fighter dispatch: `cpu_ranged_candidates`, `load_special_attrs`, `landing_action`, `item_visible`, `item_invisible`, `knockback_enter`, `knockback_exit`. |
| 16 callbacks | Original entity lifecycle closure: 15 article callbacks and one model-effect update. They now prepare through their separate interpreted owner/scheduler records. |

Body bindings remain another gate. For example, article creation/owned-reference
operations, owner/creator field receivers, effect-joint selection, article bounce
and reflect operations need their actual typed selector/native contracts. An
event signature or an imported Article proves none of those operations. Compile
every body and reject its first unresolved semantic operation normally; keep the
failed candidate unpublished. Do not advertise full Mario acceptance from this
census or the fighter-only transition fixtures.

## Where native authored addresses enter ordinary behavior

`nativeFighterModule_Open` loads the DLL and calls its exported Definition entry
with the base Definition and the host table. The resulting records are not just
eight special entry functions. Runtime subsequently copies/publishes them:

| Record/path | Callback-bearing fields to replace with script identities or shared engine dispatch |
| --- | --- |
| `Definition.motion_states[]` | MotionState has `anim_cb`, `input_cb`, `phys_cb`, `coll_cb`, `cam_cb`. Current `memcpy` preserves them. Copy `anim_id`, `x4_flags` and `x8` motion metadata separately. Any retained camera leaf must be an explicitly audited generic engine operation; never preserve an arbitrary donor/authored address. |
| `NativeFighterDefinition` | 122 callback fields, including lifecycle, landing/throw/item/held policies, movement/attack/shield/capture/dynamics and CPU decisions. Four special categories cover eight entry fields, with a separate `neutral_article` callback. Zeroing only those entries leaves most authored native policy intact. |
| CPU | 27 `on_cpu_*` hooks (full list below), reached by shared planners beyond the special entry paths. `NativeFighterCpuProfile` is separate numeric attack/threshold data and can remain immutable asset data; its presence does not replace script decision hooks. |
| Callback exports | `NativeFighterCallbackExport` has Void/Bool/Int/Float function-pointer alternatives. Replace authored exports with code IDs and audited signatures; instance callback installation must preserve retained source/recipient identity and retirement behavior. |
| Article behavior | `NativeFighterArticleLogic.logic` points to ItemLogicTable: 13 logic callbacks; its states point to ItemStateTable's three phase callbacks. Additional fields `on_created`, `on_out_of_bounds`, `on_held_status`, `on_held_request`, `on_holder_neutral` also carry native addresses. `nativeItemBehavior_Select` installs these pointers into `Item.xB8_itemLogicTable` and `xBC_itemStateContainer`. |
| Projectile/signals | `NativeFighterProjectile.read_values`/`on_launched` and each article signal callback. Projectile values must reference typed immutable parameter IDs, not a generated read-values function. |
| Model effects | `NativeFighterModelEffect.update` becomes native effect policy/`EF_Effect.update`. Current creator validation also compares exact Definition/policy identity; a new package-owned policy must preserve that lifetime relation using stable owner/descriptor identities. |
| Workshop items | NativeItemDefinition contains `on_thrown`, `on_tick`, `on_ground_collision`, `on_hit`; Mario does not declare these, but an ordinary source package cannot retain the DLL path here as its eventual entity fallback. |

The CPU fields are `on_cpu_pending_action`, `on_cpu_initialize`,
`on_cpu_mode_update`, `on_cpu_output`, `on_cpu_prepare`, `on_cpu_drop_command`,
`on_cpu_stick_step`, `on_cpu_stick_limit`, `on_cpu_spacing_update`,
`on_cpu_attack_motion`, `on_cpu_grab_motion`, `on_cpu_attack_decision`,
`on_cpu_capture_decision`, `on_cpu_pummel_special`, `on_cpu_ranged_candidates`,
`on_cpu_ranged_allowed`, `on_cpu_special_candidate`, `on_cpu_projectile_hit`,
`on_cpu_threat`, `on_cpu_charge_decision`, `on_cpu_charge_cancel`,
`on_cpu_travel_decision`, `on_cpu_falling_recovery_allowed`,
`on_cpu_falling_destination_reached`, `on_cpu_available_jump`,
`on_cpu_recovery`, `on_cpu_projectile_defense`.

The census stores the complete 122-field list plus nested record fields. The
ordinary candidate must hold no authored machine address in any of these paths.
Shared engine primitives may remain native. Adjust actual engine dispatch call
sites to execute code IDs with their event descriptors; do not generate a C
wrapper for each source callback, helper, fighter or article. Optional event
absence must follow its audited shared-engine/default policy, not select a stock
fighter by `code_key`. Mutable trail pointer payloads stay rejected until their
own contract exists. Check coverage for every reachable owned motion, including
the donor's appeal slots outside the eight special entries. The current phase
patch table can overlay native callbacks; that does not establish a complete
source-only owned table. An absent authored phase must have an explicit audited
generic/no-operation policy or be rejected, rather than retaining its donor
function through the numeric table copy.

## Concrete next slice: candidate preparation and joint publication

Introduce an inactive `NativeSourcePackageCandidate` that substantively owns
package bytes, character/animation/costume import arenas, copied data-only asset
records and motion metadata, `NativeScriptMoveLayout`, independent compile-bank
buffers, typed parameter supplies and prepared program/scheduler records. All
arrays have declared capacities; reserve them only at menu/preview transitions.
Pin source and asset banks at stable addresses for their complete live lifetime.

Proposed interfaces, to implement directly rather than through legacy loaders:

```c
bool nativeSourcePackage_Prepare(NativeSourcePackageCandidate* candidate,
    const NativeFighterMemberData* member, const NativeSourceAssetFacts* facts,
    const NativeSourceParameterValues* overrides, NativeScriptDiagnostic*);
bool nativeScriptFighter_PrepareProgram(const NativeScriptCompileBank* bank,
    const NativeSourceOwnedDefinition* definition,
    NativeScriptFighterProgram* prepared_program, NativeScriptDiagnostic*);
bool nativeSourcePackage_CanActivate(const NativeSourcePackageCandidate*,
    const NativeSourcePackageCandidate* current, NativeScriptDiagnostic*);
void nativeSourcePackage_Activate(NativeSourcePackageCandidate*,
    const NativeSourcePackageCandidate** accepted);
```

`PrepareProgram` is the pure portion currently inside `SetProgram`: validate
the candidate's own numeric layout/asset facts, reserved storage, complete event
ABI/owner coverage and duplicate scheduler targets; fill caller-owned scheduler
records. It must not query the installed Definition or mutate `programs[]`.
Preparation may run while the old preview actors still use the accepted bundle.
The current “installed Definition first, then SetProgram validation” cycle
cannot preserve the old preview if that final validation fails.

The transaction has the following actual dependencies:

1. Validate canonical package/member/source/asset spans and family mode. For an
   ordinary authored member, source is required. Replace the DLL chunk/fields
   with explicit asset metadata and change Build/Parse/project/editor/tool call
   sites together. No old-format fallback or version bridge is needed. A donor
   may supply initial asset facts once at project import; package load must not
   use its behavior Definition or `FindDefinition(code_key)` as a source fallback.
2. Materialize the candidate's data-only asset facts: roots, descriptor counts,
   bone/omission mappings, audio/effect dependencies, numeric motion records,
   special IDs/flags and article slot/kind mappings. Validate them before graph
   import. Import DAT/animation/costume using existing substantive importers,
   retaining their arena ownership, visibility checks and animation-tag fixups.
3. Build and validate the move layout from that actual owned motion data. Copy
   numeric MotionState fields without donor/authored callback pointers. Prepare
   audited shared engine callbacks and code-ID event maps separately. Unknown
   metadata, event policy or asset fields fail explicitly.
4. Run direct `nativeScriptCompile_Build` into candidate scratch with its complete
   metadata validator and owned layout. The existing parameter provider runs
   after declaration/body/lowering validation and before parameter publication.
   Supply root DAT imports with `nativeScriptMetadata_ImportFighterParameters`;
   validate sidecar overrides against exact indexed owner/name/type and reject
   unknown/missing imports. Never rewrite source into raw attribute references.
5. Article import closure resolves each declared kind through the candidate's
   slot mapping, uses its imported Article's `native_special_attribute_source`
   plus bounded original DAT to read the four Fire scalar fields, and copies
   typed values into article-owner parameter slots. Validate declared state
   counts/animation IDs against imported ItemStateArray. Projectile recipes bind
   those parameter IDs. Graph/pointer recipes require explicit schemas and are
   not inferred from scalar imports.
6. Prepare all supported fighter/entity/effect scheduler records and generic
   dispatch ABI before publication. For the first bounded loader implementation,
   reject entity declarations and unmigrated payload/hooks exactly as today;
   demonstrate the package transaction with the already supported fighter subset.
   Closing schema/entity/ABI gates is required before using *all* Mario source.
   In paired families prepare both members and the form/companion lifecycle
   together, or explicitly reject that mode; current DLL-specific paired checks
   must not be replaced with unconditional acceptance.
7. If any prior step fails, destroy only the candidate and keep the accepted
   bundle, scheduler pointers and running preview intact. After all preparation
   passes, request the normal preview restart; drain old actors, articles,
   effects and synchronous continuations. `CanActivate` confirms this boundary
   before any global table publication. Fighter `HasActors` alone is insufficient
   once entities retain Definition/program references.
8. `Activate` publishes the prepared engine assets, numeric motion table and
   interpreter scheduler as one no-fail transition, before fighter OnLoad.
   It performs no parsing, allocation, signature lookup or late SetProgram check.
   Keep the previous accepted bundle/content ID pinned through successful new
   preview startup. If startup fails, recreate the previous preview using its
   accepted bundle; release an old bank only after all matching source references
   are gone. The existing home preview fallback provides the user flow to retain.

This first implementation slice needs four focused units: package candidate
ownership/validation, data-only asset-fact materialization, pure fighter program
preparation/publication, and the parameter provider. Existing loader call sites
must use them directly. Tests should import actual owned DAT assets, compile a
supported source-only bank with no module/compiler installed, and inject failures
at each stage while comparing accepted pointers/bytes and retained preview ID.
Reject a full Mario candidate with an exact unsupported entity/ABI diagnostic
until those closures exist. This produces a reviewable source-loading capability
without hiding incomplete Mario gameplay behind copied native callbacks.

## Reproducing the census

`tests/native/source_package_audit/run.py` builds a small source-only census with
the production frontend and event signatures. It also extracts callback-field
declarations and current scheduler-name classes from authoritative headers/code.
No legacy FMS parser, DLL loader, external service or game process runs.

```powershell
python tests/native/source_package_audit/run.py `
  --compiler C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe `
  --output A:/FreakMelee-test-archives/source-package-audit-oct07
```

The JSON report includes all callback owner/span/ABI/evidence records and input
hashes. Scheduler categories are source-audited class matches; they are not a
successful `SetProgram` call. The report explicitly leaves body/package/game
acceptance false. Current receipt: 60 fighter, 15 article and one effect callback;
48 value declarations; 31 root plus four article imports. No production edits.
