# Recipient callback exports and synchronous callback lifetime

Status: source-backed proposal only, against frozen baseline 6 (`e904cea53`).
No shared SDK changes, active Link override, game launch or new acceptance claim
are part of this document. Bow/Arrow and Boomerang remain unregistered until the
remaining fighter cleanup and article lifecycle requirements pass comparisons.

## Required source behavior

`itLinkBow_Logic100_Destroyed` in `itlinkbow.c` calls
`ftLk_SpecialN_UnsetFv14(item->owner)` for Link and Young Link Bow kinds whenever
current owner is non-NULL. It does not require the constructor creator to exist
or equal current owner. `itLinkbow_UnkMotion5_Anim` invokes this cleanup in states
2/5 at frame 0 or frame >= 24 before its creator/current-owner mismatch checks.
`ftLk_SpecialN_UnsetFv14` clears the recipient's own Bow reference if non-NULL;
it does not compare that reference with the notifying article. Its ignored
Boomerang-exists query has no state effect. This is a recipient-owned state
operation; using the notifying article's fighter offsets against that recipient
would be incorrect.

The constructor assigns `linkbow.x4` once, and Bow interaction only clears an
owner matching a destroyed object. No normal transfer path was established in
the accepted source batch. Generic attachment/reflection can change current
owner, but Bow eligibility for those paths is still unproven. Therefore the
absent-creator and transferred-owner fixtures are synthetic contract coverage,
not evidence that these situations occur in normal Bow gameplay.

The current Bow fixture accepts 8,350 stock-C/generated comparisons. It excludes
768 terminal cleanup configurations. An earlier run found 192 actual failures
with absent creator/current owner present; the 768 excluded cases must not be
reported as 768 proven failures. Transferred-recipient fixture data currently
starts with its Bow reference NULL, so the follow-up must initialize a separate
non-NULL recipient reference and compare recipient state explicitly.

## Declaration and dispatch

Use the existing function declaration with an explicit export modifier:

```text
state int bow_handle

export func release_bow():
    state.bow_handle = -1

func private_cleanup():
    release_bow()
```

`export func` has zero arguments and no authored return value. Ordinary `func`
stays private and supports its existing typed arguments. Exported functions are
ordinary readable functions: direct calls and callback bindings point to the
same generated `script_fn_release_bow`; no adapter or forwarding callback is
introduced. An export consumes one of the existing 64 function declarations,
not a second function slot. Function names use existing identifier rules and a
maximum 31 bytes. Exports are fighter functions, not article-state functions.

The notifying article uses:

```text
on destroyed:
    targets.call(item.current_owner, "release_bow")
```

The same operation is available in fighter callbacks and helpers. Its bool
result means an export was invoked. It remains true if the recipient removes
itself while handling the event. Dead/zero target, absent active authored
recipient, missing export or dispatch-depth exhaustion returns false. No stock
handler fallback or caller-layout write occurs. Both same-package and
foreign-package recipients require explicit exports and use the same lookup.
`creator.call` and `owner.call` retain their private same-module semantics.
Article references/signals retain their separate immutable-author/kind policy;
public fighter exports do not grant foreign access to article state.

The event name must be a quoted compile-time identifier. The compiler hashes it
with the existing FNV1a32 name hash. Handle expressions must preserve integer
bits, including IDs above 2^24 and high-bit IDs stored in int32 state. They must
not pass through the current float-only `let` declaration. Target 0 is invalid.
Initially accept integer state fields/array reads, integer typed function
arguments, and the existing target-handle queries such as item.current_owner,
item.reflector and item.contact_fighter. Emit those directly, with an outer
uint32_t conversion which preserves their bits. Accept checked integer literal
tokens using exact C integer constants, bypassing the ordinary float literal
emitter. Reject float fields/locals and general arithmetic in the handle argument
until shared expression type propagation proves an integer result. Casting an
already-rounded float back to uint32_t would not satisfy this contract.
Article ID rules are distinct: only UINT32_MAX/-1 is reserved, not all negative
int32 values.

The initial expression surface supports a standalone call, a whole bool
assignment RHS, and a complete `if`/`elif` predicate with optional `not`.
Reject calls nested in arithmetic, other function arguments, mixed `and`/`or`
expressions or expression-valued returns until ordered expression preludes can
carry lifetime checks. This restriction concerns expression sequencing only;
calls inside ordinary helpers remain supported.

## Immutable definition metadata

Proposed exact additions to the shared runtime header:

```c
enum { NATIVE_FIGHTER_CALLBACK_EXPORT_LIMIT = 64 };
typedef struct NativeFighterCallbackExport {
    uint32_t name_hash;
    HSD_GObjEvent callback;
} NativeFighterCallbackExport;

// NativeFighterDefinition, adjacent to other callback metadata:
uint16_t callback_export_count;
const NativeFighterCallbackExport* callback_exports;
```

Generated tables are `static const`, sorted by ascending nonzero hash, and hold
direct generated function pointers. Reject duplicate names, hash collisions,
zero hashes, arguments on exports and declarations exceeding the existing
function limit. Module validation checks count<=64, pointer/count consistency,
non-NULL callbacks and strictly increasing hashes. Manual authored Definitions
can opt in with the same validated table; stock Definitions expose none.
Binary search is bounded and requires no runtime allocation.

After `generated = *base`, generated Open explicitly replaces both export
fields with its own table/count or NULL/0. Base exports cannot be accidentally
inherited. Required Host checks include functions reachable only via exports,
even when no local callback calls them. Update the ABI fingerprint for the new
Definition fields and Host slots; no versioned compatibility metadata is needed.

## Recipient module binding

Generated callbacks already use their own DLL's `engine`, `script_state_base`
and immutable parameter storage. The dispatch backend resolves the live
recipient with `ftLib_8008741C(target)`, reads its kind while live, obtains the
**active** Definition with `nativeFighterRuntime_Definition(kind)`, finds the
export there and invokes that direct pointer with the recipient GObj. It never
calls ModuleOpen, rebinds a DLL, changes a parameter block or uses the notifying
article's author Definition. `CharacterDefinition` can select staged assets and
must not substitute for active runtime lookup.

`nativeFighterModule_OpenCached` selects `<store>/modules/<complete-package-id>.dll`.
`prepare_slot` derives its base from that package's fighter data. Different
packages therefore load distinct images/storage, even if they export the same
name. Multiple slots using one complete package share the same fixed module
binding. The same-path generated DLL must not be reopened with a conflicting
base/state offset or parameter block while an older active Definition still
uses it. Repeated Open before gameplay is valid; live rebinding is not. This is
an existing global-storage constraint, not something dispatch should attempt to
repair by sending caller offsets across the boundary. Loader validation should
reject any active binding conflict rather than silently retarget existing code.

Export table/function-pointer lifetime is the recipient module lifetime. Modules
are unloaded after scene fighters/articles are torn down. Dispatch retains no
callback or Definition pointer beyond the synchronous call and never accesses
the recipient after invoking it. A recipient may remove itself. Gameplay code
must not unload an active module from within a callback.

## Generic caller-lifetime contract

A target-only check is insufficient. A recipient can remove the caller; returning
from a nested helper does not stop an outer callback using cached Fighter/Item
state. Existing synchronous signals have the same hazard. Do not impose a
Bow-only or non-destructive-export restriction to hide it.

The preferred two Host additions are:

```c
bool (*target_call)(uint32_t target, uint32_t name_hash);
bool (*callback_source_alive)(HSD_GObj* source, uint32_t generation,
                              uint32_t source_class);
```

The source class is a bounded internal enum, Fighter or Item. Backend functions
can live in the existing target and article/lifetime components; names are
`nativeFighterTarget_Call` and `nativeCallbackSource_Alive`. The latter scans the
appropriate live GObj list by pointer **before** reading user_data, then compares
Fighter.x8_spawnNum or Item.x1C as complete 32-bit values. It must reject a removed
object or a new object at the same recycled address with a different generation.
No script pointer, lifetime token allocation or raw state offset is exposed.

This supersedes the earlier one-Host caller_alive-out-parameter sketch: a single
out parameter only protects the immediate dispatch site, whereas existing
signals and all helper callers need the same independent identity check.

At every affected generated callback/helper entry, snapshot source generation
while it is known live. After a potentially invalidating synchronous call,
query the snapshot before any cached source access. Lower call results into a
stack temporary first, perform the guard, then write source state or branch:

```c
const uint32_t callback_generation = item->x1C;
/* ... */
const bool invoked = engine->target_call(target_id, name_hash);
if (!engine->callback_source_alive(gobj, callback_generation,
                                   NativeCallbackSource_Item)) return false;
/* Only now is an authored item bool assignment or branch safe. */
```

Void functions/callbacks return immediately; item predicates return false,
because the entity has already been removed. That false return is safe only
with the engine dispatch checks below; it must not request a second deletion.
Generated statements must evaluate source-dependent arguments before dispatch
and never access cached source afterward until the guard succeeds.

Extend compiler effect metadata with `may_invalidate_source`. Mark dynamic
recipient exports, article/linked/local signals, creator/owner calls and item
removal/owned article removal as potentially invalidating. Propagate this effect
through the existing function call DAG and article signal call graph. Emit guards
at **each** returning call boundary, not only inside the deepest helper. Thus a
signal -> creator helper -> local helper -> removal path stops every generated
outer frame. Function signatures and direct callback pointers stay unchanged.
Callbacks/functions without such calls need no extra registry scans. Effects
must include callbacks synchronously invoked by a backend operation (for example
state-change pickup/launch notifications), not merely explicit authored calls.
Audit these operations as a complete bounded list before accepting the batch.

Existing local article signal parsing excludes direct self-calls but does not
prove all multi-signal cycles impossible; reject local cycles during effect
analysis. Dynamic exports/linked signals also need a common synchronous dispatch
depth limit, proposed 64, checked before invoking a callback. A rejected dispatch
returns false and leaves the live caller untouched. The depth counter is an
ordinary balanced transient call-stack counter, not gameplay state or allocation.

## Confirmed engine dispatch sites

Generated guards do not protect an engine dispatcher after the callback returns.
Check the same GObj plus generation directly at each dispatch site before
cached data accesses, destruction or the next callback. Keep actual callbacks
and call order; do not add forwarding wrappers. The following are confirmed
sites in `src/melee/it/item.c`:

| Site | Current continuation requiring a live check |
| --- | --- |
| `Item_80269528`, animated at line 1423 | Cached item destroy_type/removal when true; timers and helper calls when false. |
| `Item_802697D4`, physics_updated at line 1499 | Velocity, position, collision and blast-zone processing. |
| `Item_80269978`, collided at line 1542 | Cached destroy_type/removal or model transform and bounds handling. |
| `Item_80269A9C`, on_accessory at line 1560 | Re-reads gobj->user_data and updates camera/model state. Re-reading a freed GObj is unsafe. |
| `Item_80269B60`, jumped_on at line 1580 | Cached destruction or further grab/hit processing. |
| `Item_80269BE4`, grab_dealt at line 1599 | Reads cached grab_victim and invokes grabbed_for_victim. |
| `Item_80269DC8`, shield_bounced/hit_shield at lines 1691/1700 | Cached destroy_type and removal. |
| `Item_80269F14`, reflected at line 1740 | Cached destruction and owner/reflector updates. |
| Hitlag entry/exit paths at lines 1801/1828/1888 | Cached item flags and attack-victim bookkeeping. |
| `processCallback` at line 1841 | Cached destruction/removal on true; its caller must stop processing if the entity vanished regardless of predicate result. |
| `Item_8026AB54`, picked_up at line 2137 | Item_8026B074(item_data). Constructor already checks after on_created, but pickup itself still retains data. |
| `Item_8026ABD8`/`AC74`/`AD20`, dropped/thrown at lines 2149/2164/2181 | Attachment, sound and collision helper calls. |
| `Item_8026ADC0`, entered_air at line 2190 | Cached xDC8 flag assignment. |

For predicate dispatch, first save the result, then check identity. If vanished,
return the dispatch site's existing handled/stop result; do not write destroy_type
or remove again. If still alive, retain existing result processing. Propagate
that handled result through outer hit/damage processing so it does not retain an
Item pointer after a deeper callback deletes it.

`Item_8026A8EC` invokes destroyed at line 2071 and continues cleanup with the Item.
Literal self-removal is currently rejected in destroyed/created/pickup/spawn
notifications, but transitive removal through a helper or foreign event bypasses
that check. The generic removal path needs an explicit removal-in-progress
policy: add a generic Item removal-in-progress flag, initialized false on
creation and set before the first destruction callbacks, and reject repeat
removal of that same live entity before calling destroyed
again, so recursive destruction cannot double-free. Preserve cleanup of other
entities in source order. If a notification removes another entity which later
removes this one, the same rule applies. Do not silently defer removal and change
source call order. Test the removal guard and scene teardown separately; generation
checks alone do not prevent reentrant removal before the first call returns.

Fighter direct callbacks also have retained state: `fighter.c` anim at line 1781,
input at line 2243, collision at line 2373 and accessory at lines 2415/2421/2426/2442; `fighter_physics.c`
phys at line 26/hitlag callbacks; `ftcommon.c` damage at line 232. The animation continuation calls
common helpers and physics immediately integrates cached fp. Apply the same
live-generation check before those continuations. Nested throw/capture callback
sites and caller continuations must be included in the dispatch-site audit.
No fighter removal primitive is added by this proposal, but a generic callback
boundary must not assume foreign/native exports can never delete a fighter.

The existing backends `signal_owned_article` and `nativeArticle_SignalLink`
already avoid recipient dereference after their signal callback. Generated
`articles.signal_owned`, `item.signal_link`, local `item.signal`, `creator.call`
and `owner.call` currently emit bare synchronous calls. Include these existing
operations in effect/guard coverage so the repair is reusable.

## Milk: selected-joint visibility

Add `item.joint(model, "path").self_visible` through existing joint-value Host
operations, proposed channel 15 after quaternion channels 11..14. The getter reads
only the selected joint's local JOBJ_HIDDEN flag, false when missing. The setter
calls single-joint `HSD_JObjClearFlags`/`HSD_JObjSetFlags`, changing only the
selected joint's hidden flag. Descendant, sibling and ancestor hidden flags are
unchanged; stock matrix-dirty propagation inside those HSD helpers is preserved.
No new Host slot is required. Existing `.visible` continues calling
SetFlagsAll/ClearFlagsAll on the selected subtree (stock recursion stops at
JOBJ_INSTANCE). It does not visit the selected joint's own following siblings;
its getter already reports the selected joint's local flag. Neither property reports effective rendering through hidden ancestors.

`itClinkmilk_UnkMotion1_Anim` assigns both variables named child/grandchild from
`HSD_JObjGetChild(root)`: both refer to the same first child. It toggles that one
joint before creator checks, then changes that child's scale. Translate that
exact operation, not an inferred grandchild or subtree hide. Its creator/current
owner checks and owner clearing remain explicit ordinary script decisions.

## Acceptance and ownership

Before activating Bow, add generated two-module tests with the same export name,
different base state sizes/alignment and parameters. Verify recipient offsets,
caller/creator preservation, same-package dispatch and foreign opt-in dispatch.
Test unknown names, NULL table, invalid/dead/zero IDs, high-bit IDs, recycled
addresses, recipient self-removal, caller removal inside nested helpers, signal
paths and no post-removal state write. Reject malformed tables and live module
binding conflicts. Include required-Host rejection and absence of inherited
base exports. Direct engine tests must exercise both predicate results when the
callback deletes its caller, accessory/pickup continuations and reentrant destroy.

Restore all 768 synthetic Bow terminal configurations with non-NULL alternate
recipient Bow references and explicit missing-handler assertions. Include frame 0,
24 and greater than 24 for states 2/5, current owner NULL, creator absent, creator
same/current owner and creator different/current owner. A missing foreign export
is an explicit unsupported recipient branch, not a passing cleanup comparison.
Only recipients included in the authored acceptance scope claim source parity.
The original constructor/fighter cleanup activation gate still applies afterward.

For self_visible, compare actual HSD single-joint operations on a parent, selected
child, descendant and sibling graph, retaining existing All-operation tests.
Test missing paths/models, hidden ancestors and non-hidden flag preservation;
then compare actual Milk callbacks in source order.

Root owns shared runtime/module headers, Host binding, validation/fingerprint,
engine dispatch/removal changes and CMake. Compiler ownership should be coordinated
with active agents; use a dedicated callback/export-effect component and direct
integration with existing function/signal emitters. Link owns recipient/Bow/Milk
source fixtures and translation ledgers. This proposal does not allocate gameplay
state, expose native unions, delegate authored decisions to stock fighter handlers,
or mutate any frozen baseline.
