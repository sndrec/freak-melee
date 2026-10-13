# Native ItemLink ownership and packed snapshots

2026-10-11. Stock and interpreted chains/tethers now allocate their actual
`ItemLink` records from one scene-owned `nativeItemLinkPool`. This is one family
in the explicit snapshot migration, not a complete match serializer or a timing
result. The production broad-page snapshot remains the oracle.

## Actual allocation and retirement

The five stock constructors (Link hookshot, Ness yo-yo, Ice Climbers string,
Samus grapple and Sheik chain) call `nativeMatchPool_Allocate` directly. Their
existing GObj/graph initialization and publication order is retained. The shared
userdata destructor `it_802A2474` resolves and retires the actual occupied slot.
Allocation failure now unwinds the unpublished GObj and earlier chain nodes;
unwind loops retain the next pointer before invoking a destructor.

Interpreted resource construction uses the same physical pool, retaining its
generation references, borrower detachment, graph removal, active-operation
deferral and collection tombstones. Its existing userdata callback remains the
sole physical retirement path. The old stock `item_link_alloc_data` and the
authored scene's separate main-heap payload/metadata/free order are removed,
along with their Allocate/Free forwarding functions. Recipe scene preparation
still validates package recipes; retirement still drains authored collections.

The pool binds with the actual GObj scene epoch, and is retired at the same idle
callback/VM transition as the other typed owners. Each live node has one GObj;
the 4,096-slot capacity therefore imposes no smaller live-node cap than the GObj
owner. A source constructor that allocates payload before GObj creation unwinds
it if no GObj can be published. Reused records are fully zeroed. No normal chain
allocation grows a heap, and all pool/free-order storage has stable addresses.

One native ItemLink is 536 bytes. Reserved payload is 2,195,456 bytes, plus
metadata and free order. This storage deliberately remains in `.mstate` until
the complete explicit aggregate replaces the oracle; its reservation is not a
current whole-match size reduction. The object census reports `item_link_owner`.

## Family snapshot contract

`nativeItemLinkPool_Save`, `ValidateSnapshot` and `Load` save occupied record
bytes, every slot's generation and the exact ordered free list. Two live nodes
produce 33,896 bytes; full capacity produces 2,228,280 bytes. Empty payload is
omitted. This first representation preserves every occupied field, including
collision query scratch, rather than stripping fields from saved records.

Save requires an idle HSD/VM boundary and validates actual live class-7 GObjs,
kind-6 userdata, matching ownership and a removal callback. Nonnull chain
neighbors must be occupied in this same pool. Candidate validation checks the
whole generic format, unique stable GObj addresses and neighbors belonging to
occupied candidate slots before any mutation. It does not dereference roots
from the current timeline: restoring a destroyed GObj is an aggregate concern.
Invalid/truncated/stale-epoch candidates leave records and allocator state alone.

`jobj` is a borrowed graph reference. GObj/scheduler state, owned graph pieces,
fighter attachment tables, resource collections/generations, model/animation
leases, script actor variables and other module globals require their own
matching restoration. Restoring these records alone cannot restore a whole
chain's gameplay. Packages/programs remain resident and immutable for history.

## Verification

The extended Item owner fixture runs the actual chain pool/serializer and the
complete production stock userdata destructor with actual HSD GObj lifecycle.
Its allocation/publication sequence is exercised separately from full stock
asset loading. It passes 24 cases / 33,228 checks, covering zero initialization,
immediate/userdata-only/deferred destruction, exact slot generation reuse,
occupied byte/free-order restoration, reciprocal saved links, invalid root and
neighbor/duplicate-root rejection without mutation, epochs and full capacity.
The scheduler/other families still use the broad oracle in this isolated fixture.

Read-only census and the focused semantic/collision/census/GObj CTests pass. An
allocation-isolation private baseline compiles the previous stock constructors,
destructor and authored allocation modules from `eabe22ec1` into the current
archive. All other modules are current, including collision initialization and
diagnostic hashing. It is not a historical whole-project rebuild or a production
fallback. Three 2,000-sample rendered CPU comparisons pass on Battlefield,
Great Bay and Brinstar with Link/Samus/Ness/Ice Climbers and medium items.
Every compared populated trace family agrees; both runs consume all inputs,
exit normally and report no VM/nonfinite/census errors.

Fresh ordinary interpreted Samus, Link, Young Link and Ness packages pass
stock/source CPU comparisons, 1,800 samples each. Collections are private on A:.
The actual scripted-resource fixture passes ordinary and animated graph,
borrower retirement, collision and broad-page restore/replay checks. A direct
occupied-record test also saves/restores actual authored nodes and confirms their
generation references still resolve; it does not substitute for aggregate closure.

The other actual GObj/proc, Fighter, Ground and scene-transition fixtures also
pass after binding the new owner: respectively 16 cases / 20,734 checks,
10 / 1,650, 12 / 12,411 and 13 transitions / 417 checks. The ordinary CMake GObj
test now links the actual Ground/Item/ItemLink scene dependencies as its private
runner does, with absent-Item destructor calls trapped rather than silently stubbed.

Evidence under `A:/FreakMelee-test-archives/`: `item-link-oct11-owner-c`,
`item-link-oct11-census-a`, `item-link-oct11-baseline-b`,
`item-link-oct11-stages-a`, `item-link-oct11-source-a` and
`item-link-oct11-resource-c`, each with pinned receipt/logs. The first resource
attempt is retained as FAIL: its fixture still initialized the removed allocator;
that test setup now checks the actual shared owner. No pixel/audio parity,
every-chain interaction, complete explicit rollback or speed claim is made.
Independent verification passes all nine receipt pin sets (1,611; 1,591; 1,609;
1,610; 1,609; 1,608; 204; 55; 3,333), the four focused CTests and the baseline,
current client and archive hashes. See `item-link-oct11-independent-verification.json`.

Older extracted stock-handler fixtures read their pinned SDKs; changing the
current allocator does not invalidate those historical inputs. A new run against
the current SDK would need fixtures for the direct pool calls. Those older runs
are not acceptance evidence for this migration. The current actual-owner,
scripted-resource and connected client comparisons above are.
