# Owned source article lifecycle

Source Items now retain two actor-owned relations: immutable original creator
and revocable attachment/control owner. Each stores full physical pool/scene/
slot generation, Fighter native generation/kind and userdata identity. Actor
snapshots save both explicitly. A stale relation is valid saved data, resolves
absent and cannot become live again when its Fighter slot is reused.

The actual constructor captures original provenance before Attach/Spawned,
including source Fighter creation without damage ownership. Item children copy
the parent's revocable link separately from original creator. Source factories
no longer write legacy union attachment pointers/creator IDs or recapture the
original creator after callbacks. Existing facing/launch/creation bookkeeping
is unaffected. Child creation after a cleared parent retains original provenance
while its attachment link remains absent.

The generic `attachment_owner` receiver uses the same bounded invocation pool
and typed Fighter projections as other relations. `creator` remains original
provenance. Template controller accesses must explicitly select revocable control
where old legacy creator access disappeared after detach; this is not a blanket
replacement of provenance queries.

The current template pass covers Zelda Din control and Sheik Chain control.
Typed helper calls and explicit missing-link zero/no-op defaults retain their
old cleanup decisions. Other roster revocation-dependent sites remain audited
follow-ups; complete all-template lifetime parity is not claimed by this slice.
Scope/default evidence is in [the template audit](native_source_attachment_template_audit.md).

## Direct operations

- `item.clear_current_owner()` clears only damage ownership.
- `item.clear_attachment_owner()` revokes only the actor-owned control link.
- `item.detach_owner()` does those two clears in that order. It does not invoke
  physical release, change `owner_bound`, or clear Fighter held/auxiliary slots.
- `item.attach_to_owner(raw_part[,register_auxiliary])` requires a live source
  attachment relation equal to current damage owner. Missing/nonmatching
  ownership and ordinary invalid raw parts are healthy no-ops. Nonintegral/wide
  arguments and absent required rig/model data fault before mutation. Uses the
  actual `Item_8026AB54` pickup path once; optional auxiliary registration occurs
  after callback reentry only if captured identities/programs remain healthy.
- `articles.bind_auxiliary(handle)` requires the exact same-bank source Item
  originally created by this Fighter. Reflection does not revoke creation
  identity. Missing/foreign Items return false; matched faulted Items fault.

Existing `item.owner_bound` is still the real native attachment flag. No fake
Definition, extra script state slot or authored native handler was introduced.

## Evidence and limits

Private receipts under `A:/FreakMelee-test-archives/`:

- `script-article-lifecycle-catalog-oct08-root-final/receipt.json`:111 typed
  source/index/VM/top-dispatch checks, attach arities, full-width integral
  marshalling, scope/type rejection and role5 projections. Native lifecycle
  leaves explicitly reject; no connected actor behavior is inferred.
- `script-article-lifecycle-oct08-root-final/receipt.json`:321 connected checks,
  16 direct native pickup comparisons and42 interpreted pickups. Actual source
  compiler/VM, actors, relations, snapshots, pools and pickup bodies execute.
  Imported rig/package and rendering/audio/collision/transform leaves remain
  controlled; no DAT constructor/game acceptance.
- `script-article-relation-factory-oct08-root-settled/receipt.json`:11624 checks/
  27cases/1024 reuse cycles using actual common/public Item constructors and
  original creator functions. Includes reflected damage owner, absent original
  creator, cleared-parent link inheritance, callback retirement and allocator
  failure. Asset selection/model/physics initialization leaves are controlled.
- `script-article-relation-bindings-oct08-root-final/receipt.json`:153 checks/
  19cases through source factory bindings and native constructors.
- `script-article-lifecycle-receiver-oct08-root-final/receipt.json`:4352 existing
  receiver/core checks with9 execution runs. Pure core fixture relation getters
  supply its controlled views; connected actor relation proof is separate above.
- `attachment-template-excerpts-oct08-root/receipt.json`:actual Zelda root helper
  bodies plus new attachment projections/typed calls pass owned-data source-check,
  4functions/94instructions. The probe body tests types/imports, not full move
  behavior. Sheik's resource bodies remain behind their full metadata gate.

The actor CTest adds clear/load, stale native generation, same-slot pool reuse
and malformed relation snapshot checks:1206 checks/33cases,519 frees. The scheduler
fixture's no-creator leaf now initializes fields on poisoned memory as the real
capture function does. This repaired its fixture setup, not production behavior.
Client/content tool and36 selected CTests pass. Full source forms still require
CPU queries, remaining Item operations and Chain resources. No game or installed
content update occurred; complete gameplay/rollback acceptance remains open.
