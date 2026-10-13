# Owned source pose-graph schema audit

This audit predates the implementation. See the
[direct pose checkpoint](native_source_pose_checkpoint.md) for the landed APIs,
verification and remaining gameplay gates.

Read-only audit, 2026-10-08. Production, templates and user projects unchanged;
no game/build run. The existing complete Sheik preflight rejects
`examples/sheik_full.fms:60:1` as unsupported metadata. Its receipt is
`A:/FreakMelee-test-archives/source-form-template-feedback-preflight-root/receipt.json`.

## Current boundary and owned facts

`native_script_source.c` preserves `pose_graph` as a Metadata node; syntax is
already retained. `nativeScriptCompile_MinimalMetadata` rejects it, and the
package's `metadata` callback currently delegates root metadata through the
Item/Move validators. There are no direct `pose.*` bindings, `Pose` constants or
pose-selector types in the current binding/constant/IR units.

**The current runtime resolves graph indices, not hashes.**
`nativeFighterRuntime_PoseGraph(object,index)` in `native_fighter_runtime.c`
indexes `package.assets.facts.poses[index]`, checks its imported-slot mask and
calls `nativeFighterPose_SelectGraph`. The legacy `tools/native_fighter_script_pose.c`
parser sorted declaration hashes and bound each source name to that numeric
metadata index. Its generated-C Host route is reference evidence only.

Read-only inspection of the actual private owned FMA at
`A:/FreakMelee-test-archives/source-family-project-cli-root/drafts/stock-0/partner/assets.fma`
found these exact rows, with `joint_graph_mask=0x30`:

| Source declaration | FNV-1a name hash | Owned row | Slot | Path |
|---|---:|---:|---:|---|
| ChainGroundRest | 305010971 | 0 | 4 | `0` |
| ChainAirRest | 1363947600 | 1 | 5 | `0` |

The hash algorithm is the 32-bit FNV-1a loop in
`tools/native_fighter_script_signal.c`. The inspected record SHA256 is
`671827634766039e3e3ff2ced95d305b38d907002a5c727d7bcc95e3a718ce12`;
`A:/FreakMelee-test-archives/source-pose-schema-audit/owned-pose-facts.json`
records this bounded record inspection. This is not a fresh DAT import.

`NativeSourcePoseGraph` already stores hash/slot/path in `native_source_asset_facts.h`.
`nativeSourceAssets_Validate` rejects zero or duplicate hashes, bad paths
and slots outside the declared graph mask. `nativeSourceAssets_Import` resolves
every row through the actual imported graph before accepting the candidate.
Facts validation does **not** require hash-sorted rows; a new resolver must bind
the actual facts index, independent of declaration order.

## Smallest coherent metadata and selector addition

Add a dedicated, bounded pose-metadata pass composed by the actual package
metadata validator. Keep `pose_graph Name slot N path "child/path"` root-only,
non-executable, before authored code, with at most 16 declarations. Parse the
entire line, reject trailing tokens/duplicate or colliding names, and use
`nativeFighterPose_PathValid`: empty root path or up to 12 child ordinals,
0..255, at most three digits each. Slots are 0..15.

Validate each declaration against **one exact owned facts tuple**
`{name_hash, slot, path}` and its graph-mask bit; do not amend the imported layout
from source or search a Definition. Cache the owned row index in the candidate's
immutable compiler metadata. Unreferenced valid facts rows may remain imported;
each used source name must be explicitly declared. The FMA carries hashes,
not original name strings, so validation cannot recover or prove the spelling
of an undeclared asset name beyond that existing hashed-identity contract.

Resolve bare `ChainGroundRest`/`ChainAirRest` as a program-bound, typed pose-graph
selector. Resolve `Pose.live`/`Pose.blend` as typed destinations (native values
0/1). These should be closed selectors, not numeric aliases, native pointers or
receiver Objects: reject raw/computed graph integers, foreign selectors and
cross-owner exports. Native argument marshalling needs an explicit selector
case rather than its current numeric-only default. The private VM value can
carry the prepared row index; the active source bank/facts relationship supplies
ownership. No hashing, name lookup or graph allocation belongs in ticks.

## Concrete direct compiler/VM composition

Proposed additions, not existing APIs:

1. Extend `NativeScriptCompileRequest` with `const NativeSourcePoseGraph* pose_graphs`,
   `uint32_t pose_graph_count` and `uint16_t pose_graph_mask`, supplied only from
   `package.assets.facts`. Forward-declare the facts row type in the compiler
   header; do not introduce a Definition dependency. A compile with pose
   declarations and no owned pose inputs must fail explicitly.
2. Add a fixed16 candidate-bank pose-symbol array containing source name span,
   declaration span and owned facts row index. After `Source_Parse`, prepare the
   table and exact tuples before body compilation. The package metadata callback
   admits only nodes validated by this pass; all other unknown metadata remains
   rejected. Reject collisions with root helper/value/namespace symbols during
   indexed symbol validation. Failed preparation changes only the separate candidate.
3. Extend the private `CompileContext` in `native_script_compile.c` with a pose
   resolver context `{bank, owner, context_mask}`. In its existing `resolve`
   composition, try pose names/destinations before generic constants/catalog,
   alongside the current owned-motion/effect/spawn resolvers. Statement/program
   local/parameter resolution retains its existing precedence. A matched pose
   name in Article/Effect ownership is an error, not an unrelated fallback.
4. A `Pose_TryResolve`-style resolver returns a typed `PoseGraph` literal holding
   the cached row index for a declared root name, and typed `PoseDestination`
   literals for `Pose.live`/`Pose.blend`. `pose` remains a catalog namespace and
   the eight methods are normal native Invoke instructions. No new callback or
   generated-C execution route is needed.
5. Add those two closed nonnumeric IR/value types deliberately. The existing
   expression resolver rejects types above Callable and its Literal flag permits
   only numeric types (`native_script_expression.c:366..390`); neither limit can
   be bypassed by disguising a selector as I32. Admit these bounded selector
   literals explicitly, preserve them through Constant/Move, and allow native
   formal marshalling only by exact selector type. Arithmetic, casts, storage
   declarations and receiver/foreign argument export stay rejected for this slice.
   Ordinary helper parameters in Sheik remain Bool/Int/Float.
6. Native Invoke verifies the active source bank/facts association, graph row
   bound and the prepared selector membership before obtaining the synchronous
   native pointer with existing `nativeFighterRuntime_PoseGraph`. There is no
   tick-time hash/name search. A naked index is never a public graph argument.

The catalog signatures should be exactly the following, all Fighter-context,
input-only and returning Void. `IntegralAny` means descriptor type Invalid with
`numeric_any=true`, **plus** load-time IsIntegral validation and runtime checked
conversion preserving width/sign; it avoids modulo-truncating a huge clip/start
into a valid small index. Runtime clip bounds are `[0,action_count)`; raw starts
are `[0,parts_num)`. Each F32 formal uses the existing typed numeric marshalling.

| Catalog method | Formal argument types |
|---|---|
| `pose.load_animation` | PoseDestination, IntegralAny clip |
| `pose.request_frame` | PoseDestination, F32 frame |
| `pose.evaluate` | PoseDestination |
| `pose.copy_costume` | PoseDestination, IntegralAny raw_start |
| `pose.copy_graph` | PoseDestination, IntegralAny raw_start, PoseGraph |
| `pose.blend_graph` | PoseDestination, IntegralAny raw_start, PoseGraph, F32 a, F32 b |
| `pose.blend_to_live` | IntegralAny raw_start, F32 a, F32 b |
| `pose.copy_to_live` | IntegralAny raw_start |

These calls mutate poses and are not Pure/read-only calls. Conservatively mark
lifecycle-capable operations MayInvalidateSource and retain reached-call barriers;
request-frame has no implicit evaluation. Declare no output arguments. The
existing `describe` callback can continue using catalog argument layouts after
closed-selector support is added; a second registry is unnecessary.

## Required generic operations and preservation

Sheik's `chain_pose` helper at lines377..406 needs this complete family:

| Source operation | Actual shared primitive |
|---|---|
| `pose.load_animation(destination,clip)` | owned `nativeFighterRuntime_PoseClip`, then `ftAnim_8006F4C8` |
| `pose.request_frame(destination,frame)` | `ftAnim_80070710` on the selected skeleton |
| `pose.evaluate(destination)` | `HSD_JObjAnimAll` |
| `pose.copy_costume(destination,start)` | `ftAnim_8006F994`, then `ftAnim_8006FA58` or `ftAnim_8006FB88` |
| `pose.copy_graph(destination,start,Graph)` | `ftAnim_8006FA58` or `ftAnim_8006FB88` |
| `pose.blend_graph(destination,start,Graph,a,b)` | `ftAnim_80070010` or `ftAnim_80070108` |
| `pose.blend_to_live(start,a,b)` | `ftAnim_8006FE9C` |
| `pose.copy_to_live(start)` | `ftAnim_8006FF74` |

Use the substantive existing `nativeFighterPose_Validate` before each operation.
These are Fighter-only; the source decides the sequence. Starts index the raw
runtime parts array and omission traversal, not semantic bone remapping.
`Pose.live` targets `object->hsd_obj`; `Pose.blend` targets
`fighter->x8AC_animSkeleton`. Clips305/308 and start1 are authored Sheik data,
not generic defaults.

The reference `ftSk_SpecialS_80110610` in `ftseakspecials.c` selects slots4/5 and
passes `item->child`, exactly path `0`. That descriptor begins the rest-pose
**forest**, including following siblings consumed by `ftAnim_GetNextJointInTree`;
it is not merely one transform. With nonzero magnitude, source installs and
evaluates the directional clip in the alternate rig, mixes rest and directional
poses when magnitude<1, then blends/copies alternate to live by phase. At zero
magnitude it blends or copies the ground/air rest graph directly into live.
Removing the graph steps changes Chain's neutral/rest pose.

Keep the two weights independently evaluated F32 inputs, finite but unclamped;
never synthesize the second weight or replace a full-weight blend with copying.
Copy/blend paths differ in bone flags, quaternion handling, model scaling and
root adjustment (`ftanim_transition.c`, `ftanim_skeleton.c`, `ftanim.c`).
Loading tracks does not request/evaluate a frame or enter/restart a motion.

## Lifetime and acceptance gates

A new native pose binding must capture/check the actual scene/pool generation,
logical actor status/storage, selected package+bank, imported rig/parts and both
live/alternate heads. Existing `PoseAssets` validates source data, animation bank,
costume and parts selection but is not by itself a logical actor-status barrier.
Validate graph/clip spans, omissions and descriptor/runtime topology with the
existing pose validator; retain native graph/clip pointers only within the reached
synchronous operation. Recheck after animation load, evaluation and copy/blend
callbacks, stopping on retirement/fault/head or costume replacement before any
further native read or VM output publication. Unreached source branches remain
unexecuted. Package leases/exact-bank references already prevent resource release
while active; no per-tick retain/allocation is needed.

Acceptance should cover reordered declarations/facts, tuple mismatch, missing
slot/path/cycle/span, unknown or numeric/foreign selector, wrong owner, raw starts
with omissions, independent finite weights, phase/magnitude branches and actual
shared pose results. Use owned imported graphs plus real source/VM operations,
then callback retirement and live/alternate-head replacement cases. Existing
legacy compiler/runtime fixtures are reference material; they do not establish
this direct schema or complete Sheik acceptance. Metadata alone only moves the
first rejection to the missing `pose.*` family; later template/article/family
preflight gates remain separate.
