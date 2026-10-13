# Source guard fixture

This bounded console cohort uses the actual source frontend/catalog, compiler,
program preparation/publication, Fighter actor, Execution/VM, receiver storage
and GObj pools. It exercises `guard.create`, `guard.update_position`, five Bool
flags, two Float hitlag fields and recorded integer damage/Float facing.

The reflection-direction cohort reads the engine's last reflector contact
direction through a compiled owned helper. It retains all tested Float bits,
leaves Fighter data unchanged, rejects writes, and rejects stale or wrong-class
contexts before publishing output. This covers the generic observation API;
projectile collision itself remains outside this fixture.

The cohort also runs source helpers through the shared guard timers, release
and powershield flags, three signed input slots, shield health and read-only
shield data. Distinct values exercise Yoshi/common layout interoperability and
signed integer endpoints. The actual common guard release-input body consumes
those writes before the VM reads them back. Seven common tuning reads use the
real property backend with controlled common data. Missing data, noncanonical
Bool/wide Int inputs and final-admission identity changes reject publication.

This does not execute a complete scripted Yoshi shield,
animation/pose copying or full powershield logic. The fixture's property routing
is explicit; public production property dispatch is compiled in the native
targets, not executed by that fixture.

```powershell
python tests/native/script_guard/run.py --compiler .local/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-guard-final
```

The runner requires a fresh A: directory, uses `CREATE_NO_WINDOW`, compiles the
backend and test with strict warnings, and verifies recorded input hashes after
execution. `receipt.json` records the commands, cohort counts and source hashes.
`reference_source.json` pins independent native constructor/update/ShieldHit
reference bodies to a Git commit. No shared build or game runs.

The native comparisons use complete `ftColl_8007B1B8` and `ftColl_8007AEE0`
bodies. Forty-two joint/radius combinations compare exact Fighter fields and
callback-ID preservation against the reference admission policy. Negative,
out-of-span or wide joints and negative/nonfinite radii are healthy no-ops;
valid raw joints 43/104 are not semantic part indices. Recreation replaces the
native pointer and interpreted slot; `None` clears explicitly. One position
refresh pair proves its exact skip-update reset. One independent ShieldHit pair
compares the actual ProcessHit branch with a native reference reaction and a
real source helper writing guard fields and state.

Properties run through real VM Read/Write routing. Both Float fields preserve
negative zero, NaN payloads and infinities. Integer contact damage preserves its
signed native value. Negative cases cover malformed helper IDs/result/arity,
noncanonical Bool writes, wrong scalar types/widths, missing joints, undersized
JObj class metadata, readonly writes and identity changes during admission.
The compiler validates owned direct CodeRef syntax/None; the backend validates
root ownership and zero-argument Void ABI before any guard mutation. Those are
distinct gates; helper result/arity rejection is not claimed at compile time.

The exact ShieldHit callback branch and subsequent native damage reset execute;
ordinary-shield reactions remain a separately controlled native branch. Actual
VM faults, detach, deletion, pool reuse and generation changes stop callback
continuation. Userdata, classifier, schema and parameter substitutions can occur
after a callback successfully finishes its local writes; the retained native
lifetime guard rejects them before cached Fighter reads, damage reset or the
native continuation. Explicit position-write/admission observers provoke these
mutations. They are test boundaries, not natural reentry inside pure guard
constructors. VM scratch must remain balanced in every case.

Imported package/rig/JObj class data and physical Fighter userdata are controlled;
actual class ancestry, source-layout admission, actor ownership and program
execution run. The position write is a controlled ordinary native field write.
Ordinary shield contact reaction/audio leaves are recorded. This fixture does
not prove collision-contact detection, motion/animation changes, rendering,
complete ProcessHit hitlag/tails, full-match snapshots or in-game Peach behavior.
Unrelated engine binding families reject or abort rather than reporting success.

The fixture also links the actual combat backend and runs `hurtboxes.define`
through source compilation, the attached fighter actor and VM. Its capsule
initializer and all-status routine are exact bodies pinned to `7fde757c9`.
1,512 combinations cover valid/absent capsule and raw joint indices, all three
height classes, both grab flags and native Float scale values. Whole-Fighter
comparisons preserve cached endpoints, update flags and unrelated data. The
scripted shield recipe disables all capsules, then enables one capsule on the
owned shield joint, matching the same native operations. Wide bone values never
narrow to a different joint; bad types, missing tables/joints, excessive capsule
counts and source identity changes fail without output publication. New low/high
height constants are exercised through the compiler and VM.

Unrelated combat damage, RNG, effects and fighter-wide status leaves abort if
reached. Capsule definitions are not proof of contact detection, full Yoshi shield
behavior or rendering. The fixture still uses explicit property routing; native
targets compile and link the production dispatcher.

The connected actor now also runs the generic raw-part deferred-position effect
operations. Four native queue bodies match pinned `7f6ab2bd6`. Immediate and
deferred paths, both payload signatures, absent indices, negative zero, packet
order, queue clearing and allocation failure are compared. Moving a joint after
enqueue changes the sampled position; changing the original scalar does not
change its copied value. A source helper reads the actual `model.effect_scale`
backend before queueing, reproducing the native first-Float attribute contract.
The geometry sampler and typed position/scalar recipe delivery remain controlled
leaves; no particle/model recipe construction or rendering acceptance is claimed.
The fixed sixteen-packet allocator is a fixture boundary, not evidence that the
production ObjAlloc never grows its backing heap. Later arbitrary replacement of
a queued borrowed joint remains outside this operation's guarantee.

Whole shield-transition scheduling runs through the actual compiler,
Prepare/Publish, attached Fighter and VM. Complete production bodies from the
nine native entry sites execute; ten cases cover all six modes, both shield-hit
Boolean values and the alternate hold/release paths. Named `Shield` constants
are checked through compiled comparisons. Release statistics precede delivery;
the special-hit tail follows only successful delivery. Wrong argument count/type
leaves output unchanged. Script faults and eight controlled lifecycle/storage
changes stop that tail; retrying startup on a faulted actor never calls stock
entry. All stock/fighter-specific motion/effect leaves abort if reached in these
authored-hook cases. Startup completion now executes the actual drain body;
break entry, release statistics and special-hit bookkeeping remain controlled
leaves. No actual motion changes, powershield input arbitration or visual/gameplay
parity is claimed by these scheduler tests.

Nine shared guard operations and both radius/alpha hooks run through the same
compiler, actor and VM. Operation recipes, SDI, depletion, window timers and
stock scale/alpha calculations are compared with bodies pinned to `7ff672421`.
Comparisons cover entire Fighter/JObj state and relevant recorded leaves. The
common reflector constructor is checked against that same revision. Shared
SDI replaces a previous interpreted post-hitlag slot; nested hold/release
requests reach the authored whole-transition hook. `reflector.active` is read
through the VM for both flag values.

Wrong arity, absent common/rig data, undersized joint classes and a missing
reflect-hit handler reject without output publication. Query faults and eight
actor/storage mutations stop scale/alpha continuation; five rig/table mutations
inside a scale query leave its original joint untouched. The same eight actor
mutations during controlled shield-break entry stop the subsequent sound and
drain result. VM scratch remains balanced. Imported rig/class metadata, property
routing, alpha delivery and break/statistics/audio leaves are controlled. These
checks establish shared-kernel and dispatch behavior, not collision traversal,
full shield-break motion, rendered transparency or match-level parity.

The guard-flow cohort adds eleven interpreted input/physics/collision operations
and both roll/spotdodge entry hooks. Complete current input predicates, entry
scheduling, common ground movement and powershield priority traversal are
compared with bodies pinned to `926b6e33d`. Predicate cases cover both sides of
timing/stick thresholds, held/pressed input, C-stick, facing, held item and delayed
throw timer; physics cases cover friction, speed, floor normal, SDI selection and
geometry refresh flags. False input leaves that retire an actor cannot advance
to another check. Authored hook faults and actor/storage replacement stop
remaining roll/spotdodge initialization and statistics. Motion/animation,
item-throw/drop entry, platform query, ground-friction query and terrain collision
are controlled leaves. Native predicate reference delivery uses controlled
powershield/release entry rather than an attached script actor. This establishes
input ordering and shared-kernel parity, not full motion or stage/gameplay parity.

The jump-policy cohort runs `jump.update_animation()`, `motion.fall_animation`
and `damage.reverse_knockdown_pose` through the same compiled actor/VM path.
The complete animation query (including normal/blended AObj flags), jump
completion and fall-entry bodies are compared with `040bc5613`. Sixty-four
animation combinations compare whole Fighter state and the motion trace;
24 selector cases cover both fall states, unrelated states and signed extremes.
Both knockdown flag values are checked. Missing active joints, malformed
arguments, stale contexts and eight actor/storage mutations reject or stop
continuation before cached fall writes. Motion initialization remains a
controlled leaf; this establishes completion/selector policy, not skeletal
deformation, rendered pose or complete aerial-jump gameplay parity.
