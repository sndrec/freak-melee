# Boomerang owned-state and model animation proposal

This is an implementation proposal; Boomerang and Bow/Arrow remain unregistered.
The complete stock Boomerang constructor and lifecycle must become authored
before fighter cleanup can own an ordinary `boomerang_handle` variable.

## Bounded state arrays

Source `itlinkboomerang.c:it_802A013C` initializes sixteen position Vec3 values
and sixteen Euler rotation Vec3 values. `it_802A0C34_sub_2` writes them at a
wrapping index; `it_802A0930` reads them at two independent wrapping indices.
The 384-byte pose history required expanding the original 256-byte article state.
The shared array batch is accepted: fighter state remains 256 bytes and article
state now has 512 bytes, with compact validated declaration metadata.

Declarations remain ordinary typed state, with one field per declaration:

```text
state float history_x[16]
state float history_y[16]
state float history_z[16]
state float history_rx[16]
state float history_ry[16]
state float history_rz[16]
state int history_write
state int trail_read[2]
```

Support bool, int and float arrays for both fighter and article state. Counts
are compile-time decimal integers 1..128. Scalar declarations have count 1
and require no indexing; explicit arrays require indexing even at count 1.
Bool elements use one byte; int/float use four bytes aligned to four. Storage
is declaration order with one contiguous region per declaration. Fighter state
remains bounded by its existing 256-byte region; articles use 512 bytes.
No runtime allocation or pointer-bearing mutable state is introduced.

The compiler `ScriptStateField` becomes `uint16_t offset, count`, plus type
and an array bit. Keep the current limit of 64 named declarations. Element
stride follows type; sixteen elements do not require sixteen descriptors.
The shared declaration/index implementation belongs in a new
`native_fighter_script_state.c/h`, with fighter/article callers invoking it
directly. Existing declaration code moves there rather than remaining behind
a forwarding wrapper.

Indexed reads/writes accept ordinary integer expressions and evaluate the
index once. Generated typed helpers use the existing memcpy access semantics;
`(uint32_t)index < count` precedes every access. Out-of-range reads return the
typed zero, and writes do nothing. Literal indices outside the declared range
are rejected by the compiler. This gives safe deterministic behavior for
custom scripts without changing valid stock ring decisions.

Preserve names/count/type/offset in immutable module metadata for inspection:

```c
typedef enum NativeFighterStateType {
    NativeFighterState_Bool, NativeFighterState_Float, NativeFighterState_Int
} NativeFighterStateType;
typedef struct NativeFighterStateField {
    const char* name;
    uint16_t offset, count;
    uint8_t type, stride;
    bool array;
} NativeFighterStateField;
```

`NativeFighterDefinition` gets owned `state_field_count/state_fields` alongside
its existing `state_bytes`. `NativeFighterArticleLogic` gets
`local_state_bytes`, `state_field_count/state_fields`; its existing `state_count`
continues to describe motion states. Metadata is one static descriptor per
declaration. Validate names unique, count nonzero, type/stride coherent,
alignment, non-overlap, and every region inside the owning state bound. Base
fighter inherited state bytes are prepended as today; descriptors for newly
authored fields report their final runtime offsets.

Pre-array 64-bit clang layout audit: Item 5464 bytes; item union 504 bytes;
native article 312 bytes; stock Boomerang state 472 bytes; native article state
starts at Item offset 5012; fighter state is 256 bytes. Increasing article state
to 512 makes the native member 568 bytes, growing the union/Item by 64 bytes.
Root must update the SDK offset/size fingerprint and verify rollback's actual
Item copy extent before enabling this change. No Fighter size growth is needed.

Meaningful tests: mixed-type scalar/array packing with both compiler modes;
last valid indices; negative/oversized dynamic indices; one evaluation of a
side-effecting index; immutable metadata extent/overlap rejection; independent
articles; generated write/read across offset 255; real 16-entry ring wrap.

## Named immutable animation bundles

Boomerang constructor `it_802A013C_LoadAnim` loads model roots at special
attribute offsets 68/72, adds independent AnimBundles at offsets 76/88, then
requests frame zero. Each bundle contains animation, material-animation and
shape-animation pointers. Subsequent trail updates call `HSD_JObjAnimAll`.
Use declarations such as `model_animation trail_first 76` to import relocated
immutable bundles at activation, bounded by eight names like model roots.
Importer/Article metadata must carry resolved relocated descriptors, not runtime
DAT offsets or fighter-specific aliases.

Proposed generic statements and direct underlying operations:

* `item.add_model_animation(model, animation_name)` -> resolve extra model and
  `HSD_JObjAddAnimAll` with the imported immutable bundle.
* `item.request_model_animation(model, frame)` -> `HSD_JObjReqAnimAll`.
* `item.advance_model_animation(model)` -> `HSD_JObjAnimAll`; retain existing
  zero-argument root form with the same model-only semantics.
* `item.remove_model_animation(model)` -> `HSD_JObjRemoveAnimAll`.
* `item.set_model_animation_rate(model, path, rate)` -> actual `lb_8000BA0C`
  subtree animation-rate operation used for the root child and both trails.
* `item.read_joint_matrix_rotation(model, path, x, y, z)` -> real
  `HSD_MtxGetRotation` output; Euler property reads cannot substitute for a
  decomposed world matrix.

These are generic model operations. They must not call a stock Boomerang
constructor, return-state handler, or article-kind switch.

## Other confirmed generic needs

The special attributes contain seventeen numeric fields at offsets 0..64;
the accepted array batch raised the per-article attribute limit from 16 to 32. Lifetime and return
turn duration at offsets 0/4/8 are typed integer attributes; remaining values
are floats. This is independent of array storage.

Reflection `it_802A20E8` clears the original creator's fighter reference even
after current ownership changes. Existing `owner.call` and owner state reads
are intentionally conditioned on current-owner equality, so they cannot express
this callback. Add a creator context that resolves the immutable creator's live
fighter and validates its authored definition before any named function/state
access. It must never interpret another package's fighter state layout. The
normal constructor guarantees original creator/module identity; interaction
clears the immutable creator when that object is removed.

That reflection chooses its direction target from current owner or reflector
according to the shared reflected flag, gets target position, sets life to
half-life, and applies the existing reflection multiplier. Safe fighter target
handles for `item.current_owner`/`item.reflector`, `targets.position` outputs,
and a read-only half-life property make the selection authored. Reflection
decisions and creator cleanup remain in the script.

Existing generic APIs already supply named collision normals, mirrored vectors,
action categories, state-change callbacks, and thrown-item statistics; do not
add Boomerang-specific duplicates. Return homing reads creator position plus
the owned fighter vertical offset, normalizes/clamps angles and turning duration
in script. Catch calls the authored creator's ground/air catch entry, then
attaches at the semantic LeftThumb joint; eligibility reads common action
category and the common attack-state flag. Fighter return-phase input checks
must preserve their original short-circuit priority through ordinary common
input primitives, rather than a stock Link IASA callback.

## Cleanup gate

The stock fighter damage/death callback removes Boomerang, Hookshot, Arrow,
Bow, then Young Link milk. Fully authored Boomerang constructors and ordinary
state handles close only the first dependency. Hookshot linked GObj-chain
release should expose the reusable actual resource traversal also used by
Samus/Sheik; activation still waits for Hookshot's own translation. No API may
read a stock fighter union offset to recover unported references.

## Accepted shared infrastructure

The array checkpoint passed 54 focused checks. Named animation bundles, actual
HSD model operations, imported joint/material/shape graphs and validated creator
context are implemented in the following shared batch; expanded model and
generated-array fixtures pass. Boomerang now has an unregistered authored fighter/article candidate with the
source-only coverage below. Full fighter cleanup and runtime acceptance remain
gated; this proposal registers no article override and claims no Bow cleanup closure.


## Baseline 5 source comparisons

The unregistered candidate is preserved as the test input
`tests/native/link_boomerang_fixture.fms`. It contains both article families and
ordinary fighter state for the six ground/air throw, empty and catch phases.
It has 493 article state bytes, including six sixteen-element float histories
and two independent readers. All stock animation, homing, catch, collision,
reflection, sound, destruction and constructor decisions are authored.
The damage helper explicitly retains the full-resource cleanup gate.

`link_boomerang_fixture.py` includes the complete unchanged original
`itlinkboomerang.c` and `ftlinkspecials.c`, their actual fighter queries and
vector math, the actual generated C, and the real generic model operations.
`fighter_link_boomerang_test.c` passes **15,452 comparisons with zero failures**
against `.local/baseline5`. Coverage includes both families; history wrap and
expiry; category/hitlag/creator/turn-duration catch boundaries; speed clamps;
all stage-contact masks and facing priority; reflection and destruction across
creator/current-owner/trail combinations; fighter input-angle and smash-window
boundaries; all motion-phase decisions and interrupt priority; creation failure,
created/pickup/held-registration order and launch.

The constructor fixture checks current collision-center and previous-bone
positions, velocity, facing and spawn-collision flags against the stock spawn
payload. It models the approved Host constructor contract; it does not link the
actual `module.c` spawn backend. External allocation, HSD animation and matrix
leaf operations use matched-contract stubs. Generic lifetime/definition creator
validation is covered separately by the real article-creation test. These are
source comparisons, not owned-package activation or gameplay acceptance.

The strengthened catch matrix initially found 174 mismatches. The fixture's
item-removal stub omitted the synchronous `logic->destroyed` call made by
`Item_8026A8EC` before freeing the item (`item.c`, `func_8026A8EC_inline2`).
Calling each actual destroyed body at that boundary resolved every mismatch;
no candidate or backend change was needed. Stock's subsequent trail release
finds already-cleared pointers. The authored immediate removal safely ends its
callback after the destruction notification.

Stock launch writes only XYZ into an uninitialized `Quaternion`. The candidate
initializes W to zero and these fixtures compare only meaningful Euler XYZ.
No stock source was changed. Quaternion-joint launch or undefined-W comparisons
are outside this evidence; actual owned model flags must be checked before
runtime acceptance. The detailed scope, frozen-tool/source/fixture hashes and
initial failure are in
`.local/link-script-port/boomerang-baseline5-direct-results.json`.

## Remaining behavior map and proposed next batch

| Behavior | Stock source / motion | Current boundary |
| --- | --- | --- |
| Spin Attack | `ftlinkspecialhi.c`,356/357 | Accepted authored template and full-owned runtime matrix. |
| Bomb pull and Bomb article | `ftlinkspeciallw.c`,358/359; `itlinkbomb.c` | Accepted authored template and runtime matrix. |
| Bow start/charge/end and Arrow | `ftlinkspecialn.c`,344..349; `itlinkbow.c`, `itlinkarrow.c` | Unregistered owned scratch; source checks pass, complete fighter cleanup and recipient Bow cleanup remain gated. |
| Boomerang throw/empty/catch | `ftlinkspecials.c`,350..355; `itlinkboomerang.c` | Source candidate and 15,452 comparisons; no owned import or runtime acceptance. |
| Hookshot, grounded grabs and airborne catch | `itlinkhookshot.c` states0..8; `ftCo_AirCatch.c`,360/361 and grab phases212/214 | Unported linked model/collision resources and fighter callbacks. |
| Young Link milk taunt | `ftclinkappeals.c`,342/343; `itclinkmilk.c` | Unported; ordinary named `milk_handle` must replace native reference. |
| Sword forward-smash follow-up | Common `ftCo_AttackS42` callbacks,341 | Inherited native common behavior; no authored acceptance from this batch. |
| Fighter load/death/item/knockback presentation | `ftlink.c` / `ftclink.c` lifecycle functions | Independent asset import exists; residual presentation and multi-resource callbacks require a separate source audit. |

The exact damage cleanup order remains Boomerang, Hookshot, Arrow, Bow, then
Young Link milk (`ftLk_800EAF58`). Each constructor must own its declared handle
before the handler can be authored completely. Death slot3 removes Boomerang;
neutral-special and milk phases register full cleanup in damage/death slot2.
There is no native-union reference recovery, incomplete active override or
stock cleanup delegate in the candidate.

The next generic proposals are **not implemented**:

1. Recipient-owned named exported events. Bow destruction calls the current
   owner's `UnsetFv14` even if the original creator is gone or ownership changed.
   A bounded immutable name-hash/zero-argument-function export table on each
   recipient definition and `targets.call(handle, "release_bow")` would dispatch
   that recipient's own function and module state base. A dead target or missing
   export returns false. The source article cannot run its own function against
   a recipient's unrelated state layout. This closes a distinct contract from
   `creator.call`; it requires an explicit missing-handler policy before use.
2. A joint-only visibility channel, for example
   `item.joint(model, "path").self_visible`, using HSD's single-joint Set/ClearFlags.
   Milk toggles only the primary model's first child's hidden flag; existing
   `.visible` changes the selected subtree, not the selected joint's own
   following siblings, and must keep that contract. Milk creation uses owner origin, plane on, initial
   collision off and held off, then pickup selects facing animation. Its removal
   clears creator/current owner before removal; its fighter handle is cleared
   separately in source order.
3. Generic owned linked model/collision resources. Hookshot constructs pooled
   GObjs/ItemLinks with per-link position, velocity and CollData, two alternating
   immutable model roots and a final endpoint root at DAT offsets 84/88/92.
   Script owns scaled lengths, count, motion and collisions. Creation and
   failure cleanup are explicit transitions; regular updates allocate nothing.
   Release traverses the saved next link before freeing each GObj, exactly as
   `it_802A2B10`; the script clears its fighter handle/accessory2/death1/accessory3
   registrations first, then removes the article. Constructor translation must
   precede activation; a raw stock-chain-union release API is not acceptable.
4. A generic external model-joint attachment contract needs a separate bounds
   review. Hookshot's constructor writes its endpoint to `fighter->parts[139]`,
   and AirCatch later uses the semantic ThrowN endpoint. The fighter pool reserves
   `MAX_FT_PARTS = 140`, whereas the authored physical skeleton/runtime rig counts
   are75/76 and79/80. This is an external endpoint, not an extra costume bone;
   physical skeleton metadata must not be inflated to hide it.

## Recipient callback proposal (baseline6, unimplemented)

The concrete [recipient callback exports proposal](recipient_callback_exports_proposal.md)
defines zero-argument exports, recipient module binding, missing-handler policy
and generic caller lifetime guards across helpers/signals and engine dispatch.
It also defines Milk `self_visible` as a selected-joint operation. This is a
source-only proposal; the 768 excluded Bow cleanup configurations and complete
fighter cleanup activation gate remain pending.

## Milk article source batch (unregistered)

The [Milk translation ledger](link_clink_milk_translation.md) records the bounded
selected-joint visibility and article callback batch. The appeal and full fighter
cleanup gate remain pending; Milk passes 69,180 original-C/generated comparisons
with zero failures and the standalone real-HSD visibility graph checks pass.
