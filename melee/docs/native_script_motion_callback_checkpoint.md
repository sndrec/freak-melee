# Interpreted motion helper callbacks (2026-10-08)

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Scripts can register their own named helpers for animation, interrupt, physics,
collision and accessory phases. Accessory has explicit every-frame and
subaction-cue policies. The actual actor validates current status and requires
a root-owned, zero-argument Void helper. None clears the phase and its native
slot. There is no native function thunk, parsing, allocation or fighter-specific
registry during registration/execution.

The compiler's CodeRef arguments accept a same-program helper name or None;
foreign receiver/computed callable values cannot be flattened into local IDs.
This ownership rule also applies to existing instance callback registration.
The actor separately checks the selected declaration's owner and signature.

Motion selection restores that motion's declared phases. Existing shared native
slot assignments supersede a script binding. Helpers can clear or rebind
themselves without discarding their new choice after return. NewLife preserves
persistent variables and phase IDs, matching the native reset's sequencing;
later motion selection resets them. Helpers still require the current native
generation and live source owner before executing.

Snapshots retain helper IDs, cue policy and exact program/schema identities.
Root helpers are validated directly; immutable event callbacks must still match
their prepared phase and cue, so helper snapshots avoid scanning the motion
patch table. Wrong signatures, foreign program identities and inconsistent
cue/function pairs reject before restoration. The native Fighter/GObj peers
must be restored separately; this does not claim a complete game savestate.

The former ClearMotionPhase entry was removed; production clearing and fixtures
call BindMotionPhase directly. See [the scripting reference](scripting/motion_callbacks.md)
for the user-facing methods. Actual actor and scheduler tests record helper
registration, reset, supersession, cue/every-frame execution, self-clear/rebind,
snapshot restoration, pool resurrection and retirement/fault boundaries.

Evidence under `A:/FreakMelee-test-archives/`:

- `script-motion-callback-bindings-oct08-guard-final/receipt.json`: 94 checks
  of the actual catalog, owned CodeRef guard, compiler/bytecode and setup
  dispatch; the actor registration leaf is controlled.
- `script-fighter-motion-final/receipt.json`: 2,404 checks using actual source
  compilation, VM/actor state, authored registrations and the accessory scheduler
  body, including 512-frame cue/every-frame runs and actual GObj pool resurrection.
  Asset, graphics, physical motion and native presentation leaves are controlled.
- The direct actor fixture passes 307 callback/lifetime/motion/snapshot checks.
  The client/content tool rebuild and 35 selected regression CTests pass;
  `.local/source-motion-callback-ctest.log` records the selected suite.

No game was launched for this batch; these are engine/VM/scheduler checks, not
rendered full-member gameplay or complete aggregate snapshot acceptance.

Complete Zelda/Sheik source preflight still stops at obsolete `base` metadata
before full behavior validation. The audit receipt is
`A:/FreakMelee-test-archives/source-form-template-preflight-oct08-root/receipt.json`;
owned draft inputs remain unchanged. The [primitive closure audit](native_source_form_primitive_closures.md)
specifies raw-joint effects, copied-position wind and flag/rate-preserving entry.
Those operations and complete-member entity resources still need implementation
and actual paired gameplay checks. Snapshot completeness/performance remains
separate project work; no user installation was changed.
