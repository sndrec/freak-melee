# Native stage collision snapshots

2026-10-11 checkpoint. Stage collision now has stable actual runtime storage
and an explicit scene-local serializer. Production rollback still uses the
broad page oracle. This component is not a complete match snapshot or a
whole-match performance result.

## Actual ownership

`native_collision_state.c` owns the engine's actual vertex, line and joint
arrays at the original capacities: 2048 vertices, 1536 lines and 256 joints.
`mpLibLoad` publishes those arrays at a scene transition and initializes them
with the original collision code. Original queries and mutations use these
same arrays; there is no second collision representation. The arrays stay in
`.mstate` for the production page oracle.

Snapshots copy the active prefixes, the bound `MapCollData` description, and
the source vertex, line and joint prefixes. Source descriptors cannot all be
treated as immutable assets: dynamic updates change platform flags and
alternate line links. Save/load retains these writes as well as transformed
and previous-frame vertex positions.

The component also owns joint list roots, bounding boxes and culling state,
collision/update counters, the actual collision-query control and scratch
state, the island-list roots, and all four dynamic surface-attribute records
and their active/free roots. Query callers now access the named fields of
`nativeCollisionQuery` directly.

Island records use the existing class-piece allocator at their three original
allocation sites. Their complete payloads and allocator/free-list state belong
to the class and OS companion serializers. The original island algorithms,
order and retained pointers are unchanged. Save/load allocates nothing and
invokes no callbacks. This does not prove that every dynamic island allocation
has been prewarmed before gameplay; aggregate scene preparation remains work.

## Lifetime, validation and restore contract

Scene publication freezes the source pointer/count binding and publishes a
monotonic collision identity plus the current GObj epoch. A second map load
invalidates earlier snapshots even when the source addresses and GObj epoch
are unchanged. Source descriptor backing and borrowed graph targets must remain
resident for the history window. Publication rejects active page history or
script, reservation, GObj, draw and delayed-mutation boundaries.

Save/load requires an idle boundary. Local candidate validation checks exact
length, source binding, capacities and ranges, descriptor line links and
endpoints, canonical runtime/source pointers, active joint membership and list
topology, and attribute link addresses and disjoint acyclic lists. Load validates
the complete local candidate before modifying live state. Output must not alias
this owner's controls, runtime storage or bound descriptor arrays.

Attribute lists need not cover all four physical records. The original
`grDynamicAttr_801CA1C0` can unlink an expired non-head record without returning
it to the free list. Snapshots preserve those unreachable records too rather
than changing this behavior or refusing a legitimate saved state. Nonfinite
runtime float bits also remain representable for reproducing engine faults.

Island nodes, JObj bindings and callbacks/query targets are borrowed references.
The class/ObjAlloc/OS and other relevant owner candidates must be validated and
restored together before collision queries or callbacks resume. This component
does not resolve those references against current occupancy, which can differ
after destruction and reuse. Complete validation against the other candidate
owners, aggregate buffer alias checks and production restore integration remain
open. Loading only this component is not a gameplay restore contract.

## Native line-stride correction

`mpLinesConnected` and its alternate-link helpers previously indexed flags at
two `u32` words per line. That matches the console's eight-byte `CollLine`, but
native pointers make the record sixteen bytes. They now index typed records
directly. Enabled/hidden flags, distance and line-kind rules are unchanged.
The connected fixture exercises adjacent nonzero line indices, both traversal
directions, disabled/hidden lines and endpoints outside the original tolerance.

## Evidence and remaining limits

Archives use `A:/FreakMelee-test-archives/collision-oct11-`:

- `owner-e`: 145 checks, four cases and fourteen malformed-candidate refusals.
  The synthetic six-vertex/three-line/three-joint component occupies 1184 bytes;
  this is not a real-stage or full-match size. Real JObj matrix evaluation moves
  the platform. Actual floor, speed, island and timed-attribute queries return
  after model destruction/reuse and coordinated collision/class/ObjAlloc/OS
  restore. Every component re-saves exactly and subsequent mutations repeat.
  Boundary/alias refusals, same-epoch map reload and NaN bit preservation pass.
- `resource-a`: the actual script/resource/graph cohorts retain 3082 and 4812
  checks, including 39 native/interpreted animation pairs and 68 scheduler/script
  checks. This run preceded the final collision-only validation correction;
  its frozen sources and libraries remain independent evidence.
- `build-d.log`: native client, content/source-assets, GObj consumer and census
  targets build successfully with the final collision validation correction.
- `stages-a`: Battlefield and Great Bay match the previous client for 2000
  rendered CPU samples with Link/Samus/Ness/Ice Climbers and medium items.
  Brinstar first differs in P2 position Y by one float bit at sample 1072;
  the old/new trace comparison remains FAIL. Runs finish without VM/nonfinite
  or census failures. The attribution below identifies the corrected query as
  the cause; the original exact old/new FAIL receipt is preserved.
- `self-brinstar-a`: two runs of the same new client match all traced families
  for 2000 samples. This narrows the old/new discrepancy; it does not prove its
  cause or old/new parity.
- `source-a`: freshly prepared interpreted Samus, Link, Young Link and Ness
  each pass 1800-sample stock comparisons with no VM/source faults. This is
  separate scripted-fighter evidence, not a collision snapshot timing test.

### Brinstar query attribution

`query-isolation-a` independently recompiles just `mplib.c` with the production
flags and read-only production libraries. One control contains the current
source; the other substitutes only the three original line-connectivity
functions from `ab57d0e4b`. The class/island, actual fixed collision storage,
query controls and source ownership are identical between these diagnostics.
Libraries/executables are linked outside the checkout and the production
inputs remain unchanged. `runtime-deployment.json` pins the copied UI/DLL
bundle; the first diagnostic runs failed before gameplay because the UI bundle
was absent, and their failed receipts remain.

`query-control-b` matches the production client exactly across all populated
trace families for 2000 samples. `query-unfixed-b` finishes normally but retains
the old/new fighter difference. The original pre-migration client and this
isolated unfixed client match exactly. Both old/current comparisons have the
same fifteen differing fighter records, starting at sample 1072; actions,
damage, items, resources and RNG families match. Thus the query correction,
rather than an unattributed owner migration effect, accounts for this bounded
cohort's discrepancy.

`query-attribution-final.json` records the exact comparisons using
`tests/native/native_collision_snapshot/compare_query_migration.py`. No fields
are normalized or excluded. Diagnostic receipts pin source/commands/libraries,
and the connected collision fixture separately establishes the corrected
typed-record indexing against the original enabled/hidden/kind/distance rules.
The fix remains: the old native stride reads fields from the wrong record on
64-bit PCs. This does not establish console pixels or every stage interaction.
`query-independent-verification.json` verifies 87 build-input pins, 237 pins
per gameplay comparison, 258 deployment pins, both executable hashes and the
exact three-function source substitution. The unfixed comparison must retain
its trace FAIL while both of its gameplay runs finish without runtime errors.

The collision fixture supplies a synthetic map and model binding explicitly;
it is not a production stage loader, every moving platform/hazard, script
fighter simulation, GPU comparison, network run or timing acceptance. Earlier
receipts retain fixture symbol mistakes, missing dynamic-island metadata, and
the too-strict full attribute-pool partition check for diagnosis. Full-world
state, cross-owner closure, the production aggregate/ring and measured whole
match save/load/resimulation cost remain unfinished.
