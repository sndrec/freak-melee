# Remaining direct interpreter binding closures

Baseline source audit, 2026-10-07, before the connected entity implementation.
Its counts below are historical evidence, not a fresh census of the extended catalog.
Scope: all 33 authored `examples/**/*.fms`
files, with complete Mario and the six-file Rook workshop source as the first
activation targets. No production files or previously shared documents changed.
The package/asset transaction is detailed separately in
`native_source_package_loading_audit.md`; it is a prerequisite here.

The next useful port is an **entity lifecycle and retained-context suite**:
metadata/assets → spawn → storage/parameters → item event dispatch → owner and
creator access → attachment/held behavior → retirement. Existing Item-self calls
are useful primitives, but do not install an article/item scheduler. Port the
suite as a coherent source-executed path before widening to resource collections
and multi-actor capture. Mario then needs effect/reflector and CPU/event closures;
Rook needs holdable recipes plus its remaining common input/collision policies.

The connected entity runtime now includes immutable metadata preparation,
factories, owned Item state/snapshots, actual scheduler dispatch and retained
creator/owner/holder scopes. See the Item ownership, scheduler and holdable
protocol documents for their accepted boundaries. Source-qualified factories
from a recipient scope, effect owners, resources/links and ordinary package
activation remain open. Presence in this catalog is still not full-template
behavioral acceptance.

## What the census establishes

`tests/native/script_remaining_closures/run.py` reuses the production lexer and
the existing balanced postfix census, preserving arguments in chains such as
`item.resource(index).joint(path).translation.x`. It compiles the actual current
catalog macros/table and Descriptor function into an isolated audit executable,
alongside the actual event descriptors. It does not substitute working runtime
implementations or call the generated FMS compiler.

The snapshot has 317 catalog descriptors and 19,504 native occurrences across
33 files, with no unframed executable lines in the structural census. Of these,
10,908 have matching catalog name/context/access/arity shapes and 191 have event
payload property names. Those are **presence only**, not typed expression,
callback ABI, metadata, runtime or whole-template acceptance. The remaining
8,405 uses have missing paths, context/access/arity mismatches, recipient scopes
or selector chains needing semantic contracts. There are 885 distinct candidate
spellings across the closure groups; the JSON keeps individual paths/lines/chains.
Separately it records 184 cross-owned state/parameter accesses, which are not
native operations and must not disappear from the ownership work list.

| Missing candidate group | Uses / distinct spellings | Dependency closure and actual backing |
| --- | --- | --- |
| Article spawn/attach/holdable lifecycle | 784 / 71 | Owned recipes, item creation and scheduler/storage publication; `native_article.c`, `native_item_behavior.c`, `native_holdable.c`, `native_held_article.c`, runtime article asset lookup. |
| Retained receiver/target/capture | 851 / 126 | Owner/creator/holder/other/victim identity, receiver reads/writes and explicit recipient scopes; `native_item_owner.c`, `native_fighter_target.c`, `native_fighter_relations.c`, `native_capture.c`, `native_capture_item.c`. |
| Presentation/graphs/effects | 1067 / 94 | Entity joints/animation graphs, scoped effect policies, visibility and spawn effects; `native_model_effect.c`, `native_model_joint.c`, `native_article_model.c`, `itarticle_data.c`, `ftparts`, `efsync`, `efasync`. Existing fighter bone/animation/particle primitives should be reused. |
| Resources/links/signals | 1628 / 57 | Resource reservation and collection lifetime, indexed graph nodes, borrowed fighter parts, collision/matrix outputs, reciprocal links and reentrant signals; `native_article_resource.c`, `native_article.c`, `native_article_refs.c`. |
| Fighter entry/control policy | 1241 / 130 | Remaining shared input/entry/status/attribute families and old spellings; `ftCommon`, `ftCo_*`, `ftwalkcommon`, `ftaction`, `ftanim`. Kinetics, general state and transition primitives already exist; these counts do not mean those families need rewriting. |
| Item-self remainder | 1107 / 114 | Item fields not yet cataloged plus state/animation, attack/collision and model operations. Current `native_script_binding_item.c` covers common fields/lifetime/gravity/bounce/reflect/sound; the remainder must keep native field widths and actual Item/holdable applicability. |
| Collision/stage geometry | 385 / 42 | Floor/wall/ceiling probes, ledge checks, contact motion/position effects, line IDs and moving surface queries; `ftcollision_*`, `ftcliffcommon`, `mpColl`, `mpLib`, `native_article_resource.c`. Do not reduce effectful collision functions to cached Boolean getters. |
| Combat/guard/capture policy | 391 / 87 | Reflector/absorber volumes, remaining hitbox/hurtbox selection and capture policy; `ftcoll`, `ftCommon`, `lbcollision`, `native_capture*`. Existing combat bindings are the base, not a second combat API. |
| Planner/world/player queries | 375 / 55 | CPU state, candidate queues and shared policy hooks, world/player/stage lookups; `ftCo_Cpu*`, `native_fighter_target`, `player`, stage modules. Return/output and RNG ordering are part of the contract. |
| Event/callback identity | 359 / 40 | Uninstalled event ABIs, code-ID callbacks and old hidden-helper operations; `native_script_events`, `native_script_event_payload`, `native_script_fighter_*`, original callback call sites. Most `event.*` properties already have descriptors but still need engine dispatch. |
| Audio policy | 3 / 1 | Remaining audio policy spelling, beyond current direct sound calls; verify actual channel/bank routing. |
| Math/unclassified | 214 / 68 | Old `random()` spelling, vector helpers and remaining prefixes; individual evidence remains unresolved in the report rather than guessing types or calling this group implemented. |

Counts are snapshot evidence and include overlapping/WIP authored examples. They
measure repeated source uses, not independent API signatures or work estimates.
The current direct resolver's known overloads are accounted for, including
`state.enter`, `physics.fall` and root-bone rotations. A full receiver chain is
never accepted merely because its final leaf exists in the catalog.

## Suite 1: generic entity activation, ownership and callback lifecycle

This suite should contain the following pieces together:

1. Typed `article` and Workshop `item` definitions, state counts/animation IDs,
   projectile/attachment recipes and bundled icon/model assets. Resolve article
   kinds from actual owned slot mappings; copy scalar imports into per-owner
   parameters. Validate all recipes before program publication. Rook's PNG
   paths currently become generated DLL arrays; source packages must carry their
   validated bytes as immutable assets instead of reading mutable files at tick.
2. Fixed-capacity Item-instance binding keyed by actual source generation,
   program identity and source-node owner. Bind parameters/storage before the
   first authored creation callback, at the actual registered Item lifecycle.
   Direct execution already understands Item owner IDs, but the engine does not
   currently schedule them. Include the persistent instance state in rollback.
3. State-phase and logic dispatch with the audited Void/Bool contracts, argument
   payloads, default returns and ordinary state-transition semantics. Replace
   the actual `ItemStateTable` / `ItemLogicTable` authored selections at their
   engine call sites with code-ID dispatch; keep only shared engine primitives
   native. No per-article generated wrappers or stock-kind gameplay fallback.
4. Spawn/create APIs over real owned assets: projectile, at-position/velocity,
   child/held/bone attachment and Workshop recipe index. Publish handles only
   when construction/registration succeeds and the calling source survives.
   The existing substantial static operations in `native_fighter_module.c`
   are audit references; move their generic work into the direct implementation,
   calling actual engine functions. Do not route through NativeFighterHost.
5. Retained **creator**, current damage **owner**, attachment owner and current
   **holder** as separate concepts. Reflection must not rewrite creator policy;
   attachment release must not masquerade as creator retirement. Existing
   `native_item_owner.c` and `nativeArticle_Creator` already carry generation and
   exact Definition checks that the new program identity must preserve.
6. Generic typed receiver field/state/parameter access and callback invocation.
   Script Object values carry identity/generation/kind only. Capture a receiver
   and every selector before evaluating an assignment RHS or an effectful call;
   do not re-read `owner` after reflection or a nested callback. Receiver storage
   must resolve to the receiver's actual owner/program, not the caller's fields.
7. Held/attached operations, manual lifetime, pause/resume, state changes and
   interaction payloads. `nativeHeldArticle_*` already snapshots real held items
   and verifies they remain held after callbacks. Keep those source/target guards
   when replacing native policy function pointers with interpreted events.
8. Destroy/remove ordering, explicit registry retirement, state release and
   reentrant callback safety. A destroyed callback may update a live owner's
   state before storage retires; a dead owner must not be silently replaced with
   zero/default data. After every effectful native operation validate caller,
   captured receiver and retained program identities before publishing outputs.
   Popped/nested payload frames must not expose the outer callback's arguments.

This is the necessary dependency chain for `articles.owned/remove_owned`,
`articles.ref(handle, kind)` and `owner.state.*`, not three unrelated convenience
APIs. Actor and entity retirement must also pin assets/definitions/programs until
all matching Item/effect references and synchronous continuations are gone.

**Source backing:** `nativeItemBehavior_Select` currently chooses a native
article logic table and clears its private state; `native_holdable.c` installs
native `thrown/tick/hit/ground_collision` callbacks; `native_article_refs.c`
requires the expected logic/Definition and validates source/target generations.
`native_fighter_module.c:345` contains generic authored projectile construction,
`:612` bone-attached construction, and `:790` owned removal/pause/resume. Actual
leaves include `Item_80268B18`, `Item_8026AB54`, `Item_8026A8EC`, `it_8026B724` and
`it_8026B73C`. Constructor/attachment policy and source guards belong in the
direct binding implementation, not an adapter around those old static hosts.

**Completion gate:** run a source-owned Workshop item and a source-owned article
through creation → thrown/state phases → interaction/reflect → nested signal or
callback → destruction, using registered native Items and real direct execution.
Verify owner state writes, creator survival after ownership transfer, receiver
retirement during RHS evaluation, output publication and deterministic rollback.
Prove construction failure leaves the caller's published handle/accepted bundle
unchanged. These are lifecycle assertions, not instruction snapshots.

## Mario activation suite

The required missing source paths are concrete:

| Dependency | Mario authored operations / closure |
| --- | --- |
| Entity/attachment suite | `articles.spawn_authored_projectile`, `spawn_bone_attached`, `owned`, `stop_cue`, `pause_owned`, `resume_owned`, `remove_owned`; `item.attachment_active`, `change_state`, `activate_attached`, `interact`. Fire/Cape metadata, four Fire imports, three article states and fifteen callbacks must all be installed. |
| Retained receiver suite | `creator.motion`, `creator.command3`, `creator.floor.normal.x/y`, `owner.motion`, `owner.clear_damage_death` and the non-native `owner.state.cape_article` write. These cannot be flattened into self-property IDs. |
| Effect/graph suite | `effect.joint(path).visible/rotation.z`, `effects.spawn`, `spawn_root`, `wind`, `visuals.hide_group`, `item.spawn_effect_on_bone`. The Tornado update owner is an effect, with its own generation and retained creator. |
| Shared movement policy | `controls.ground_actions`, `controls.air_actions`, `facing.from_input`, `collision.ground_standard`, `air_standard`, `air_special_landing`. Reuse the already ported state, collision boxes and kinetics, but audit the missing predicates' mutations and callback effects. |
| Reflector suite | `reflector.create`, `disable`, `update`, `retain`: typed volume data, contact callback/code identity and state-change retention. Native `ftColl_CreateReflectHit`, `ftColl_8007AEF8` and actual reflector flags are the backing. |
| CPU/event suite | `cpu.grab_attempts`, `level`, `run_script`, `ranged_unsafe`, `add_candidate`; Bool capture-decision and ranged-candidates engine hooks. The existing backing includes `ftCo_CpuRangedAttackUnsafe` and `ftCo_CpuAddAttackCandidate`. |
| Scheduler ABI closure | Four item pickup/drop payload events plus load-special-attrs, landing-action, visible/invisible and knockback events. `event.flag` has a typed payload descriptor already; adding another native property does not install these callbacks. |

`callbacks.reset_root_x_on_damage()` is generated hidden behavior, not a missing
irreducible engine primitive. The old emitter creates a function that rotates
the root X to zero, then installs it in damage/death2 slots. Author that helper
in FMS and use the existing generic callback code-ID registrations; no Mario
native helper API is needed. Preserve overwrite/clearing behavior and retirement
at the actual callbacks, rather than treating registration as additive listeners.

The original hand-written decision sites to remove from ordinary source-owned
selection are `ftMr_Init_OnLoad/OnDeath/LoadSpecialAttrs`, item/knockback hooks in
`ftmario.c`; special motion callbacks in `ftmariospecialn.c`, `ftmariospecials.c`,
`ftmariospecialhi.c`, `ftmariospeciallw.c`; Cape creation/removal/hitlag/reflect
and transition policy (`ftMr_SpecialS_*`); and
`it/kinds/itmariofireball.c` / `itmariocape.c`. Reuse their underlying common
engine work as generic bindings. Do not keep
these functions selected by a donor Definition while reporting Mario migrated.

## Rook activation suite

Rook has a smaller coherent target: nine source-authored Workshop item recipes,
including eight PNG snack billboards; generic holdable spawn/held checks and food
spawn; Item scheduler/storage/rollback; common ledge/input/landing operations;
visibility; and callback code identities. Current Item-self fields, gravity,
sound, and existing root-bone rotations need no second implementation.
The fighter `on landing` hook also needs actual dispatch; Rook resets its crime
counter there, and the current fighter scheduler's accepted hook names omit it.
Workshop `hit` and `ground_collision` require their Bool-return item scheduler
entries, not just the existing Void fighter callback path.

Missing candidate paths are `items.held`, `items.pull`, `item.spawn_food`,
`visuals.hide_group`, `controls.aerial_attack_actions`, `collision.cliff_catch`,
`collision.check_ground_ledge`, and old `state.land`, `state.land_special`,
`random()` and `callbacks.on_hit_state`. Actual backing includes
`nativeHoldable_Spawn`, `nativeHoldable_SpawnStockFood`, `ftCo_800CB870`,
`ftCliffCommon_80081298`, `ftCo_Landing_Enter_Basic` and
`ftCo_LandingFallSpecial_Enter`. Existing typed landing/random primitives can
serve modernized source spellings instead of duplicating APIs.

The Rook source header currently declares `base ftDataRook 341 12`, rather than
the direct compiler's independent fighter directive. Treat this as an explicit
asset/layout assertion in a validated source schema or update the source to the
new independent schema; it must not re-enable native Rook behavior inheritance.
All twelve state fields, including `snack_choice`, are authored Float. The old
`items.pull` host casts the choice to an integer recipe index; preserve that
conversion explicitly at the typed call site (`int(state.snack_choice)`) if the
new generic spawn formal is Int. Do not guess that these fields were Int or
change RNG draw count/rounding while normalizing `random()` to `random.float()`.

`callbacks.on_hit_state(344)` also creates a hidden generated callback. The old
emitter changes to frame one and then performs startup animation. Replace it
with an authored zero-argument helper using `state.enter(344)` and
`animation.start()`, registered through the generic deal-damage callback code ID.
Do not substitute named-special `state.begin`, which starts at frame zero.
The original `rook_side_hit` hand-written function only changes motion; this
distinction must be resolved against the authored source contract, not obscured
by preserving the native fallback. Generic callback registration/storage already
exists and should be extended only where its actual lifecycle requires it.

Hand-written behavior references are `rook_on_load/on_death/on_landing`,
`rook_special_*`, `rook_side_hit`, the `rook_*_anim/iasa/phys/coll` functions and
`rook_motion_states` in `ftrook.c`, plus holdable callback decisions in
`native_holdable.c`. Their per-fighter decisions become authored helpers/events.
Immutable Rook rig/action/motion facts may be retained as assets; native Rook
private state fields must not replace the script's owned state records.

## Next large suites after Mario/Rook

**Effect owner/graph closure:** the current `nativeModelEffect_Spawn` creates a
retained multi-member group and assigns its policy update only to member zero.
Preserve that single update entry and the linked members' exact creator/origin
identity checks. Route the actual effect update to its source-owned code ID;
do not expose `EF_Effect*` as a ScriptValue. Pair this with typed joint/path
selection and writes so the Mario Tornado callback can execute, not just spawn.
Decode literal paths/recipe names at load into bounded immutable descriptors;
do not turn them into raw C offsets or repeatedly parse them during ticks.

**Resource/link/signal closure:** implement indexed resource nodes, graph loading,
external part reservation, model transforms/animation, collision sweeps, matrix
outputs, reciprocal links and signals together. Link/Young Link, Sheik and the
climbers exercise this family heavily. Hand-written references include
`ftLink/ftlinkspecialhi.c`, `it/kinds/itlinkboomerang.c`, the Link hookshot item
implementation, `ftSeak/ftseakspecials.c` and `it/kinds/itseakchain.c`. Port the
shared collection/link operations, not a Link/Sheik native API. Preserve
`resource(index)` and `joint(path)`
as separately captured selectors. Existing resource attachment/line-speed queries
publish initialized outputs after a live guard even when a query fails; target
queries publish outputs only on success. Reuse the VM's distinct output policies,
not one universal query rule. Release borrowers before nodes and invalidate the
whole collection before destructor reentry. Do not add a named fighter-chain API.

**Target/capture/recipient closure:** typed target queries and stable identities,
holder/victim/partner selection, capture/release, scoped recipient writes and
callback payloads. The numeric and Object-return paths must agree on class and
generation. Capture the selected recipient once, validate its program/storage
ownership, and bound synchronous reentry with caller-owned context stacks. An
inner callback must not inherit an outer payload or dynamically change the outer
assignment's receiver. Use the existing target/relations/capture engine units;
do not give source access to native pointers or native struct offsets.

**Remaining shared policy closure:** CPU planner decisions, heavy-held items,
throw/roll/catch/guard/dynamics/shield transitions and broader stage/player
queries. Event descriptors already record many signatures/defaults; actual
caller branches still choose native Definition callbacks. Migrate those branches
in coherent policy groups with exact Bool/Int/Float returns and payloads. Keep
mutable trail pointer events unsupported until a separate typed contract exists.

Every suite must finish direct-source metadata validation, typed expressions,
native operations and actual event installation, then exercise the behavior with
registered engine sources and retirement/rollback. Static catalog entries or
isolated primitive tests do not establish source activation. Unknown families
continue to fail with source diagnostics; no generated C, DLL, legacy host table,
per-fighter wrappers or silent native fallback belongs in the completed path.

## Reproduction and audit boundaries

```powershell
python tests/native/script_remaining_closures/run.py `
  --compiler C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe `
  --output A:/FreakMelee-test-archives/script-remaining-closures-oct07
```

`closures.json` records all occurrences, source spans, preserved chains, context,
access modes, current catalog shape checks, separate cross-owned accesses and
input hashes. The isolated catalog source is an exact current table/body
extraction, not generated FMS or a runtime shim. The report does not infer numeric
types from regex tokens, does not execute native leaves, and does not claim that
the WIP examples are already semantically valid. No game or production edit ran.
