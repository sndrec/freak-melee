# Owned Peach articles through the interpreter

2026-10-09. Peach now runs as an installed source package with its five copied
primary Article descriptors and direct FMS callbacks. This adds constructor,
state/animation and cleanup evidence to preparation. Historical generated-C
stock comparisons are not new interpreter parity claims.

## Translation and continuation fixes

Toad's native expiry predicate reads `Item.owner`. The script incorrectly used
`holder.motion`, whose generic contract also requires that the fighter's held
slot contain this item. An attachment-only Toad does not meet that contract.
Both Toad states now use `current_owner`, preserving native null-owner expiry.
State 0 notifies a present owner when it leaves its counter motions; state 1
leaves notification to destruction. No receiver API was weakened.

The first replay faulted Toad, its fighter cleanup, then a Turnip destruction
helper delivered to the already-faulted fighter. That helper copied a diagnostic
from the shared VM, which another invocation had cleared. Failed delivery now
reports the faulted recipient explicitly. The actor regression faults an actual
compiled actor and checks delivery after clearing and replacing the shared
diagnostic, preserving output and native-write sentinels.

The failure also reached common capture entry and called a null `coll_cb`:
source callbacks are VM identities, and the prior fault prevented the requested
motion transition. The continuation now retains both actual fighter lifetimes
around motion setup, propagates an unsuccessful continuation, and dispatches a
bound collision phase through the interpreter. It invokes a native callback
only when present.

This is not complete scene fault isolation. Replaying the old broken package
with these guards passes the original crash frame, then fails later in
`ftCo_800DCFD4` with a null capture peer. Partially completed capture state must
not be treated as healthy gameplay after an authored fault. The negative case
is retained at
`A:/FreakMelee-test-archives/source-peach-live-oct09-root/faulted-capture-regression/`.
The accepted canonical replay has no script faults.

## Actual construction and lifecycle cohort

`tests/native/source_peach_articles/` uses the real runtime library, private
copied package/cache, imported `PlCo.dat`/`ItCo.dat`, fixed pools, published
programs, Item constructors, HSD models and animation owners. The parent is an
explicitly initialized fixed-pool Fighter payload and real script actor, not
a full `Fighter_Init` or rendered match.

All five types are covered: Explode (98), Turnip (99), Parasol (103), Toad (104)
and ToadSpore (111). Declared null roots use actual anchor construction. All 13
article states select their animation/DATA owners and execute their canonical
animation callback. Additional created/held hooks, native destruction, actor
detach, allocation cleanup and package release are covered: 202 checks, five
constructors/removals, 13 states and 18 explicit VM calls. The Item component
snapshot is 1,306 bytes and does not establish complete mutable graph rewind.
Effect-producing Explode/Spore Created callbacks are outside this console
cohort. The native scene replay provides separate gameplay coverage.
The final receipt is
`A:/FreakMelee-test-archives/source-peach-articles-final2/receipt.json`;
root rechecked all 3,045 input fingerprints, including 47 runtime DLLs.

## Actual match

The committed `live.py` runner starts one replay-controlled Peach and three CPU
Peaches on Final Destination. Drawing is retained; the process is hidden/muted
with a private installed content root. Binary/cache/input hashes are recorded;
acceptance requires 2,400 samples, four nonfaulted actors and no script errors.
It sends no OS input.

The final run passes at
`A:/FreakMelee-test-archives/source-peach-live-oct09-final/receipt.json`.
Actual gameplay reaches kinds 98, 99, 103 and 104. Spore construction/state
behavior belongs to the separate constructor cohort; this replay does not
claim a successful Toad counter or spore hit.

Preparation contains 261 functions, 67 imports and 4,256 instructions.
Client/content-tool builds and fighter/receiver CTests pass. User drafts and
installed Collection content were not changed. Pixel fidelity, exact stock
trace parity, every combat/rare-item branch and full rollback remain open.

## Next resource boundary

Link/Young Link need independent secondary roots and Boomerang animation
bundles. Sheik and Ice Climbers additionally need resource collections.
Metadata remains rejected until its corresponding runtime owner is ready.
The [secondary resource plan](native_source_secondary_resource_plan.md) covers
indexed imports, renderer/animation demand, replacement, cleanup and history.
