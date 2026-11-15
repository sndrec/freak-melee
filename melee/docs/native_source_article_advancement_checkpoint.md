# Source Article advancement, ownership and world feedback

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The interpreter now advances Item animation in its native order: evaluate the
primary model, copy its current frame, then execute the DATA command stream.
Source/root guards separate these stages. Model-only advancement evaluates the
graph without changing Item frame or DATA state. Representative native damage
and radius commands execute; this is not a fake frame-counter update.

Creation values now belong to the mutable source actor and its packed snapshot.
Attach initializes zero. Factories install the finite scalar after Spawned and
before Created. Source factories no longer write that scalar into overlapping
Article/Holdable union storage. Getter failures preserve output, and malformed
snapshot scalars reject transactionally. This is a component state improvement,
not completion of the explicit whole-match snapshot inventory.

`item.attachment_active` uses revocable attachment control and current damage
owner, with missing initial relations returning false and captured retirement
withholding output. Its former factory implementation incorrectly compared the
immutable creator. The misleading `current_owner_is_creator` spelling was
removed from tracked templates and the test-only C emitter; no compatibility
alias is introduced. `item.current_owner` independently returns a full-U32 live
Fighter handle or zero for absent/non-Fighter/invalid initial ownership.

Generic owner-debug inheritance preserves the actual byte-mask branches for
null, non-Fighter and Fighter owners. Both Bool stage-collision algorithms and
the zero-argument true default execute substantive native map-collision bodies.
The old Item-self dispatch case was removed. Its engine-free fixture now asserts
rejection; the connected Article-motion fixture proves valid owned dispatch.

`item.rumble` uses current damage ownership and the native eligibility/pattern
path. The binding reads the actual imported `lbRumblePatternCount`, rather than
guessing a pattern maximum. Native integral casts and duration-zero mapping to
-2 remain. Eligible invalid pattern/controller/player inputs diagnose before
native assertions or indexing; ineligible/null/non-Fighter ownership still follows
the native no-op path. Source and foreign owner identities remain captured.

`camera.quake` supports the four defined native kinds in Fighter and Item
contexts. Actual camera bookkeeping, Stage GObj creation/proc setup, loop reuse
and native lifecycle remain shared engine behavior. Undefined kind zero/outside
the enum is rejected. Raw position bits are retained; there is no invented
coordinate clamping or fighter-specific quake policy.

## Evidence and remaining work

Receipts under `A:/FreakMelee-test-archives/`:

- `script-item-advancement-settled`: 398 checks, 32 native comparisons, nine
  prepared selectors, four frame-request and three advancement retirement stages.
  Real compiler/VM, DATA interpreter/control operations and damage/radius bodies;
  actor publication, HSD animation and owner damage adjustment are controlled.
- `script-article-motion-current-owner-head`: 6315 checks, 3078 native
  comparisons. Actual compiler/VM/actors/pool, native debug/getter and both map
  bodies. Map geometry/debug globals and guard observations are controlled.
  Includes direct U32 and VM owner-handle reads with high-bit values.
- `script-world-feedback-head`: 257 checks, 32 native comparisons, 44 quake
  GObjs and 36 hardware-queue requests. Actual camera/Stage constructors/proc
  scheduler and Item/ftLib/Player/lb rumble chain. Quake models/HSD animation,
  pattern/player data and the final hardware queue are controlled.
- `script-item-actor-motion-naming-refresh`: 1219 checks/38 cases.
- `script-child-motion-naming-refresh`: 18937 checks/27 cases/1024 reuse cycles;
  real callbacks observe Spawned zero then Created 7.125 through the actor getter.
- `script-spawn-attachment-motion-refresh`: 166 checks/22 cases; VM attachment
  activity becomes false after control clear while original creator remains.
- `script-positional-motion-naming-refresh`: 11629 checks/27 cases/1024 reuse.
- `script-item-factories-motion-naming-refresh`: 3150 checks/24 cases.

Client/content tool builds and 36 selected CTests pass. Actual owned Zelda
source-check now compiles all bodies and stops in preparation at line 421:
`cpu_pending_action` has no migrated scheduler for its argument/return contract.
Unsupported hook diagnostics now name the event and source span. This is not
successful whole-form activation. Sheik remains at Chain metadata line 1543.

DATA import already validates opcode widths and branch offsets but still lacks
hitbox-index, bone-index and loop-stack operand admission. Primary animation
bundles, secondary models, source-owned Chain resources, remaining typed Fighter
scheduler hooks and complete roster/match acceptance remain open. No game,
installed content or user draft was updated. The full project goal remains active.
