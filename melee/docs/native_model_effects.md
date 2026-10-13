# Authored model effects

The native backend and readable compiler support immutable model-effect
definitions with direct update callbacks, creator ownership and ordinary HSD
joint controls. See the [scripting reference](scripting/model_effects.md) for
declarations, named construction and callback context. Mario/Dr. Mario Tornado
and Luigi Cyclone now supply their own visual update scripts; they read
script-owned collision state instead of the stock private motion union.

## Construction and ownership

`EF_SpawnResult` identifies the actual primary result as a model, particle
generator or absent result. Its model generation is captured when the
constructor returns, before initial animations are drained. Composite children
retain their original behavior. The three dispatchers return this type directly;
there is no old untyped API or pointer-truncation path. Fox's unused effect
pointer history was removed, preserving the actual effect spawn calls.

A fighter definition may declare up to 32 sorted, uniquely hashed
`NativeFighterModelEffect` entries. Each supplies a recipe and an `EF_UpdateFn`.
The engine catalog currently allows 44 confirmed recipes consuming one joint,
returning a primary model and retaining that joint as their attachment. These
are construction facts, not fighter-specific scripting operations. Unsupported
argument layouts and particle recipes are rejected before construction.

Named construction validates descriptor ownership, the creator generation and
the attachment both before and after construction. It finds the returned model
in the live effect registry before dereferencing it, and rejects a recycled
generation. A successful binding replaces the primary model's update directly.
It does not delegate to or chain the original fighter-specific updater.

## Update and cancellation

Bound effects retain the original creator, generation, active definition and
attachment part. Every update validates them before reading an attachment or
model, including while paused. It revalidates after animation, which may remove
or recycle either endpoint. Changing the creator's rig or registration ends
the binding. Registry lookups verify object classification and destructor
identity before interpreting userdata.

Cancellation clears the update and marks the source dead immediately, even
when HSD defers object deletion. Generation-based cleanup cannot remove a
recycled object. Scene cleanup removes bound effects before releasing fighter
definitions. Allocation serial lives in the native rollback state section;
effect fields reside in the existing fixed HSD allocation.

The shared `native_model_joint` component contains the actual ordinal traversal
and HSD channel operations used by article and effect code. It supports
translation, Euler rotation, scale, quaternion components, existence and
self/subtree visibility. Paths, finite values, instance boundaries, quaternion
rules and matrix dirtiness retain the existing article semantics.

## Backend checkpoint, 2026-09-30

The full Release build and 14 focused tests pass. The new effect test invokes
the production registry, ownership, binding and cancellation code with explicit
constructor/free boundary doubles; it does not prove dispatcher rendering or
rollback restoration. Existing pure fighter/item fixtures explicitly model an
absent effect registry and do not claim effect-domain coverage. Joint tests use
the real HSD operations, including depth/ordinal boundaries, visibility,
quaternion behavior and matrix dirtiness.

A hidden, muted, accelerated drawn stock match completes all 520 input samples
and enters Luigi Cyclone motion 357. This is a runtime smoke check, not a visual
comparison or full scripted fighter acceptance. The compiled allocation serial
symbol is verified in `.mstate$m`. Source review classifies all 297 explicit
dispatcher cases and preserves the original constructor, RNG and animation
call order. A compiled dispatcher branch fixture and effect-specific rollback
restore tests remain pending.

Evidence is under `.local/model-effect-foundation-*`,
`.local/model-effect-stock-cyclone-smoke*`,
`.local/shared-model-joint-fixtures/` and
`.local/authored-effect-policy/model_joint_recipe_inventory.json`.
Frozen baseline15 contains the matching runtime, SDK and compiler.

The then-incomplete compiler implementation was preserved separately in
`.local/authored-effect-policy/compiler-candidate/` with hashes and remaining
validation notes. Its production edits were restored before that pause.

## Script and original-source comparison, 2026-10-02

The compiler now extracts bounded declarations, emits sorted named policies,
and specializes actual helper bodies for the effect context. Token-aware
context classification ignores quoted strings, comments and unrelated name
prefixes. Effect-only helpers cannot be exported, installed in fighter slots,
or reached through article owner/creator calls. Unused effect helpers are
validated rather than silently discarded. The callbacks use the effect's
actual GObj/generation for continuation checks; no stock updater is chained.

The cohesive compiler fixture passes 44 declaration/context/emission cases.
Both actual generated module forms pass 78 decision, required-Host, spawn and
cancel-continuation cases, including deferred deletion and a disappearing
creator. These module tests use explicit Host boundary doubles.

The full Mario, Dr. Mario and Luigi templates each pass 34,560 exact
original/generated effect comparisons, totaling 103,680 with zero failures.
The fixture loads their actual generated DLLs and compares complete synthetic
HSD graphs, visibility flags, root rotation and matrix dirtiness across
motions, command3, owned collision state, floor normals, missing children and
instance boundaries. It uses the original unchanged stock updater bodies and
actual HSD traversal bodies alongside production shared joint controls,
creator registration and callback lifetime code. An additional 24 cases reject
invalid registrations, creator generations and attachments. The originals
call `atan2f`; the scripts use the existing generic `atan2_float` operation.

Source evidence: `.local/model-effects-stock-source2/acceptance.json` and
`.local/model-effects-stock-source2.log`. Compiler evidence:
`.local/model-effect-compiler-check/result.json`. The first source-fixture
attempt's missing-header diagnostics remain in
`.local/model-effects-stock-source.log`; that failed attempt is not acceptance.

The source comparison has explicit allocation/tagged-spawn/deferred-free
boundaries and synthetic base motion rows. It does not execute the renderer,
the actual dispatcher constructor branches, effect animation scheduling, or
effect-specific rollback restoration. Those limits remain separate from the
tested authored visibility/rotation decisions and the existing general
rollback checks.

The coordinated `RelWithDebInfo` build and thirteen focused compiler,
module, parameter and callback-lifetime checks pass. Frozen baseline16 passes
three hidden, muted, accelerated drawn stock/clone matches for Mario, Dr. Mario
and Luigi: 1,560 compared controller samples and 3,120 fighter states. These
integration traces supplement the source comparison; they do not measure
rendered joint pixels. Runtime evidence:
`.local/model-effects-runtime16-corrected/result.json`. The initial runner's
`human:Dr. Mario` argument was rejected before gameplay; its failure evidence
remains in `.local/model-effects-runtime16` and is excluded from acceptance.

Rook's authored source is unchanged and its installed package was replaced
once for the expanded host layout; see
`.local/rook-authored-effects-sdk-refresh.json`. Generic multi-jump/shield-break
hooks ship in the same ABI batch, but Jigglypuff's new scripted jump candidate
has separate acceptance in `jigglypuff_script_translation.md`.


## Positioned and linked model-policy candidate, 2026-10-02

The catalog now distinguishes joint and position constructor arguments and
records the complete fixed model count. Named position construction and
indexed linked-model joints share the existing typed dispatch and HSD channel
operations. Each member captures its GObj and full generation immediately at
construction; an authored group is installed only when every required member
and the creator/author/rig still match. Partial construction rolls back newly
created members, deliberately avoiding stock's surviving partial pair.

Group cleanup marks all members before removal, validates generations before
freeing another member, and avoids retained next pointers while removing a
group. Positioned effects retain the original creator-root identity without
requiring the position constructor's NULL attachment to equal a joint. Named
queued position effects use the existing fixed queue allocator and deferred
world-position sampling phase. Saved author/policy memory is read only after
matching the current registered Definition and policy address.

A scoped console run passes 57 compiler/context/emission cases, the existing
single-model lifetime fixture, and the new production-body group fixture.
Evidence is under `.local/donkey-bowser-closure/`. The latter extracts the
actual recipe catalog, deferred queue processor/flush, and effect update body;
allocation, model resource construction and deletion are explicit boundaries.
Its cases include removed/recycled secondary objects, secondary expiry,
partial allocation failure, queued author/creator/rig replacement and both
members' animation phases. Eviction skips a currently running GObj while
HSD deletion would defer; a direct regression verifies selection of the next
eligible object, synchronous deletion during the delayed-proc phase, and
linked-group eviction. Additional cases reject a secondary's altered parent,
creator generation, attachment part, origin joint or attached joint. This does not establish rendered pixels or the
resource constructor bodies themselves.

The coordinated snapshot19 build and twelve focused compiler, module,
parameter and lifetime checks pass. Full-template generated DK/Bowser effect
callbacks match the original unchanged updater bodies in 46,080 comparisons,
with eight registration/lifetime rejection cases. Single-model regressions
pass 103,680 comparisons and 24 rejection cases; Fox/Falco regressions pass
18,432 comparisons and 16 rejection cases. These fixtures use synthetic HSD
graphs and explicit allocation/tagged-constructor/free boundaries. They do not
claim actual resource-constructor coverage or rendered pixels.

Six hidden, muted, accelerated drawn stock/clone pairs pass for repeated
Hand Slap in both directions and grounded/aerial Fortress in both directions:
3,060 controller samples and 6,120 fighter states. These gameplay traces
supplement the source fixtures without measuring effect pixels. The first DK
CLI alias was rejected before gameplay; the corrected source-supported alias
passes, and both attempts remain in the local evidence.

Evidence: `.local/model-group-tests19.log`,
`.local/donkey-bowser-model-source19/acceptance.json`,
`.local/single-model-source19/acceptance.json`,
`.local/fox-falco-model-source19/acceptance.json`, and
`.local/model-group-runtime19/acceptance.json`. Baseline19 freezes matching
runtime, tool, SDK and compiler; baseline18 remains immutable. DK/Bowser still
retain inherited behavior identity pending whole-fighter closure. The owned
effect decisions are accepted separately from that remaining work.

Rook's authored source is unchanged; her installed project package was
replaced once for the new host layout. Receipt:
`.local/rook-model-group-sdk-refresh.json`.
