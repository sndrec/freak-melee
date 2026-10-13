# Native Fighter snapshot ownership and legacy animation scratch proof

2026-10-07 source audit and approved fixed Fighter owner first slice. Unused native
archive scratch is removed and the actor/fixed satellites are migrated as
described below. The full graph/VM/global aggregate is still a proposal. No game was launched
for this audit. The existing broad page oracle remains
necessary for all current runtime owners.

## Implemented fixed owner first slice

`native_fighter_pool.c/.h` now owns64 stable Fighter-first slots with full backup,
all140 parts and124/32 ordinary DObj pointer arrays, actual32-byte-aligned
sizeof19,136. Normal and demo create allocate that slot directly. Original
constructor phase timing for backup/parts/lists and ordinary cleanup callbacks
is preserved; final retirement frees one slot. Four obsolete native allocator
families are removed. Source audit found the x2040 allocator also serves Kirby
hat/copy and Purin extra lists, so this genuine separate owner remains active.

The real GObj scene owner preflights/binds Fighter storage once per epoch and
logically retires it before preload, with script actor retirement coordinated by
root. Family normal/demo initialization requires an active empty pool and does
not reset live slots. Pool and embedded buffers are NON-HOST .mstate and recognized
by the existing broad global page oracle. Packed typed save/load validates owner
aliases/counts/geometry/epoch before mutation; other owner relations are not
silently recreated or dereferenced before their aggregate restore.

Focused actual-source fixture:
`A:/FreakMelee-test-archives/native-fighter-pool-consumer-shape-final/receipt.json`,
10 scenarios/1,037 assertions. Full64-slot packed owner snapshot1,225,272 bytes.
Read `tests/native/native_fighter_pool_consumer/README.md` for controlled boundaries
and exact allocation/lifecycle/oracle/exhaustion evidence. Whole-game/graph/VM
snapshot completeness and live timing remain unaccepted by this fixture.

## Confirmed unused32KiB owner

`Fighter.x59C` and `x5A0` point to legacy FigaTree **archive relocation buffers**.
They are not the live animation/interpolation skeleton. Source references are
limited to:

* `Fighter_FirstInitialize_80067A84` initializes the pool at0x8000 bytes, with
 32-byte alignment. Demo initialization used0xB000.
* `ftData_80085B10` and `ftData_80085B98` allocate exactly two buffers per actor.
* The buffer writes/reads, sibling copies, ARAM reads, archive relocation/parsing
  and public-address lookup in `ftData_80085CD8` / `ftData_80085E50` exist only in
  their non-native `#else` branches.
* Native `ftData_80085CD8` assigns `x590` and `x5A4` from the resident
  Fighter_WaitAnimData.x14 entry. Native `ftData_80085E50` returns that entry
  directly. `ftData_80085FD4` supplies native Nana->Popo missing-clip fallback
  outside demo mode. No native buffer reconstruction is required.
* `Fighter_Unload_8006DABC` formerly returned both unused blocks to ObjAlloc.

Whole-source searches also find `p_ftCommonData->x59C/x5A0` floats. Those belong
to the common attributes struct and are unrelated to the Fighter pointer fields.
They have not been removed. No native consumer of either Fighter buffer payload
was found after checking creation, selector/track paths and unload references.

The actual native animation importer in `ftData_80085A14` loads source bytes and
uses `ftAnimationBank_Import` into a resident arena, filling the x14 clip table.
That arena and clip graph lifetime remain necessary; this cleanup removes the
unused per-actor relocation buffers, not the imported animation bank.

Production cleanup after root approval:

| File | Exact native change |
| --- | --- |
| `ftdata.c` | Normal/demo setup assigns x59C/x5A0=NULL; hardware allocation remains in the non-native branch. Native selector bodies are unchanged. |
| `fighter.c` / `fighter.h` | Legacy scratch allocator definition/extern, normal initialization and buffer frees are non-native-only. No dead native mirrored allocator is kept. |
| `ftdemo.c` | Legacy0xB000 allocator initialization is non-native-only. Native demo clip selection remains direct/resident. |

The earlier21-family census recorded16 live32768-byte buffers for eight actors:
524,288 bytes of unnecessary live allocation. Removing them removes one current
native ObjAlloc family. That number is a workload allocation saving, **not** a
complete snapshot-size or timing claim. Native Fighter struct pointer fields
remain explicitlyNULL as agreed; no compatibility shim or new pointer API exists.

## Actual no-access proof

`tests/native/fighter_animation_scratch/` extracts complete current source bodies
for normal/demo scratch setup, native clip selection/fallback, the real
ftAnim_8006F4C8 track loop and Fighter unload. Before cleanup the fixture supplies
PAGE_NOACCESS allocations for all16 archive buffers. Selection and track setup
also receive an independent inaccessible scratch pointer, before and after.
Any attempted buffer payload read/write terminates the fixture.

Before receipt:
`A:/FreakMelee-test-archives/fighter-animation-scratch-before/receipt.json`:
73 cases,744 assertions,16 allocations/frees,128 track attachment calls,
eight interpolation cleanup calls.

After receipt:
`A:/FreakMelee-test-archives/fighter-animation-scratch-after-preprocess/receipt.json`:
73 cases,680 assertions,zero scratch allocation/free, the same128 track
attachments and eight interpolation cleanup calls. Native preprocessed output
contains no fighter_x59C_alloc_data symbol. Both traces retain the actual
normal/alternate-joint choices and Nana CPU/demo fallback.

Controlled boundaries: imported descriptors are fixture-provided; allocation,
track installation and dependent cleanup primitives record arguments. No real
FObj decoder, resource renderer or complete Fighter snapshot is accepted by this
test. Negative motion indices remain outside its defined source domain. Full
client build/game/rollback checks belong to the root after source freeze.

## Animation state that must remain

The actual interpolation owner is `Fighter.x8AC_animSkeleton`.
`ftanim_skeleton.c: ftAnim_8006FE48` creates it through
`ftParts_8007482C(fp->x108_costume_joint)`; that loads a runtime JObj graph with
ftIntpJObj class. `ftparts_bind.c: ftParts_8007462C` installs its nodes into
`parts[].x4_jobj2`. The primary graph is owned through the fighter GObj HSD object
and `parts[].joint`. Both graphs and their mutable animation/material satellites
must remain coherent with the Fighter record.

`ftanim_update.c` advances x8A8_anim_frame, reads the skeleton's framerate,
animates either primary or alternate graph and blends/copies SRT via
ftAnim_8006FE9C/FF74. `ftdynamics.c` consumes the alternate skeleton and part
matrices. `Fighter_Unload` removes x8AC and releases other runtime joints. None of
these paths was modified. Frame number alone is not enough to recreate their
state: retain JObj SRT/matrices/dirty flags/refcounts, AObj timing/flags/rate,
FObj decode/interpolation state, constraints, dynamics history and ownership.

`x590`, x5A4, x58C, current animation counters/flags and script cursors remain in
the mutable Fighter record. They refer to resident immutable clip/source tables;
their values still restore with the actor. Native x598/x5A8 are not silently
repurposed or removed by this cleanup.

## Concrete fixed Fighter owner shape

Use stable process-reserved scene-epoch storage, compatible with the accepted
GObj pool policy. Bind at the actual fighter family initialization transition;
prove normal/demo initialization does not reset a family with live actor owners.
The implemented first slice reserves a provisional64 actor slots, matching the
current interpreted actor reservation, with explicit exhaustion policies and
all-scene census still required. This is an accepted implementation bound, not a
stock hard maximum.

One logical actor slot can own the fixed per-actor satellites directly:

```
FighterOwnerSlot:
    Fighter actor
    extended_attribute_backup[FT_CHARACTER_EXT_ATTR_CAPACITY]  # 0x424 bytes
    FighterBone parts[MAX_FT_PARTS]                            # all140
    HSD_DObj* primary_dobj_list[124]
    HSD_DObj* secondary_dobj_list[32]
```

Keep the slot occupancy/generation/free order and scene epoch separately from
payload, as NativeMatchPool already does. Actor is the first member, so the actual
Fighter pointer remains the stable slot base. Set dat_attrs_backup, parts and
both list data pointers directly to this slot's storage at real constructor call
sites; dat_attrs retains its source ownership/alias contract. Remove obsolete
native satellite allocators/frees when call sites migrate, not through an adapter.

Current compiled sizes are Fighter13,440, parts140*24=3,360, backup1,060, pointer
lists992/256 bytes. Actual32-byte-aligned owner sizeof is19,136, before external
graph/dynamics/VM owners. Eight occupied fixed owners contribute153,088 raw payload
bytes without the524KB legacy buffers. This does not include packed metadata or
prove a complete snapshot footprint.

Initially save the full actor and full fixed satellites. This avoids guessing
which inactive list/reservation fields a future callback reads. Allocation must
initialize omitted inactive slot data before exposure. In particular parts139
can be a live article endpoint reservation beyond a60-bone physical rig; trimming
the parts snapshot to physical bone count would lose a valid borrowed reference.

Mutable source dependencies:

| Data | Actual owner/use | Save policy |
| --- | --- | --- |
| Fighter record | Motion/CPU/input histories, collision, hit/hurt/capture, callbacks, timers, unions, native_module_state, attachments and captured identities in `types.h`; creation/respawn/unload in fighter.c. | Full record initially, including native private pointers to stable/resident owners. |
| Extended attributes | Constructor allocates backup; PUSH_ATTRS copies source and aliases dat_attrs to it; authored copy/reload/multi-jump bindings require instance-owned backup. | Full0x424-byte mutable backup plus pointer/alias values. Do not alias immutable ft_data.ext_attr. |
| Part map | `ftParts_80074E58`, `ftparts_bind.c` primary/alternate traversal and visibility/dynamics flags, resource reservations. | All140 entries, including borrowers. Graph instances remain separate owners. |
| DObj pointer lists | Base allocated in ftParts_80074E58; secondary in ftCo_800C884C; visibility/material/metal code writes list contents/counts. | Full fixed arrays initially plus counts/selection already in Fighter. |
| Runtime graphs | Primary GObj model, alternate x8AC, accessories/x20A0/x2184, native attachments, lights, materials/textures/constraints. | Their actual mutable graph pools and reference counts; freeze source descriptors. |
| Dynamics/camera/effects | dynamic_bone_sets point into live DynamicsData; camera subject, shadow, effect queues and article/held/capture relationships. | Snapshot corresponding owners and coherent links; no eager recreation/destructor calls. |
| Allocation identities | Fighter spawn counter, private slot generation/epoch, GObj owner/data pointer, Player primary/secondary role pointers. | Save together. Respawn increments x8_spawnNum without necessarily replacing the Fighter allocation. |

Frozen source assets/programs may remain outside packed snapshots. All supported
mutable imported/shared data must be separately identified; an asset pointer is
not proof of immutability. Runtime models and animation cursors are not assets.

## Interpreted actor state is a separate required owner

Current `native_script_fighter_actor.c` stores64 ScriptFighterActor records in
.mstate. Each has GObj/full generation/kind, active/fault state, tick/completed
tick counters, callback indices, an8,192-byte state capacity, StorageInstance and
Execution bindings. `Fighter.native_module_state[256]` alone does not include it.

Proposed packed interpreter owner record: actor slot, active/lifetime identities,
kind/program/layout/parameter identities, callback array, mutable tick/fault
state and the **declared owner state_bytes** from the current valid layout. Keep
the maximum reservation physically fixed, but save only declared active state
after validating capacity/layout. Metadata must restore dormant/retired slots and
their reuse order consistently with the actor/GObj registry. Copying state bytes
alone cannot resurrect an actor binding.

On load, validate all pool slots, actor links, program/layout/parameter identities
and callback/function bounds before mutation. Restore Fighter/GObj ownership and
interpreter slot identities coherently, then reconstruct internal storage.state
and execution.storage links to the stable reserved bytes and frozen program.
The existing NativeScriptStorage_Load deliberately rejects a different/retired
instance lifetime; it is not by itself an aggregate resurrection operation.
Respawn's nativeScriptFighter_NewLife preserves authored variables while changing
generation and retiring some callback slots; snapshots must restore that exact
contract, not recreate/load the program or reset all variables.

VM registers/frames and event payload pools in native_script_fighter_program.c
are transient host scratch. NativeScriptVM_Run preserves/restores its original
depth/register_top on return; execution push/pop is synchronous. An explicit
snapshot boundary must assert VM depth/register_top and payload depth are zero
and no native callback dispatch is live. Do not serialize stale scratch capacity
or rewind host event serials to resurrect expired opaque callback handles. Program
publication already refuses replacement while live actors reference that kind.

## Next acceptance batch before actor migration

1. Compile actual owner sizeof/capacities, trace source satellite allocations and
   family normal/demo initialization across CSS/VS/results/practice.
2. Link actual creation/respawn/unload bodies against the new fixed owner: A/B/
   free/reuse/resurrect and exact generation/Player-pair/list/attribute aliases.
   Explicitly cover pool exhaustion and partially constructed cleanup.
3. Save/restore common and per-character attributes, all part flags/reservations,
   list entries and command/input/CPU/capture state; verify joint/effect/dynamics
   references with their actual lifetime guards.
4. Join interpreted owner state/callback/tick/fault snapshots, including NewLife,
   retired/reused slots, wrong program/layout/parameter and invalid record rejection.
5. Differential complete simulation/draw windows against the page oracle, then
   root-run eight actors and scene transitions. Measure actual packed bytes and
   save/load times only after ownership parity. No sub-1MB or sub100us promise.

This audit supplied the actor ownership schema and approved dead native scratch
cleanup; the fixed Fighter owner first slice is now migrated and console-tested.
It does not migrate VM/graph/global owners or declare complete explicit match
snapshots accepted.

## Root live integration observation

The client/tool build and 25 focused CTests pass after cleanup and motion-scheduler
integration. Eight-fighter gameplay completed 2,000 samples with no topology error.
The previous callback diagnostic counters and object/proc populations matched.
The partial live payload peak dropped exactly 524,288 bytes to 864,088, with 20
allocator families rather than 21. This confirms the removed allocation owner;
it still omits globals, asset backing, dynamic satellites and allocator metadata.
Receipt: `A:/FreakMelee-test-archives/live-interpreted-motions-no-scratch/receipt.json`.

All 64 full rollback endpoints reproduced with the remaining decoder/pose graph
still captured by the broad oracle. Mean 168.31 ms, all 64 deadline misses. The
Fighter pool/schema proposal above is not implemented and the full-state size /
speed target remains open. Receipt:
`A:/FreakMelee-test-archives/live-interpreted-motion-rollback/receipt.json`.

## Root owner integration observations

Normal/demo fixed-owner integration passed the client/tool build and 27 focused
checks. The actual aligned 19,136-byte owner includes complete fixed satellites;
only occupied records/free metadata are copied by its packed snapshot utility.

Live eight-fighter gameplay completed 2,000 samples with every preceding diagnostic
counter and owned-state hash matching, zero topology errors, and 17 allocator
families. Partial payload peak 864,280 differs from the preceding 864,088 by 192 bytes
(eight padded fixed slots). This is allocator/class payload observation only,
not complete mutable state or a total snapshot size.
Receipt: `A:/FreakMelee-test-archives/live-native-fighter-owner/receipt.json`.

All 64 complete broad-oracle rollback endpoints reproduced after 15 replay ticks,
with Fighter backing/free order and script state captured as actual globals.
Mean 173.91 ms /64 deadline misses. Graph/VM/global owner packing and backend
replacement still remain. Receipt:
`A:/FreakMelee-test-archives/live-native-fighter-owner-rollback/receipt.json`.

## Interpreted owner packing

The interpreter actor snapshot now saves only each live fighter's declared root
state bytes plus its logical slot, GObj reference, creation generation, program /
layout / parameter identities, clocks, fault flag and callback/motion code IDs.
No immutable program, spare 8 KiB reservation, native asset or VM scratch is copied.
Scene epochs follow the actual GObj owner. Load validates the complete candidate
and restored peer generations before mutation, then reconstructs live storage
and execution links without invoking authored callbacks or reinitializing variables.

The actual callback fixture passes 191 checks, including retired-view resurrection,
invalid identity/phase/callback/peer rejection and scene invalidation. In the live
eight-fighter benchmark, four Popos carry the diagnostic source program. Its packed
state peaks at 656 bytes. All 64 windows pass both packed-vs-broad comparisons and
full 15-tick replay endpoint checks after deliberately loading the later script
owner and rewinding it again. Capture time totals 2.76 ms over the run; two loads
per window total 2.11 ms. Those numbers cover this small **script owner only**.

Receipt: `A:/FreakMelee-test-archives/live-script-packed-state-rollback/receipt.json`;
the `[script-snapshot]` marker and full-window result are in `stdout.log`.
Full rollback still averages 182.32 ms with 64 deadline misses in this diagnostic
run. Animation, graph, item, stage and remaining global owners are not replaced
by this record. Full snapshot/performance acceptance remains open.
