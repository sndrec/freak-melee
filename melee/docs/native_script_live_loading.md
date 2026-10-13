# Direct interpreted fighter lifecycle entry

The client can now validate a source-only program at startup and bind its
supported fighter lifecycle hooks directly. `--script-kind <numeric FighterKind>`
and `--script-source <path.fms>` require an offline direct match. This is an
authoring/integration diagnostic, not the finished Workshop/package loader.

`native_script_compile.c` builds in an independent caller-reserved bank and
publishes only after schema, signatures, all bodies, helper graph, bytecode,
execution bindings, storage and immutable parameters validate. Failed compilation
preserves the previously accepted bank byte-for-byte. The initial schema validates
root declarations, helpers and named fighter hook actions. Wider move/entity/asset
metadata requires an explicit validator; unsupported lines are not ignored.

`native_script_fighter_program.c` owns immutable kind selection and transient VM
scratch outside rollback state. The separate actor owner unit contains actual
mutable state, identity, clocks and fault flags in `.mstate`. Actor state is fixed
at 8 KiB per reserved slot; allocation does not occur during invocation. Program
replacement is rejected while any actor references the selected kind.

Load, tick, death/reset, late update and userdata removal have migrated call sites. A selected hook executes
its program/code ID; no per-authored-function native thunk or generated C is used.
Untranslated/absent hooks retain their existing native behavior during this
partial migration. Other declared callback contracts are rejected by publication
until their real scheduler sites migrate. This is not an independent complete
fighter clone; that still needs all relevant hooks, entities, assets and metadata.

Damage, death1/2/3, pre/post-hitlag, dealt-damage and one-shot state-change
callbacks now have explicit instance registration. Publication caches their
source function IDs; registration/dispatch never scan source names or install
authored machine-code pointers. Root zero-argument Void helpers can occupy these
slots too, while wrong signatures, owners and event identities are rejected.
Motion reset clears the same migrated instance slots as the original engine.
A shared native engine assignment supersedes its script registration. Respawn
clears the original death slots while retaining ordinary authored variables.

The focused fixture compiles actual source and runs production actor owners,
bytecode, registry, payloads and storage. It mechanically extracts the complete
damage/hitlag call sites; 84 checks cover delivery/order, native overwrite,
clearing, respawn, one-shot behavior and retirement during a callback. Asset,
animation and native primitive boundaries remain controlled. Native article
cleanup that writes these fields (including grapple death1 cleanup), other hook
families, motion callbacks and entity owners still await their own migration.
This is broader callback integration, not full-roster independent acceptance.

Common reset/respawn changes `Fighter.x8_spawnNum` while retaining its allocation.
The actor view rebinds that new creation identity at the actual reset site,
preserving authored state. This retires earlier invocation contexts. The first
live attempt counted entry requests despite failed generation validation and
left state zero; it is explicitly excluded from accepted evidence. Reporting now
distinguishes completed interpreter calls from requests and records actual state.

## Accepted live observations, 2026-10-07

Four CPU Popos in an eight-fighter Battlefield match each completed all 2,000
requested interpreted ticks with no fault. The source's Int counter reached
2,000 and Float accumulation reached 250. Each active-state hash was
`52638c803ad01f69`, matching the independently computed little-endian eight-byte
state. Generation changes during gameplay were retained. No authored compiler or
DLL was built/loaded. Other fighter behavior remained native stock behavior.
Evidence: `A:/FreakMelee-test-archives/live-interpreted-fighter-life/receipt.json`
and `stdout.log`.

The same source ran in the actual eight-actor rollback benchmark. All 64 windows
reproduced the complete endpoint after 15 replay ticks with drawing retained.
The four script owners ended at exactly 664 completed/requested ticks (600 warmup
plus 64 advancing ticks), with matching owned-state hashes and no faults.
GObj/proc pool ownership and actor variables were saved by the existing broad
global/page oracle. Evidence:
`A:/FreakMelee-test-archives/live-interpreted-fighter-rollback/receipt.json`.

The observed complete batch averaged 175.53 ms and missed the 16.667 ms deadline
in all 64 windows. Page tracking still dominates. This is correctness evidence
for the new live entry and owners; it does not establish full-template coverage,
interpreter speed acceptance, sub-100-us snapshots or complete explicit rollback.

## Broader live callback observation, 2026-10-07

`tests/native/script_fighter/live_callbacks.fms` registers matching damage and
hitlag events, and a zero-argument helper as the dealt-damage callback. Four CPU
Popos in an eight-fighter Battlefield match each completed 2,000 ticks and 2,000
late updates. Damage counts ranged from 17–35, hitlag entry 18–29, hitlag exit 44–48,
and dealt-hit 17–27; reset/death callbacks counted 1–2. Entry/exit counts are engine
call counts, not a claim that every registration survives until its matching exit.
All four seven-Int owned-state hashes matched independently reconstructed values;
no VM fault or topology error occurred. Native stock actions, other hooks and
Nana behavior remain in use. Receipt:
`A:/FreakMelee-test-archives/live-interpreted-fighter-callbacks/receipt.json`.

The same registrations/helper and persistent variables reproduced all 64 actual
full-state rollback endpoints after 15 replay ticks, with eight fighters and draw
callbacks retained. Each script owner ended at 664 advancing ticks/late updates;
callback counters were restored too. The broad page/global oracle still supplied
snapshot capture. Mean 170.36 ms, all 64 deadline misses: no explicit-whole-state or
16.667ms speed acceptance. Receipt:
`A:/FreakMelee-test-archives/live-interpreted-callback-rollback/receipt.json`.

Shutdown diagnostics now print up to 32 scalar owned variables by source name,
alongside the existing completed/requested clock and hash. Arrays are omitted;
this is an integration diagnostic, not the Workshop's finished variable inspector.

## Motion scheduling and scratch cleanup, 2026-10-07

All eight special entry paths and the actual animation/input/physics/collision/
accessory scheduler sites can now select validated interpreted function IDs.
Owned layout agreement and aliases are checked at load; ordinary source bodies
still require the remaining binding families. Startup diagnostics support common
states only. Scope and actual/controlled evidence:
[native_script_motions.md](native_script_motions.md).

The eight-actor live run completed 2,000 ticks per four source owners and delivered
92/154/54/101 idle accessory callbacks, with independently checked state hashes.
Its earlier event counters matched the preceding diagnostic run. The partial
census dropped from 1,388,376 to 864,088 bytes, exactly 524,288 bytes, when the two
confirmed unused 32 KiB animation archive buffers per actor were removed. Other
object/proc populations matched and topology errors were zero. This is a live
allocation saving, not complete mutable state or a snapshot speed measurement.

The subsequent real rollback benchmark reproduced all 64 complete endpoints after
15 replay ticks with the new motion IDs/variables included. Mean 168.31 ms still
misses 16.667 ms in all windows; broad capture remains in use. No authored DLL or C
compiler was used by either live source-only run. See the motion reference and
[native_fighter_snapshot_ownership.md](native_fighter_snapshot_ownership.md).
