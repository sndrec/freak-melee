# Combined source Fighter event scheduler fixture

```powershell
python tests/native/script_fighter_scheduler/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-fighter-scheduler-review
```

Use a fresh output directory on A:. Compiler/console processes are hidden; logs,
commands, generated actual bodies and input hashes remain in the archive. Checks
fail unconditionally independently of NDEBUG. No game or global build is run.

The fixture links the actual compiler/metadata/program preparation, catalog,
Setup binding unit, VM, typed storage, registered GObjs, Fighter actor ownership
and packed callbacks. It extracts actual CPU capture/ranged, special-attribute
reload, landing, pickup/drop, visibility, knockback and accessory scheduler bodies.
Authored hooks and helpers execute in the actual VM.

Auxiliary-item checks extract both native slot-attachment paths, visibility
reset, their common continuations and the jab-entry motion-flag query. They
exercise all four slot-occupancy combinations, field assignment before callbacks,
source-only dispatch, absent hooks, detached authored/stock paths and callback
fault/removal before native continuation. Common item/sound/model leaves are
controlled; actual attribute reload and fighter lifetime validation run. These
checks establish scheduler contracts, not Pikachu/Pichu gameplay parity.

The checks cover:

- Actual idle-input and action-status bodies, preserving common input ordering
  and common-state shortcuts. Both Bool choices, every predecessor/successor
  stop position, absent hooks, faults and physical retirement are exercised.
  Installed native predicates and stock Fox fallbacks are excluded for source
  actors. Common input decisions are controlled ordinal leaves; this is not a
  full conversation gameplay or input-physics test. Unselected owned-layout,
  velocity-facing and ECB-unlock leaves abort if reached.

- Bool results versus execution status, normalized Bool flags, common-tail order,
  missing/invalid hook contracts and exclusion of installed native authored handlers.
- LandingAction's captured Bool payload for true and false entries, unchanged
  after a controlled position-write leaf overwrites the common motion union;
  missing and incorrectly typed hook arguments preserve the output sentinel.
- Target retirement, source fault/removal with unchanged output, detached running-slot
  protection and exact program references.
- Source-authored registration of all five motion phases through `callbacks.*`.
  Each phase runs through the actual actor/VM entry. The accessory helper additionally
  runs through the actual `Fighter_8006C80C` body for 256 every-frame and 256 cue frames.
- Clear/rebind from inside the running source helper, `None` clearing cue policy,
  native engine-pointer supersession and motion reselection restoring the immutable cue.
- Atomic registration rejection for parameter/result ABI, callback declarations,
  out-of-range IDs and cue on a non-accessory phase. Packed snapshots reject invalid
  helper/callback IDs and malformed cue policy without mutating the accepted actor.
- Packed state/helper/cue restoration, NewLife preserving authored state and motion
  registration while retiring the old generation, and actual freed/reused GObj
  resurrection through pool snapshot restoration followed by actor restoration.
- A reached motion helper's fault or removal stopping both its following authored
  statement and the native accessory scheduler tail.

Resurrection snapshots the actual GObj pool metadata/payload and restores the
fixture-owned Fighter peer and PLink head/tail roots before loading packed actor
state. The fixture creates no renderer or proc graph. Heap allocation occurs only
at this explicit test snapshot transition; production callback execution is unchanged.

This is ABI, scheduling and ownership evidence, not full Mario gameplay/asset
acceptance. Imported assets, graphics, collision, physical motion change, native
presentation and CPU playback are controlled leaves. The controlled position write
injects retirement/fault; the source decisions, catalog registration and actor
execution remain real. Unrelated binding families reject operations, and unrelated
Setup animation/item presentation leaves abort if reached. Root bone transform
comparison and full scene/rollback acceptance belong to their separate fixtures.

The contact-callback extension exercises named reflector-contact, completed
reflection-turn, body-contact and shield-contact events, helper assignment and
clearing, native callback takeover, shared motion reset, NewLife preservation,
and packed restore/cross-slot rejection. These tests run the real actor/VM/binding
code; they call the actor callback entry directly and do not simulate the full
hit-processing or reflector collision pipeline. The corresponding native call
sites are separately compiled in the client.

Callback identity coverage compares owned helpers/None, rejects incompatible
signatures and invalid slots, checks native takeover without mutating packed
state, and observes restored registrations after snapshots. The source/VM path
uses `callbacks.matches(Callback.reflect_hit, Pulse)`; the pure actor query is
also tested directly. It does not inspect native callback addresses in scripts.

Helper entry returns `NativeScriptVMStatus` directly. The grouped actor tests
distinguish a healthy Bool result, invalid/missing root helper or arguments,
fault during execution, repeated entry into a faulted actor, and physical
retirement/reuse during execution. Invalid/not-delivered calls return Error;
reached execution that retires returns SourceRetired. Both failures preserve
the unpublished result. Reached faults expose the VM diagnostic; delivery to an
already-faulted actor reports that recipient's faulted status, without copying
the shared VM's potentially unrelated latest diagnostic. Wrong argument
count is rejected without faulting a healthy destination. Snapshot restoration
then proves the actor and shared scratch remain usable. The existing owned
receiver helper contract remains Bool and explicitly accepts only status Ok;
its expected-program rejection and healthy route execute in this fixture.

The helper retirement/fault triggers use the same controlled position-write
leaf described above. These are real actor/Execution/VM status and publication
checks, not dynamic recipient-export lookup or gameplay acceptance.
