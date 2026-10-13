# Interpreted model-effect ownership

Source `model_effect Name gfx_id` declarations prepare an immutable recipe with
author, source owner node, program identity, actual native constructor family and
member count, and optional Void/no-argument update code ID. Preparation validates
the actual efSync_ModelRecipe, owned state size, update ABI and duplicate names
before changing output. Empty data-only owners use an explicit `pass` declaration;
no dummy state is required. Helpers and typed state/parameters share the real
source program/storage contract. Root publication embeds recipes and pins against
live Fighter, Item and Effect owners.

The new effect actor owner reserves 512 stable group slots with 8,192 state bytes
each and a fixed GObj-slot-to-group index. This is a provisional explicit bound,
not a census-derived capacity guarantee. Group members retain separate native
GObj/EF/model payloads. The interpreted owner makes no allocation during create,
update, detach or packed save/load. Existing native EF/JObj/animation constructor
allocation is a separate ownership problem; this change does not call that entire
path allocation-free or migrate those owners.

SpawnRoot captures the current Fighter GObj pool reference, native generation,
scene epoch, actual interpreted root view/schema/parameters and model origin.
After actual native construction, it validates the complete returned group and
binds one typed source storage/execution view. Every native member's authored
update pointer stays NULL. The actual efLib_Update retains native pause, lifetime,
scale and animation handling, then executes the prepared code ID for member zero;
other members still execute their complete native animation tick. Group/creator /
origin/generation/schema validation prevents source continuation after replacement
or retirement. Existing generation resolution uses the fixed source-owner index;
legacy native effects retain their original registry route.

A completed surviving native group remains when the caller retires during
construction. Source setup stops, the group stays coherently owned/pinned for
cleanup, and its next native tick cancels it. An incomplete returned group is
removed through normal native teardown. Removal retires source storage as soon as
any member reaches userdata cleanup and retains its program pin until every
member reaches final cleanup. Deferred self-cancellation stops the VM's subsequent
writes even before physical GObj deletion completes. Scene retirement occurs at
the coordinated quiescent GObj boundary before native heap/assets are reclaimed;
it does not run callbacks/destructors again.

`effects.spawn_root(Name)` resolves source owner identity once during compilation.
`effect.joint(path[, member]).visible` and `.rotation.z` prepare bounded child
ordinals (12 levels, 0..255) and exact member ordinals; no text parsing occurs in
ticks. Runtime uses the current owned effect/group identity and prepared selectors,
then the substantive native model-joint operations. Missing paths retain the
audited primitive defaults/no-op; stale source/group/member identities fail. Creator
properties, owned state and helpers use real scoped retained receivers, not a
borrowed Fighter Host or an authored native callback.

Private packed records contain only occupied group metadata and declared state:
creator/member GObj references, native generations, program/layout/parameter and
creator schema identities, origin pointer, counters/fault status. Validation checks
record order/duplicates, sizes, Bool state, immutable identities and epoch. Load
preflights every native EF/GObj/creator peer and execution bind before mutation.
It then rebinds typed views without defaults, callbacks or allocations. Aggregate
restoration must first restore GObj/proc/table metadata, native EF/model/animation
payloads and the creator Fighter/interpreted owner. Immutable model assets and
programs remain resident and pinned. This component does not restore the whole EF
subsystem or match.

The connected bounded fixture runs actual source to native scheduler with explicit
controlled asset/platform boundaries; see `tests/native/script_effect/README.md`.
Its final receipt is `A:/FreakMelee-test-archives/script-effect-final/receipt.json`:
25 cases / 5,201 checks, 1,024 repeated lifecycles and 512-group exhaustion.
The reservation is 4,395,008 bytes plus an 8,192-byte index; the 512 empty-state
occupied groups pack 114,704 bytes. These describe this component, not a complete
match snapshot or timing result.
The native Tornado reference still reads its original overlaid integer move flag;
the comparison fixture supplies those bytes with memcpy. Interpreted source uses
its declared Bool state and preserves the observed branch results without requiring
the old aliasing representation. No game was launched by this agent.
