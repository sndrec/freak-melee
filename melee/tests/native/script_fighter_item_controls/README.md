# Fighter Item controls

One connected compiler/Prepare/VM/Fighter+Item actor fixture covers nine slot,
attachment and scale controls, four held-item queries, three presentation/drop
controls, seven common held-item policies and the existing Article auxiliary binding. Source bindings
operate on actual actor storage and pool identities; native pickup, scale, throw setup and engine creation prefunction
bodies are extracted from current source. No Definition wrapper, donor behavior,
successful fake Prepare/Run or generated-C runtime is used.

```powershell
python tests/native/script_fighter_item_controls/run.py `
  --compiler .local/baseline72/compiler/clang.exe `
  --output A:/FreakMelee-test-archives/script-fighter-item-controls-fresh
```

Creator-gated binding/attachment/scale admit the exact pinned source Item bank
and immutable original creator, or a raw ordinary engine Item with matching
actual native_creator/generation/kind. No source spec is fabricated for raw Items. They do not substitute current damage owner or
revocable attachment owner. Explicit `bind_existing_held/auxiliary` restore
foreign source or raw Items without modifying those relationships. Clear methods
only clear the Fighter slot; they do not destroy, drop or release the Item.

The source VM exercises all method spellings. Tests cover signed and unsigned
high-bit identities, missing/foreign targets, invalid handle widths/Float types,
noncanonical Bool payloads, faulted actors, malformed PLink classifiers and
callback-time class changes.
Selected physical pool/native generation, payload, class, author/spec/storage,
caller program and full scene epoch must survive through publication.

Attachment uses the supplied raw joint index; a deliberately different semantic
mapping does not affect the selected native joint. Invalid index/rig cases return
healthy false and preserve prior held slots. The actual pickup body executes
the interpreted picked-up hook. The final flag repeats debug-display inheritance
after pickup, matching the audited native helper; it is not a Fighter pickup
event. Native damage/pending owner changes and ordinary reflection are retained.
Cleared attachment control stays cleared before or inside attachment; the
operation never re-enables it implicitly.

Scale uses the captured current damage owner's actual Y scale, including a
foreign raw Fighter. Ten comparisons against the actual scale body cover positive,
negative and signed-zero factors, remember on/off, JObj scale, hitbox scaling,
grab-range and ECB fields. The immutable creator/control roles are unchanged.
The native scale body and its hitbox/range/ECB helpers are pure field updates;
no authored callback exists inside that arithmetic. Source/Item/current-owner
guards still surround the operation and precede throw-scale/output publication.

Cleanup-window behavior preserves the previous defined native/source admission:
an Item already marked removal-in-progress can be bound while its physical and
logical actor/view remains live. The captured removal bit must remain unchanged.
Entering removal during an operation rejects output and stops continuation;
completed logical retirement is never admitted merely because a GObj remains.
Cases distinguish initial queued cleanup from callback-time removal. Item and
Fighter retirement, same-slot reuse, native generation changes, actor faults and
state-schema corruption preserve output. Snapshot tests restore actual Fighter
and Item actor records with explicitly restored native peers and preserve both
held pointers and revoked attachment control.

Held/auxiliary queries inspect actual native slots, including foreign ordinary
Items, preserve the full creation-ID word and report absence as -1. Four further
native comparisons cover show/hide and pause/resume using the unchanged common
Item and JObj flag bodies. Drop notification runs the actual common holder body
and an interpreted drop hook, with controlled statistics leaves.
Caller retirement, new life, Item retirement and callback failure exercise its
continuation guards; no complete stock pickup/drop integration is claimed.

The four physical held-slot observations now use the substantive HeldItems
backend: held_id/kind/motion/frame. Auxiliary/use-category policy still uses its
existing source-owned control backend. The actual ftLib_GetItem and native
target-held getter bodies provide 28 field comparisons, including NaN, infinities,
signed zero and full32 ID bits. Eight Item signal executions pass through the
actual compiler, Item actor, Execution/VM, retained receiver pool and typed native
property reads. `holder.items.*` works on an ordinary foreign Fighter without a
program; `current_owner.items.held_motion` can observe a different ordinary Item
in that Fighter's held slot. Registry absence and logical removal return -1/0;
an unrelated Item script fault does not erase physical fields. Action controls
retain their own fault/ownership checks.

Unknown classes, mismatched entity identity, caller generation/class/epoch and
wide IDs are covered. Four test-only injections after sampling the actual native
slot getter exercise source new-life/kind/free and slot replacement before any
Item dereference/output. This native getter is pure in production; injection is
a continuation hazard check, not a claim of an authored callback at that point.
The new source bodies are real compiled callbacks, not generated-C runtime code.

The additional common-policy suite covers `items.notify_pickup(Bool)`,
`items.notify_held_neutral()`, `items.reopen_blocked`, `items.throw_held(motion)`
and `items.create_engine(kind,x,y,z,vx,vy,vz,facing,group,damage)` through the
actual source compiler, prepared programs, VM and actors. Pickup notification
executes the native common body and real interpreted `item_pickup_ext`; temporary
`x1978` precedence and absence preserve its native statistics/callback ordering.
Holder-neutral notification executes the actual prepared Item hook, including
missing hooks and logical/physical retirement; no native authored article policy
is substituted. A successful held-neutral hook that retires the holder leaves
the Item live; its retired Fighter held-slot page is protected with PAGE_NOACCESS
until the binding returns, proving caller validation precedes that cached read.
The reopen flag's guarded Bool read/write is tested through the
VM and restored alongside the explicitly saved native Fighter peer.

Twenty-six additional direct-native/VM comparisons cover every inclusive common
throw motion from `ftCo_MS_LightThrowF` through `ftCo_MS_HeavyThrowLw4`: command
clearing, exact damage-multiplier speed, facing selection and setup order. The
motion, animation and accessory leaves are controlled. Retirement/fault at the
first two leaves tests the actual common body's continuation guards. These cases
do not claim actual animation execution or throw release/collision acceptance.
Arbitrary special motion rows reject before that native table is entered.

Engine creation uses the actual `Item_InitSpawn`, `Item_CreateEngine`, default
hold-kind selection and common-asset availability check. The common Article table
and `Item_8026862C` constructor are controlled, with real GObj allocation and raw
Item identities; no full DAT/model constructor acceptance is claimed. Cases
verify supplied position/previous-position, velocity/facing, initial collision,
airborne default, full signed/unsigned group bits and finite damage truncation.
Unavailable assets, allocation absence and immediate constructor cleanup share
the actual NULL/-1 boundary. Physical or logical caller retirement after a live
creation preserves the Item side effect and withholds output. Invalid Float/wide
group words, nonfinite values and out-of-domain damage reject before allocation.

The ordinary-item group executes full source create/attach/scale/bind flows for
Bomb, Saturn and Sword, preserves native current-owner reflection independently
of creator admission, rejects foreign/stale creator identities, and exercises
callback-time creator, Article/descriptor, attributes, logic, JObj, generation,
removal and caller changes. Raw engine pickup callbacks are controlled C leaves
of the actual common pickup routine; authored source callbacks still execute
through the real VM. Native raw Item/Fighter peers restore alongside the actor
state where relevant; no new persistent raw provenance record is introduced.

`items.spawn_food(x,y,z)` executes the actual Foods helper, airborne prefunction,
ECB-center arithmetic, availability check and success-counter increment. Source
coordinate helpers prove once-only X/Y/Z order. Direct native/source comparison
checks supplied previous position versus current ECB center, fixed facing and
unowned parent defaults. Gate failures, constructor absence/immediate cleanup,
nonfinite input and caller retirement preserve the native outcome boundary.
A completed food side effect and counter remain after caller retirement.
Stage item-policy/frequency getters and common Item constructor/assets are
controlled; actual food variant selection/RNG/rendering and gameplay are not
claimed. This is distinct from Article `item.spawn_food()` dispersion.

The selected package/immutable asset table and rig publication boundary is
controlled at stable addresses. Compiler, metadata preparation, program
publication, VM, actors, GObj pools and exact production source-layout checks are
real. Rendering/audio/transform leaves are controlled and no DAT constructor or
graphics initialization is claimed. JObj pointers rely on current engine peer
lifetime; this does not add per-JObj allocation generations or whole-match
snapshot ownership. Current native damage ownership is captured at operation
entry; the native pointer field itself is not a new persistent provenance record.

Immediate `items.release_held(vx,vy,vz,multiplier)` uses the actual shared
`Item_8026ABD8` dispatcher and drop-sound selection. Ten native/binding pairs
cover source and ordinary heavy Items, signed-zero/negative damage factors,
and normal collision-tail ordering. Compiled helpers release both item families
and preserve the absent-slot no-op. Authored Dropped hooks execute through the
actual VM. Both authored and ordinary dropped callbacks exercise native generation, class,
removal, caller retirement and destruction. Destruction and same-slot replacement stop native continuation and
remain successful terminal outcomes; faults and surviving identity changes
reject output. Caller retirement rejects output without erasing completed
native side effects. Invalid signatures and nonfinite arguments are rejected
even when the held slot is empty. Transform/detachment, ownership and collision
leaves remain controlled; complete gameplay release physics is not claimed.

Processes use `CREATE_NO_WINDOW`, 120-second bounds and fresh private A: outputs.
Consumed source/header/compiler hashes are recorded and verified at completion.
No game, global build or real user project is launched or changed. The existing
`script_article_lifecycle` regression separately retains all prior scenarios and
its 16 actual pickup comparisons, with auxiliary calls routed to the substantive
new Fighter control backend.
