# Typed mutable motion callbacks

The generic source APIs take one owned root helper or `None`, returning Void:
`callbacks.animation`, `interrupt`, `physics`, `collision`, `accessory` and
`accessory_cue`. The five ordinary methods bind their corresponding motion
phase. `accessory` runs every frame; `accessory_cue` binds the same phase with
cue gating. None sends UINT32_MAX to clear; the actor normalizes cleared cue
state. No arbitrary integer, native pointer or foreign receiver callable is a
code reference.

The load-time CodeRef contract is shared with existing instance callbacks:
only Null or an actual helper Name in NATIVE_SCRIPT_FUNCTION_SEMANTIC is
accepted. Local, receiver/member and computed Callable values reject before
bytecode publication. The actor independently checks a live current program,
root ownership, noncallback identity and zero-argument Void ABI.

Catalog dispatch routes these appended IDs explicitly into actual setup
invocation before broad enum-family routing. Setup calls the substantive
`nativeScriptFighter_BindMotionPhase`; no SourceFighterLayout dependency is
introduced, so attached interpreted actors can use raw stock assets.
Retirement or rejected actor/ABI leaves the caller result unchanged.

Run `run.py --compiler <MSVC-target clang.exe> --output <fresh A: folder>`.
The final scoped receipt is
`A:/FreakMelee-test-archives/script-motion-callback-bindings-oct08-guard-final/receipt.json`:
**94 checks** cover actual catalog, Program/statement/IR, bytecode, top-level
Invoke and setup dispatch. The controlled actor leaf checks real indexed helper
metadata and records phase/cue, while cases reject numeric/text references,
oversized IDs, foreign ownership, wrong ABI and missing actor, and suppress
results after retirement. Existing callback CodeRef mode receives the same
foreign/local/computed-value negatives.

The shared presentation fixture is copied privately with its explicitly absent
BindMotionPhase leaf removed, because this test provides its controlled owned
actor leaf. Native presentation remains unrelated/controlled. Actual actor
phase installation, self-clear/rebind, cue scheduling, snapshots and resurrection
are verified by the separate scheduler fixture. No scene/game acceptance is
claimed here.
