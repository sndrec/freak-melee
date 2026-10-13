# Rook interpreter gameplay checkpoint (2026-10-07)

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Rook's canonical source now owns her air-jump entry and state 341's animation,
input, physics and collision phases. Generic engine primitives provide the
native jump bookkeeping, full aerial-jump input sequence, root rotation,
landing threshold and walljump check. Both native jump-entry routes dispatch
prepared source hook IDs directly; errors or retirement cannot fall through to
fighter-specific native callbacks. Lifecycle guards stop continuation after
motion/control callbacks retire an actor.

Her actual DAT allows two total jumps and supplies one air jump with vertical
impulse 3, horizontal impulse 1 and animation 295. The source imports the raw
turn/count data without normalization. Shared common drift is retained rather
than substituting Purin's F1 physics. Animation-driven jump mode is explicitly
rejected before mutation until its source-owned physics policy is implemented.

## Evidence

All receipt paths below are under `A:/FreakMelee-test-archives/`.

| Evidence | Result and scope |
| --- | --- |
| `script-jump-final/receipt.json` | 121 VM/native jump comparisons; asset/controller, motion/audio and geometry leaves controlled. |
| `source-jump-catalog-oct07-second/receipt.json` | 39 checks of typed descriptors and bytecode mapping. |
| `script-rook-items-final/receipt.json` | 997 checks, 65 cases, nine exact recipe declarations and 57 actual userdata removals; imported model/geometry/Food asset leaves controlled. |
| `source-rook-jump-preflight-root/receipt.json` | Complete Rook source prepares 57 functions, five raw attribute imports and 1366 instructions; original user project inputs unchanged. |
| `source-rook-jump-package-root/receipt.json` | Actual source package build, verification and acceptance into a private cache. |
| `source-rook-jump-snack-live-root/jump/receipt.json` | Hidden/muted actual client: 600 samples, exactly one state-341 jump using animation 295, no further eligible jump, no VM faults/nonfinite traced fields. |
| `source-rook-jump-snack-live-root/snacks/receipt.json` | Hidden/muted actual client: 1900 samples, pulls and throws, held/thrown/resting source Items, a hit increasing Mario's damage to 19.97, no VM faults/nonfinite traced fields. |

The final client/content-tool and the focused test targets rebuild. The selected
35 CTests for script/entity/storage/ownership/content/held behavior pass;
`.local/source-jump-batch-ctest.log` is the local log. This is a selected suite,
not a claim that every historical test target was run.

The complete UI framebuffer was captured in two real-time hidden/muted runs.
The small held icon is visually ambiguous, so these images do not establish
billboard rendering acceptance. That gate remains open, along with exhaustive
live recipe/contact behavior and interactive Workshop use of the migrated Rook
project. The user's original installed Rook was deliberately left untouched;
all gameplay here uses an independent source package/private cache.

See `native_rook_jump_closure.md` and `native_rook_item_protocol_audit.md` for
the underlying contracts and limits. Paired source fighters and complete
explicit mutable rollback snapshots remain project work. Generated C remains
only in the separate test-reference exporter, never the ordinary package/client
execution route.
