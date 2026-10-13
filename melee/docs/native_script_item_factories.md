# Direct interpreted Item creation

`native_script_item_spawn.c/.h` resolves a prepared immutable spec by author and
source owner node. Physical kind and holdable recipe index remain data checks;
multiple source declarations may share a kind. The caller must have a current
GObj reference, captured scene epoch and actual live Fighter/Item interpreted
storage/program view. Factories use the public three-argument Item_CreateAuthored
constructor and substantive common native operations directly. Food uses the
ordinary Item_CreateEngine path. No authored C callback table or Fighter Host is
in the route.

The factory API returns NativeScriptVMStatus and publishes a native Item_GObj*
only through its final output. It never chooses a script handle representation.
Captured native generation and logical caller storage are checked after callback
boundaries. Missing/native child fault, invalid metadata and caller retirement are
distinct outcomes. A completed surviving creation side effect is preserved when
the caller retires; later setup stops and output remains untouched. Actual native
construction cleans failures before a viable Item exists. Owned child status also
stops setup after logical retirement even when GObj deletion is deferred.

Projectile metadata stores data-only appearance/state, immutable same-owner F32
parameter values or finite literals for speed/angle/lifetime, and lifetime/velocity/
update flags. Preparation validates animation bounds, state count, positive lifetime,
unique fields and owned parameter type before publishing. Attachment metadata
stores origin/previous-position, plane, initial collision and held registration.
A declaration without attachment metadata follows the original generic defaults:
bone origin and previous position, collision enabled, held registration enabled.

Projectile ordering is native construction/Spawned, velocity and launch payload,
lifetime/state, optional manual lifetime, appearance, creator provenance, Created,
debug, optional update, final manual lifetime and Launched. Optional explicit launch
preserves its native angle/facing/ignore-ID semantics. Presentation is a separate
route preserving the source sequence of initial state, appearance, attached state,
shared native follow operation, Created, debug and update. Created remains separate
from Spawned because payload/appearance setup occurs between them. Pickup uses
actual Item_8026AB54; source holdables use the real shared four-state pickup protocol.

Current bounded evidence is `A:/FreakMelee-test-archives/script-item-spawn-review/receipt.json`:
24 cases,3149 checks,1024 actual constructor/free cycles. Its README distinguishes
the actual native/VM pieces from controlled imported asset and engine leaf boundaries.
The preparation/publication receipt is
`A:/FreakMelee-test-archives/script-item-program-duplicate-final/receipt.json`:
42 cases,247 checks. No game launch, full render/physics parity, whole snapshot size
or timing result follows from these component fixtures.
