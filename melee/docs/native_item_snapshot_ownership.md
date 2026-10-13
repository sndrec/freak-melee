# Native Item owner and occupied-record snapshots

2026-10-11 checkpoint. Actual Item userdata and its optional model bone lookup
table now have one stable native owner. This is not complete explicit match
save/load; production still uses the broad page oracle.

## Storage and lifecycle

`Item_8026862C` allocates directly from `nativeItemPool`, after its existing
GObj creation/draw registration and before kind-6 userdata publication.
Article selection, initialization, model setup, callbacks, collision and item
admission limits retain their existing paths. There is no heap fallback.

`Item_802682F0` assigns the owning slot's `DynamicBoneTable` at the existing
model-construction phase when a table is needed. It retains the original
depth-first traversal and 100-entry layout. Empty model tables stay null;
an oversized runtime tree rejects construction rather than overwriting storage.
Descriptor admission already checks this bound, but runtime storage now checks
it too. This changes no valid asset behavior.

`Item_OnUserDataRemove` still cancels that item's resource generation, then
detaches its script actor, then retires its occupied slot. The bone table no
longer needs an independent allocation/free. The public `item_link_alloc_data`
and chain/rope `ItemLink` consumers have not been migrated or replaced.

The 4,096 slots match GObj capacity. `Item_8026784C` caps several hold categories
but not every category; those admission rules cannot justify a smaller global
reservation for arbitrary authored items. Each slot contains a 5,608-byte Item
and its 800-byte bone table, with alignment padding: 6,416 bytes total, or
26,279,936 reserved payload bytes. Scene preflight/bind/retirement follows the
actual GObj scene epoch. Allocations clear reused slots, including satellites.

Storage, metadata/free order and the descriptor remain NON-HOST `.mstate` so the
old oracle covers them during migration. This temporarily adds reserved bytes
to that oracle; it is not a present-day reduction in whole-match snapshot size.

## Packed family save/load

`nativeItemPool_Save`, `ValidateSnapshot` and `Load` serialize generations,
ordered free slots and occupied slots only. An occupied slot includes its
reserved bone table, even when the table pointer is null. Two occupied items
produce 45,656 bytes; all 4,096 produce 26,312,760 bytes. Metadata is retained
for free slots, but their payload is omitted.

Save checks an idle callback/VM boundary, actual typed storage/epoch, live
CLASS_ITEM roots, kind-6 userdata, exact owner and actual removal callback.
Candidate validation checks complete generic format and exact optional
bone-table aliases before mutating anything. Saved roots are checked against
stable GObj storage geometry without dereferencing current objects: a later
aggregate must be able to resurrect objects removed in the current timeline.

GObj topology, joint/material graphs, dynamic-bone physics allocations,
article resource collections, script actor/storage owners and item global
counters are separate dependencies. ItemLinks now have a separate
[shared stock/authored owner](native_item_link_snapshot_ownership.md). This family does not claim their
closure or provide a complete standalone match restore.

## Verification and outstanding differences

Client/content-tool build passed. Focused fixtures passed:

- Item: 14 cases / 24,945 checks; actual complete model-table builder and
  userdata destructor extracted unchanged from item.c and hashed.
- GObj/proc: 16 cases / 20,734 checks.
- Fighter/satellites: 10 cases / 1,650 checks.
- Ground: 12 cases / 12,411 checks.
- Scene lifetime: 13 transitions / 417 checks.
- Read-only object census: 13 cases / 45 checks, including the actual Item
  owner descriptor in payload accounting.

The Item fixture covers immediate/deferred/userdata-only retirement, generation
reuse, zero initialization, optional/full/oversized bone tables, exact occupied
restore and allocator order, malformed/root/alias/truncated candidate rejection
without mutation, stale epochs and all 4,096 slots. Full asset/gameplay creation
is not run in this isolated fixture; cancellation/VM internals are controlled
boundaries checking argument/liveness/order. Other owner fixtures construct no
Items and trap any unexpected call to the absent Item destructor.

Fresh interpreted Samus, Link, Ness and Peach CPU source-vs-stock cohorts passed
after the build, 1,800 controller samples each. These use newly prepared private
packages and do not modify the installed collection.

Eight fresh-source results cases also passed: win1 and lose for Ness, Peach,
Game & Watch and Ice Climbers, 900 samples per case. These exercise actual
death/match-end/results transitions; they do not establish results pixel parity.

Old/new rendered, hidden, muted 2,000-sample CPU comparisons completed for
Battlefield, Green Greens, Brinstar, Great Bay, Pokemon Stadium and Corneria,
plus a Battlefield Peach/Link/Ness/Samus roster. All exited normally with no
VM/nonfinite/census errors. Populated fighter/companion/action/damage/RNG
families matched, as did the resource channels present in three cases. Empty
optional channels do not establish coverage. **All seven initial strict receipts
are FAIL because Item hashes do not match.** They are retained; the field-level
follow-up below supersedes their unresolved diagnosis, not their historical status.

The first Battlefield Game & Watch parachute difference is in the cumulative
motion group; its identity, visible position/velocity, relations, model pose and
visual hashes match. The projectile roster's first PK Fire difference is in
the cumulative hitbox group; prior identity/motion/animation and model pose match.
Green Greens first differs in item relations. At that checkpoint, clearing
formerly reused heap payload was a hypothesis. The named-field trace now resolves
these cases and subsequent differences with source read/write evidence below.

Evidence is under `A:/FreakMelee-test-archives/`:
`item-owner-oct11-native_item_pool-b/receipt.json`, the corresponding owner
fixture `-b` receipts, `item-owner-oct11-native_match_object_census-a/receipt.json`,
`item-owner-oct11-sources-a/receipt.json`, `item-owner-oct11-stages-a/receipt.json`
`item-owner-oct11-projectiles-a/receipt.json` and
`item-owner-oct11-results-a/receipt.json`. Build output is
`item-owner-oct11-build.log`. No full-state timing, every-item interaction,
pixel/audio, long-term gameplay or multi-PC claim is made here.

## Named-field allocation comparison follow-up

`FREAK_MELEE_ITEM_FIELDS=start:end[,start:end...]` emits named raw values in
selected inclusive controller-sample ranges when fighter tracing is enabled.
The diagnostic module has fixed host-only storage, performs no gameplay writes,
and is disabled by default. Object values use the existing semantic object IDs;
float values print their exact bits. It includes disabled payload deliberately
so old allocator residue can be distinguished from active simulation state.

The batch traced these concrete differences:

| Case / first sample | Differing field | Source contract and correction |
| --- | --- | --- |
| Battlefield / 1046, parachute | `CollData.contact` | `mpColl` sweep queries write this contact output before their successful-query consumers use it. Exclude idle query output from the semantic digest; retain collision positions, history and surfaces. |
| Green Greens / 259, Random article | Disabled hitbox victim pointer | `lbColl_80008A5C` does not age disabled capsules, and item collision does not query them. Match the core digest's disabled-state handling in the relation digest. Active victim and owner references remain compared. |
| Projectile roster / 1246, PK Fire | Hit contact position/distance and empty victim counter | `lbColl` intersection routines produce contact results for `ftColl`/`itColl`; admission overwrites an empty victim's counter, and aging reads counters only for nonnull victims. Exclude result scratch and canonicalize empty counters; retain active geometry/damage and live victim timers. |
| Projectile roster / 1399, yo-yo | Collision stop flag and previous environment flags | `mpColl_80041EE4` omitted both initial values. Initialize them to zero in the actual shared initializer. Previous collision flags remain in the digest because other items and fighters consume them. This intentionally removes undefined initial heap contents. |
| Brinstar / 1526, Pokemon; Stadium / 973; Corneria / 910 | `CommandInfo.frame_count` | `it_802799E4` overwrites it from `x5CC_currentAnimFrame` before any command reads it. Observe that authoritative frame and active command wait; ignore derived frame scratch and inactive waits. |
| Projectile roster / 1843, Food | `xDB4_itcmd_var2` | Stock Food's four states use animation -1 and no DAT command stream. Its callbacks use the Food union, never the four command scratch words. Exclude those words for stock Food; preserve them for authored callbacks using the generic command properties. |

This changes the **read-only diagnostic digest**, not the byte representation of
saved Item records. The Item restore fixture now explicitly poisons query
results, a disabled victim pointer, empty victim counters and command-frame
scratch and checks exact full-slot restoration. It passes 14 cases / 24,945
checks (`item-field-oct11-owner-d/receipt.json`). No fields were removed from
the occupied-record serializer.

The extended `native_semantic_state` fixture checks both directions: these idle
fields do not change the digest, while current/previous capsule positions,
damage, occupied victim timers/references, previous collision flags, active
command waits and authored Food command words do. `native_collision` runs the
actual initializer on poisoned reused bodies and then exercises ordinary and
subdivided floor landing. Both focused CTests pass. Its older stack-only physics
actor needed registration in the actual GObj list for the production callback
lifetime contract; the fixture now creates and retires a real GObj and retains
its original callback/wind/hitlag expectations.

The final allocation-isolation baseline replaces only the actual `item.c` archive
member with revision `a995e271f`; all other modules, including the corrected shared
collision initializer and digest, are identical to the current build. It is a
private test binary, not a production fallback. Therefore this comparison proves
the allocation change under the current engine contracts, not that deliberately
undefined initial flags match the previous whole executable.

Final allocation-isolation batch: **all seven cases PASS**, six selectable stages
and the Peach/Link/Ness/Samus Battlefield roster, 2,000 samples each. Both binaries
exit cleanly, consume every sample and have no census, VM or nonfinite errors.
Every compared trace family, including Items, agrees. This is selected semantic
trace acceptance, not every item's behavior, raw heap equality or pixel parity.
The diagnostic range environment is unset for both final receipts.

Fresh ordinary interpreted Samus, Link, Ness and Peach packages also pass
stock/source CPU comparisons on the final client, 1,800 samples each, with
unchanged pinned inputs. Their private collections do not modify installed
content. Results evidence earlier in this document belongs to the preceding
client build; it was not rerun as part of this diagnostic follow-up.

Final receipts under `A:/FreakMelee-test-archives/`:
`item-field-oct11-stages-e/receipt.json`,
`item-field-oct11-projectiles-e/receipt.json`,
`item-field-oct11-source-e/receipt.json` and
`item-field-oct11-baseline-e/receipt.json`.
Comparison build hashes and private compile/link logs are retained in the
`item-field-oct11-baseline-e` receipt/directory. The later ItemLink rebuild log
is `item-link-oct11-build-a.log`.
Independent pin/focused-test verification:
`item-field-oct11-independent-verification.json`.
That verification passes: all 1,607 owner-fixture pins, 36 stage pins, 36
projectile pins and 55 source-package pins are unchanged; both focused CTests
pass and the recorded client/archive hashes match.
The original failing receipts and intermediate diagnostic failures remain
preserved on A:. Whole-match serializer closure and save/load timing remain
separate work.
