# Source-owned Workshop holdable protocol

`native_holdable_protocol.c` implements the shared held/resting/flying/dropped
mechanics for interpreted Item recipes. Recipe data comes from the immutable
prepared `NativeScriptItemProgram`; authored thrown/tick/hit/ground_collision
hooks execute directly through the Item actor and VM. Source recipes contain
no native callback function pointers.

The protocol preserves the actual `native_holdable.c` state table behavior:

| Operation | Shared behavior |
| --- | --- |
| PickedUp | Enter held state0; select appearance animation0; advance animation. Age is preserved. |
| Dropped | Reset u16 age; enter state3 with animation/drop update flags; select appearance0. |
| Thrown | Reset age; enter state2; select appearance0; prepare actual hitcaps from recipe size, damage, angle and knockback; then execute thrown. |
| Animation | Held state0 is neutral. Other states increment u16 age, execute tick, then test nonzero recipe lifetime. |
| Physics | Flying/dropped states2/3 use shared falling/spin physics only when no authored tick exists. |
| Collision | Held/resting states0/1 are neutral with no geometry or authored hook. States2/3 store contact flags; an authored ground_collision owns its Bool. Otherwise contacts0xE run ground response and enter state1; remaining contacts3 request removal. |
| Hit | Authored hit owns the Bool. Without it, bounce from the victim and enter state1; return false. |
| Clanked / Reflected / ShieldBounced | Preserve actual shared victim bounce / reflect / shield-vector mirror behavior and return false. |

Custom tick presence suppresses default gravity, irrespective of whether that
particular tick changes velocity. Physics does not execute tick a second time.
The age field keeps its actual u16 wrapping behavior. True results are removal
requests for the native scheduler; the protocol does not synthesize true on error.

## Identity, errors and cleanup

`nativeScriptItem_Status(object)` returns Ok only for a current logical storage
view, current prepared spec/source identity and nonfaulted actor. A retired view
returns SourceRetired; a live faulted actor returns Error. This is distinct from
`Spec`, which remains accessible while the destructor retains its cleanup pin.

Each protocol entry checks that status before native mutation. Its continuation
guard retains the actual GObj pool slot/generation/scene lifetime and Item creation
generation, and rechecks logical status after lifecycle-capable native stages.
This prevents a physically live deferred-removal object or a recycled source from
resuming native field access. Error/SourceRetired leaves the caller output
untouched. Ordinary successful state transitions retain their native behavior.
No guard allocates memory or resolves a source name during execution.

Destroyed remains responsible for completing writable source cleanup and retiring
the logical view. Detach remains responsible for final userdata release. The
protocol does not change this ordering or make the cleanup spec disappear early.

## Comparison fixture

`tests/native/script_holdable_protocol` prepares real immutable recipes with the
production source compiler and metadata pass, creates real registered GObjs,
attaches actual Item actors and executes authored hooks in the shared VM.
Mechanically extracted legacy holdable statics provide the no-authored-hook
baseline. Actual common Item state/removal, hitcaps, gravity/spin, victim bounce,
reflect, shield mirror and facing functions run on both sides.

The complete console fixture passes5802 checks in18 cases, including1024
comparisons with actual native bodies. It includes queued-removal and live-fault
continuation regressions plus faulted destruction becoming logically retired.
It also prepares real no-hook/no-state Item and Article declarations; the general
storage owner index records those explicit source owners with zero-byte rows.

The fixture separates generic native parity from authored hook decisions. It does
not implement authored hooks as native test callbacks. Graphics/assets, scene
contact observation/ground-response leaves and selected position-write reentry
triggers are controlled and recorded in its README/receipt. A native caller can
retire while an independent child hook completes; this fixture checks both actual
identities, while authored Fighter factory caller guards belong to the connected
factory/recipient acceptance suite. No game scene, full aggregate rollback or
retail asset acceptance is claimed by this console fixture.
