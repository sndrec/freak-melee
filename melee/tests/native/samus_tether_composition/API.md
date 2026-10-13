# Historical tether contract audit and remaining runtime work

This SDK52-era audit predates the interpreter. Several contracts below now
exist; consult the linked current scripting documentation before implementation.
The constructor/lifecycle checkpoints supersede its preparation status. The accepted
SDK52 fixture deliberately stops at the fighter phase boundary. Preserve the
already accepted shared resource pool: six collections globally, at most90
nodes each, immutable per-article capacity/reservation, exact generation and
Definition validation, and direct model/joint operations. Authored helpers
must make every fighter/article decision; no Samus solver or stock-handler
delegation belongs in a backend.

## Small input and callback reads

`samus_grapple_state_sync` in `it/kinds/itsamusgrapple.c` reads held D-pad Up
bit8, then Down bit4 in sequence. The modified beam's hang path reads physical
Z bit0x40. `input.down_held` already exists; propose RO Bool `input.up_held` and
`input.z_held`. The Z read must not use the analog LR/shield mask. These are
fighter reads; an article can directly invoke its own creator helper to retain
ordinary `tether_code_sequence` state, with existing creator lifetime guards.

The actual cleanup `ftCo_800D9C98` clears `death2_cb`, `take_dmg_2_cb`, then
`take_dmg_cb`, after removal. The interpreter uses generic `callbacks.secondary_damage(None)`. Current
lifecycle.component.fms follows all three native slot clears after grapple
removal; it is composed by samus_grapple_constructor/compose.py. The historical
SDK52 receipts do not cover those edited source bodies.

## Indirect immutable animation components

`itsamusgrapple.c:it_802B743C` and `it_802B7C18` dereference the first entry of
the declared HSD animation pointer arrays. `itCharItems.h` declares all15
components0x74..0xAC as `HSD_AnimJoint**`, `HSD_MatAnimJoint**` or
`HSD_ShapeAnimJoint**`. The existing typed fighter importer
`ftcharacter_assets.c:import_graph_pointer_slot` already implements this
indirection; generic article animation metadata does not.

Extend each immutable named model-animation component with an explicit
source-pointer recipe: aligned attribute offset, direct versus pointer-array
mode, and bounded constant element index. Proposed declaration spelling:

```text
model_animation ChainA joint indirect 116 index 0 material indirect 120 index 0
```

Exact spelling is open to the existing declaration parser. Do not store runtime
DAT offsets in script state, expose pointers, assume contiguous three-component
bundles or fabricate local graphs for missing channels. Resolve/import once
with the owning arena, validate every relocation and index, preserve independent
joint/material/shape omission, and publish only a complete table. The normal
DAT audit records externals such as
`ItmSamusGBeamChainA_TopN_shapeanim_joint`. Current typed native import preserves
these as unresolved/NULL; any generic recipe must state that policy explicitly,
not treat the external-chain word as a local graph pointer. Test direct and
indirect components, null/external channels, malformed arrays, arena failure
and Definition retirement.

## Safe nearest eligible fighter

`it_802B9328` uses `ftCo_800A4A40` from `ftCommon/ftCo_0A01.c`. Its policy scans
the live fighter list, excludes self, rejects candidates outside blast-zone
margins via `inlineD0`, rejects unavailable statuses via `inlineD1`, and chooses
strictly smaller `ftCo_800A1AB4` distance. Equal distances retain the first
candidate. It is not an enemy-only or CPU-current-target query.

Propose `targets.nearest_eligible() -> Int` in fighter context, returning the
existing safe generation handle0 on absence. An article can creator.call an
authored helper which writes that handle; read position through existing safe
target queries. A backend can reuse the actual common search and convert the
result to a live generation handle; it must not expose a Fighter pointer or
move target-selection policy into an article solver. Test self exclusion,
all eligibility gates, exact tie order, no candidate, high-bit IDs and recycled
addresses. Caller CPU margin data is part of the original search contract.

## Complete immutable hitbox descriptor construction

The modified beam path `it_802B9328` calls `it_802B7160`. That body constructs
a complete **fighter** capsule from `it_803B8660`, bound to declared resource
part139. Existing generic setters can enable, clear victims, set groups,
damage and world positions; they cannot initialize the descriptor and bind
its bone. Calling the original Samus function would delegate authored policy.

Use a bounded immutable named fighter hitbox descriptor with ordinary typed
fields and a generic direct constructor, for example
`hitbox.create("BeamContact", int(0), int(139))`. The script supplies the
hitbox index and declared part; the backend validates both and registers the
exact borrowed endpoint for resource teardown. Never accept arbitrary raw
joint pointers or resurrect a retired reservation. Expose descriptor fields
semantically rather than private flag ordinals.

The stock initializer is fully decoded by `lb/types.h`:
ID0, group0, raw bone139, damage0; size1200 times the original0.003906 factor;
zero offsets; angle361; growth100; set-knockback0; item interactiontrue;
ignore-thrownfalse; ignore-scalefalse; clanktrue; reboundfalse; base-knockback0;
element8; shield-damage0; SFX severity1/kind2; groundedtrue/aerialfalse.
`it_802B7160` additionally sets the ordinary capsule enable/initialization
flags and invokes common damage/angle helpers. Preserve their order and exact
scale coefficient rather than silently replacing it with1/256.

The caller copies only `hitbox_data.create_hitbox`; its trailing
`hit_grabbed_victim_only` bit is uninitialized original stack state. A readable
authored descriptor should initialize this Bool deterministically and disclose
the behavioral choice. It must not reproduce stack-spill UB. Test actual
constructor field effects, reset/victim-history behavior, existing unrelated
hitboxes, declared reservation ownership, nonfinite/out-of-range fields and
resource release before model destruction.

Before adopting this descriptor, separately verify the native layout of the
body's raw `(u16*)((u8*)data + 2)` damage read and byte-scoped grounded flag.
The values above decode the typed stock initializer; this phase fixture does
not establish that raw packed-byte reads produce the same values on Windows.
Do not silently substitute a retail-intended value if an actual native layout
defect needs a source-backed repair.

## Resource-scoped effect dispatch

Standing/dash sparks call `efSync_Spawn(0x3F3, endpoint_gobj, &position)`;
modified contact uses0x41C. The actual `efasync.c` cases consume only position
for both IDs; the extra ItemLink argument passed by the source0x41C caller is
unused. No script ItemLink exposure is needed.

Propose `resources.spawn_effect(index, id, x, y, z)` with typed Int index/ID
and finite Float coordinates. Resolve the validated collection node and use
its actual GObj in the existing typed effect dispatcher; no fake Item context
or fighter fallback. If a result is exposed, preserve particle/model effect
provenance. Lifetime checks must follow any synchronous effect callback before
cached node/article writes. Keep RNG draws and coordinates in authored helpers.
Particle rendering and engine allocation are separate from the current controlled
source fixture.

## Owned fighter accessory plus post-common throw entry

`ftSs_Init_CreateThrowGrappleBeam` uses `ft_data->x48_items[4]`, whose normal
owned DAT contains a structured root/base-animation/four-throw-animation
descriptor. The native asset importer already decodes this into an owned
`UNK_SAMUS_S1`; the script API does not expose it. Existing `attachments.load`
loads public costume roots and cannot faithfully select this descriptor.

Prefer bounded immutable named fighter accessory declarations with explicit
owned item-slot/component recipes, direct generic model load/Add/Req/Anim/
attachment operations, and ordinary helper selection of motion219..222's
animation index. Required original ordering: replace/load accessory;
scale all components from instance-scale Y; add base joint/material animation;
add selected throw joint animation; set joint AObj rate only where not looping
(exclude material/texture traversal); request frame0; advance; bind raw part51.
Reuse actual shared joint operations and asset arena. Do not reinterpret this
slot as a simple graph or combat article.

Actual `ftCo_800DD398` performs motion entry â†’ animation start â†’ capture mask â†’
victim thrown setup â†’ native Samus accessory initialization â†’ common thrown
hitbox scaling. Existing `on_throw_animation` and `on_throw_finish` are later
callbacks. Propose a generic post-common `on throw_enter` event with the actual
selected motion and animation-rate inputs at the native initialization slot.
Authoring it on tick or animation changes ordering. Guard caller and linked
victim identities after the callback before continuing the cached common
hitbox loop. Test all four throw motions, rate/loop behavior, accessory
replacement, actual linked-model attachment, retirement/recycling and missing
owned metadata.

## Authored solver and lifecycle batch after contracts

Construct ordinary resources using verified **integer** node counts. The
source `x34` is s32, not Float. At source scale1, ground/air counts are30/45,
and modified mode1 doubles to60/90. Preserve source blend/truncation and use
explicit bounded failure for custom scale/count above the90-node capacity;
no silent clamp. Each node stores its actual position/velocity/CollData in
the existing pool. Private fixed/anchored bits can be ordinary article Bool
arrays indexed by node. No new native solver is necessary.

Item physics callbacks0..8 only schedule the pending solver; fighter
accessory2 executes it afterward. Accessory3 remains a distinct hitlag path.
Held/update/model animation ordering and current-owner versus immutable-creator
semantics must follow each original body. Ground capture uses state5 and
endpoint139; release must clear exact external part/capture/hitbox borrowers
before freeing every graph. Use existing generation-safe resource teardown.

Next actual-source matrix should cover constructor model/radius/type/animation
selection and partial failures; held normal/hitlag paths; ordered physics â†’
accessory2 across all nine modes; modified-target/contact paths; capture state5
and common victim notifications; hang input/cliff transitions; then damage,
death1/death2, primary death and throw accessory composition. Native uninitialized
stack bits, missing raw pointers and stale linked nodes must be classified
explicitly rather than recreated. Current accepted phase fixtures and shared
resource/joint/lifetime fixtures can be reused unchanged. Default promotion and
gameplay/renderer acceptance remain later gates.
