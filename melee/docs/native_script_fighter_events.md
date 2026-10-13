# Direct Fighter event ABI and scheduler

Static source events use `nativeScriptFighter_Run(object, hook, arguments, count,
result)`, returning `NativeScriptVMStatus`. CPU capture has a Bool result; item
pickup/drop and their extended forms receive Bool `event.flag`; the other migrated
events return Void. An absent static hook produces its typed neutral value. Error
and SourceRetired stop native continuation and leave caller output untouched.
Execution status is never treated as an authored Bool decision.

The actual CPU capture/ranged, special-attribute reload, landing, item pickup/drop,
visibility and knockback sites select source code IDs for attached actors.
Unscripted fighters retain their existing engine selection. Source ownership,
rather than presence of an individual hook, prevents a missing source hook from
falling into a native authored callback. Existing load/death/tick/late-update and
special-entry callers use the same typed Run contract directly.

CPU capture true stops capture processing. False continues the shared capture
tail while skipping fighter-specific native capture/pummel handlers. Victim
identity remains checked after source execution. Ranged source execution retains
the shared pre-candidate ordering and replaces the native kind-specific candidates.
Item flags normalize the actual native argument with `!=0`. Pickup/drop bookkeeping
and visibility updates occur only after successful source execution and source/
retained-item checks. Landing updates its common motion/allow-interrupt state,
then runs the source landing event without a native kind-specific cleanup fallback.

## Motion cue and callback identity

`neutral_special_cue` remains a named move Accessory event for both prepared
neutral-special states. Its immutable patch records the cue trigger. The actual
accessory scheduler interprets its code only when `throw_flags_b0` is set, consuming
that bit before execution, matching the source fireball cue primitive. Ordinary
Accessory events still run per frame. `nativeScriptFighter_BindMotionPhase`
binds a root-owned zero-argument Void helper, or clears a phase with UINT32_MAX.
Accessory clearing also clears the trigger marker. A new motion selection restores
its declared phases. The [motion callback reference](scripting/motion_callbacks.md)
describes the direct source methods and explicit cue/every-frame policy.

Packed actor records retain the cue marker and selected callback IDs. Validation
rejects an inconsistent cue/function pair and callback functions with the wrong
owner/signature/slot. Zero-argument Void authored helpers remain the mechanism for
damage/death callback behavior; no native ResetRootX policy or generated authored
callback is introduced. Mario motion slots341/342 are preserved as the actual
explicit `ftCo_SM_None`/NULL placeholders, not invented taunt behavior.

## Ownership and preparation

Actors retain actual pool slot/generation/scene references. Status distinguishes
live faults from retired source identity/storage. A running reservation prevents
detach/reentry from reusing the suspended actor's state buffer. Program references
remain pinned for active or running actors; `ReferencesProgram(bank)` checks actual
execution/storage program pointers, independently of current kind selection.
Snapshots require idle execution and reconstruct those native references from
validated packed identities.

Fighter program preparation accepts the owned numeric `NativeScriptMoveLayout`
instead of a callback-bearing NativeFighterDefinition. Common/first/count and
named ground/air/transition flags must match source lowering. CLI and test callers
supply that explicit layout. Effect-owned metadata/programs are prepared by the
separate effect subsystem, whose active owners also prevent publication reuse.

## Evidence

`tests/native/script_fighter_scheduler` executes production-compiled source
events/helpers through real registered GObjs, Fighter/Item actors, the VM and
typed owned storage. Actual scheduler bodies are mechanically extracted. The
145-check fixture covers typed flags/results, CPU false/true continuation, absence
and invalid entry contracts, retained targets, source errors/retirement, reentrant
slot reuse protection, native-callback exclusion, cue pulse/clear behavior, helper
callback IDs and packed identity rejection/restoration.

Its source hooks deliberately record ordering and decisions; this is scheduler/
ABI evidence rather than full Mario gameplay parity. Unrelated graphics, geometry,
CPU script playback and physical motion leaves are controlled. Full template
bindings, prepared retail assets/factories/effects and game acceptance are separate
combined requirements; this console fixture does not launch a game or prove an
entire rollback aggregate.
