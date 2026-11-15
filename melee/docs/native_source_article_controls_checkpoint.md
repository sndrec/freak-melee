# Interpreted Article models, capsules and child creation

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Primary-model joint selectors now retain their literal model/path recipe at
package preparation. Model index zero and bounded child-ordinal paths address
the actual Item root graph; secondary models are not aliases to that graph.
Visibility, existence, Euler rotation and scale use the shared native model
primitives. Uniform root scale and animation-frame operations preserve their
different native contracts. Animation updates check the source and root after
each rate/request/evaluate/frame-read stage.

Selectors use a fixed aggregate pool of 2048 records in the prepared Item
programs, with at most 128 per declaration. Copy/rebind adjusts each declaration's
pointer into that immutable pool. There is no tick-time path parsing or allocation.
The first inline-per-declaration layout caused a verified Windows stack overflow.
Pooling removed that duplication; CDB then located the remaining overflow in
Effect preparation beneath the caller and Fighter preparation candidate. The
transactional Fighter candidate now uses temporary allocation only during
package loading and frees it on every success/failure path. Published outputs
remain unchanged on failure. No linker stack-size increase or test-only bypass.

Article capsules use Item storage, not Fighter layouts. Generic indexed
enabled/radius/damage methods, single/all disable, and the four stage-contact
properties execute through the actual native fields and setters. Damage keeps
current-owner scaling and stale-move calculations. Undefined floating-to-U32
conversions diagnose before mutation; defined nonfinite cases are not broadly
sanitized. Geometry checks prove required attachment-joint presence, not graph
ownership or allocation lifetime.

Item-source positional and child factories resolve the declared Article at
load. Child creation preserves original creator, optional attachment control,
explicit damage owner, full-word collision group, and backlink timing. Creation
value/backlink install after Spawned and before Created. Already-stale optional
control remains absent; same-slot replacements are never adopted. Retirement
after live creation withholds output while preserving completed side effects.
A constructor NULL result also includes Spawned removal/fault cleanup and yields
-1 with a live caller, matching the existing positional constructor boundary.

## Evidence and limits

Private receipts under `A:/FreakMelee-test-archives/`:

- `script-child-article-spawn-frozen/receipt.json`: 16826 checks, 27 cases,
  1024 reuse cycles, both public Item factories through the actual compiler/VM,
  native constructors, actors, pool, snapshots and full-U32 target lookup.
  Model/platform/audio/asset selection leaves are controlled.
- `script-article-combat-oct08-root-final/receipt.json`: 262592 checks and 84 native
  comparisons, including exhaustive contact masks, native scaling/stale math,
  both disable operations and capsule geometry updates. Geometry transforms and
  publication data are controlled; mutation is an explicit completed-setter
  observation, not a claim of native authored callbacks.
- `script-spawn-child-oct08-root-final/receipt.json`: 161 checks/21 cases.
- `script-positional-child-oct08-root-final/receipt.json`: 11624 checks/27 cases,
  including 1024 reuse cycles for the earlier constructor family.
- `script-item-model-freeze/receipt.json`: 194 checks, 16 native comparisons,
  nine prepared selectors and all four animation-stage retirement boundaries.
  Actual source/bytecode/VM, descriptor validation, GObj pool/registry,
  nativeModelJoint and HSD flags/class ancestry execute. Actor publication and
  animation-stage leaves are controlled; no native animation math or Item
  scheduler/scene acceptance is claimed by this fixture.

Client/content tool builds and 36 selected CTests pass after the storage repair.
Zelda's actual owned-data source-check advances from Article line 786 to line
802, `item.inherit_owner_debug_display`. Sheik remains at Chain metadata line
1543. A first-error source-check is not whole-body or model-runtime acceptance.

No game, installed content or user draft update occurred. Primary model extras,
animation bundles, resource collections, remaining Item primitives and complete
paired/roster runtime acceptance remain open. The full project goal is active.
