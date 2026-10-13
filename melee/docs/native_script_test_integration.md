# Direct receiver and Item test integration

The October 7 integration registers `native_script_receiver`,
`native_script_item_actor`, `native_script_item_scheduler` and
`native_script_item_program` as isolated CMake/CTest targets. Compiler, execution,
transition and Fighter fixtures now link the current receiver implementation and
shared runtime. Fighter preparation links the actual Item metadata/program
implementation; actor lookup uses the actual two-class dispatch.

Immutable Item program state is excluded from the rollback section alongside
immutable Fighter programs. Item actor/scheduler targets compile actual actor,
snapshot, GObj and pool code with the production rollback-section policy. The
shared interpreter runtime remains host state. Generated fixture headers have
explicit dependencies on the native bodies from which they are extracted.

Older isolated fixtures lack entities or a scene allocator. Their explicit
`script_receiver_unavailable_*` boundaries return absent identities/views;
runtime reads, writes and invokes are never fabricated as successful helpers.
The receiver fixture instead uses actual registry checks, real GObj pool
references, retained generation/epoch tokens and production Execution/VM
routing. Its motion installation and unrelated engine leaves remain controlled.
Item scheduler extraction exercises actual dispatch/state/removal/constructor
bodies, with asset, graphics, collision and audio leaves controlled.

The event payload ABI fixture uses recognized retained Object kind tags and
rejects old untagged identities, unsupported classes and an Item token supplied
to a Fighter payload. Exact metadata call/index rejection remains tested. The
event provenance runner checks all 188 named slots against actual declarations;
its 1,861 resolved and two unresolved template signatures do not establish whole
program or scheduler acceptance.

Current-source ordinary Item constructor fixtures pass `source_owner=0` and
verify that selection clears poisoned interpreter ownership while preserving
the older native asset/union/render behavior. Historical composition runners
that compile only a frozen SDK retain that SDK's constructor signature.

Verification is archived at
`A:/FreakMelee-test-archives/script-integration-oct07/receipt.json`:

- All 32 focused CTest targets pass, including the 25 direct script targets,
  current Item policy/article creation, GObj/match pools, census, heap and
  animation ownership fixtures.
- Receiver: 4,316 checks and nine production Execution entries.
- Item actor: 1,193 checks across 32 cases, including actual deferred GObj
  retirement and snapshot restoration.
- Item program: 222 checks across 37 cases, with five imported scalars and four
  published entity descriptors.
- Standalone receiver, Item actor/program/scheduler, compiler, transitions,
  events, Fighter constructor pool and animation scratch runners pass with
  hidden subprocesses and source hash receipts.

No game was launched and neither client nor content tool was linked. The first
attempt reused an older test list whose animation target transitively rebuilt
`melee_game`; it stopped at a missing runtime declaration, which the production
owner corrected. The final verified builds selected only isolated targets.
These results do not establish live gameplay, complete asset/render/physics
parity, or full Mario/Rook source package activation.
