# Scripted article resource registry snapshots

2026-10-11 checkpoint. The actual scene-local resource collection registry and
its generation counter now have an explicit serializer. This closes a separate
owner from the Item script actor: script variables do not contain the retained
model, attachment or ItemLink identities. Production rollback still uses the
broad page oracle; this component does not establish whole-match timing.

## Owned state and lifetime

`native_article_resource_snapshot.c` saves occupied collection prefixes and
their created node records from `nativeArticleResourceCollections`, plus
`nativeArticleResourceGeneration`. It writes no unused node tail or empty
collection payload. Load clears the actual registry, restores occupied rows and
the counter, and invokes no constructors, destructors, scripts or callbacks.
There is no mirror registry, new tick tracking, dynamic allocation or format
compatibility machinery.

The retained rows include source Item and attachment Fighter lifetimes,
parts/rig references, recipe counts, external-part index, graph descriptors,
actual graph/GObj/ItemLink pool references, binding flags and collection
generation. A generation remains attached to the physical model and node,
rather than being inferred from its currently occupied address.

Prepared source programs, recipes, descriptors and model reservation geometry
remain resident and leased throughout a history window. Scene and model
preparation each publish a monotonic identity, separate from the GObj epoch.
Retire/re-prepare in the same object epoch therefore invalidates old snapshots.
Only successful preparation changes these identities; save/load does not.

## Validation and companion owners

Save requires idle script execution, no active reservation claim, GObj process,
ordinary/max draw traversal or delayed mutation, and no resource operation or
release in progress. It verifies the actual live collection and every node
before writing output. Output buffers cannot overlap the registry, control
state, relevant pool payload/metadata, prepared package structs, ordinary
ObjAlloc backing or reserved graph backing.

Candidate validation checks lengths, ascending collection slots, counts,
generation consistency, recipe/frozen program identity, pool address/epoch/
slot identities, descriptor roots, duplicate node endpoints, and attachment
parts at their canonical Fighter-slot address. It checks known frozen programs
and known pool metadata before dereferencing them. It does not resolve saved
references against current occupancy: their slots may have been freed or
reused since the save. Malformed candidates reject before live writes.

Registry records borrow payloads owned by other components. Restore requires
the scheduler, ItemLink, graph reservation, class/ObjAlloc, animation, ID roots,
typed Fighter/Item and Fighter/Item script-actor state before any normal queries
or callbacks resume. Both borrower and borrowed graph must return together.
All candidates must be validated before the first owner is modified.

This is not complete cross-owner candidate validation. In particular, local
registry validation does not establish every retained Item payload, VM storage,
rig or child-joint pointer against the other serialized components. The final
aggregate must prove those relationships against the candidate peer state,
including occupancy and generations. Loading a registry component by itself is
not a safe gameplay restore contract.

## Connected fixture evidence

`tests/native/script_item_resources/resource_snapshot_cases.h` runs actual
source collection/model/controller operations and interpreted Item callbacks.
It saves eleven actual components, changes a real script variable, destroys a
bound animated collection, creates another actor's future collection in reused
storage, validates all components, then restores them before inspecting state.
It proves the Fighter's external part and hitbox borrower return, all component
bytes re-save exactly, and the next collection generation/GObj/ItemLink/model
allocation repeats. The OS Cell snapshot remains unchanged in this bounded
window; no OS allocation is hidden by a copied heap restore.

Fourteen refusal cases cover truncated/trailing data, frozen identity changes,
oversized counts, active operations, foreign program/pool/parts addresses,
zero generations, and actual model/scene retire-reprepare in the same GObj
epoch. Additional output-alias and in-progress boundary refusals preserve state.

Final archives use `A:/FreakMelee-test-archives/resource-registry-oct11-`:

- `resource-e`: ordinary cohort 3,082 checks/18 cases; animated cohort
  4,812/19. The new registry exercises 172/175 checks, with 14 refusal cases
  each. One two-node registry occupies 580 bytes; this is not a full-match size.
  Existing 39 native/interpreted animation pairs and 68 scheduler/script checks
  remain passing. Source/tool snapshots are pinned and unchanged.
- `build-c.log`: native client, content/source-assets and GObj/census targets
  build successfully.
- `stages-a`: Battlefield, Great Bay and Brinstar each run 2,000 rendered CPU
  samples with Link/Samus/Ness/Ice Climbers and medium items. All populated trace
  families match the previous compiled client; no census failures.
- `source-a`: freshly prepared interpreted Samus/Link/Young Link/Ness each
  pass 1,800-sample stock comparisons without source diagnostics or VM faults.
- `independent-verification.json`: all 3,345/228/66 receipt pins, the prior
  87-file client bundle, and eleven production/fixture source files match.
  README and ownership documentation were updated after testing; compiled
  inputs remain identical to the frozen fixture.

The fixture uses a controlled small Fighter rig, now stored in the actual
typed Fighter slot, and controlled Item heap construction. It is not the full
production Item constructor, every package/graph, a GPU pixel comparison or a
complete explicit match restore. Existing broad-page restore cases remain as
separate oracle tests; the registry component test above restores its explicit
owners without those pages.

Earlier receipts remain for diagnosis: strict fixture compilation failed on
missing initializer fields, and an animated test incorrectly addressed a child
joint although its declared bundle belonged to the root. The final fixture uses
the declared root and preserves production animation admission.
