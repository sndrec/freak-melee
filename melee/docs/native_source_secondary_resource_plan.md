# Owned secondary models and resource collections

Source-backed implementation plan, 2026-10-09. This document proposes admission
and runtime work; it does not enable metadata or claim graph construction,
rendering or rollback acceptance. The audit made no engine changes or game runs.

Subsequent implementation: checked material preparation and actual four-root
Arrow construction/matrix/cleanup now have bounded evidence in
[the static model checkpoint](native_source_secondary_material_checkpoint.md).
The remaining metadata/Item/scene/render/history steps below still apply.

Named immutable root import and the bounded pool/scene storage implementation
are now recorded in [the owner checkpoint](native_source_secondary_owner_checkpoint.md).
Compiler metadata/selectors remain blocked pending actual operation/history
acceptance; this does not yet activate full Link/Young Link source packages.

## Declaration inventory

| Source | Exact declaration/data | Mutable owner |
| --- | --- | --- |
| `examples/link_full.fms:1343`, `young_link_full.fms:1356` | Arrow roots `trail_first 36`, `trail_second 40` | Item secondary slots 1/2; no animation bundle |
| `link_full.fms:1826`, `young_link_full.fms:1839` | Boomerang roots 68/72; animation bundles 76/88 | Same slots, plus animation state/allocation |
| `sheik_full.fms:1543` | Collection capacity 30; Link/Tip roots 100/104 | Separate ItemLink GObjs, collision and attachment joints |
| `popo_full.fms:1621`, `nana_full.fms:1847` | Collection capacity 40; Segment/Tail roots 36/40 | Same collection structure, separate imported descriptors and authored physics |
| `link_full.fms:2268`, `young_link_full.fms:2281` | Capacity 90, reserved part 139; Odd/Even/Endpoint roots 84/88/92 | Collection plus borrowed fighter-part/capture-anchor references |

The offsets are relative to each Article's original special-attribute block.
They are not joint indices or offsets into live Item memory. Capacity is a
declared upper bound; runtime counts remain supplied by typed parameters/state.

## One immutable import schema, two mutable lifetimes

`src/melee/ft/native_source_entity_assets.c:25` already binds declaration names to
owned imported Articles and the package's copied DAT bytes. Reuse that binding.
Add bounded owner-scoped rows for named roots and animation bundles. Names become
typed row selectors at compilation; runtime code must use the destination owner's
exact row, not a name hash, donor Definition or primary-model alias.

The substantive import routines are
`src/melee/it/itarticle_data.c:30` (`itArticle_ImportModelRoots`) and `:74`
(`itArticle_ImportModelAnimations`). They validate alignment, attribute extent,
local relocations, optional array indices and typed DAT graph imports, then
publish a complete table. Refactor their recipe inputs away from the legacy
NativeFighter model/hash types. A null pointer must remain explicit; it cannot
stand in for a declared secondary graph. Animation components are separately
typed joint/material/shape descriptors, not another model root.

`native_script_item_program.c:168` currently rejects `model_root`,
`model_animation` and `resources`. `native_script_binding_item_model_resolve.c`
currently admits only literal primary model 0. Extend validation and resolution
only alongside the complete corresponding runtime owner.

**Item slots:** `native_article_model.c:115` constructs independent HSD graphs;
slots 1..3 are `Item.native_models[model-1]`. `itdraw.c:237` draws those graphs,
and `item.c:2515` destroys them. Construct a replacement before releasing the old
slot. Keep `native_article_model.c:220` clear-before-free teardown. Capture exact
source spec/program/storage and the selected root across operations.

**Collections:** `native_article_resource.c:246` currently admits owners through
Definition/logic-pointer identity, and retains a legacy creator. Replace those
checks directly with prepared source spec/bank/storage, full scene/pool/native
identity and captured revocable attachment relations. `CreateNode` at `:301`
creates real ItemLink GObjs with their own position, velocity, CollData and
borrowed attachment JObj; they are not Items or aliases of Item secondary slots.
Preserve sequential creation and false-on-capacity-failure. `release_collection`
at `:165` revokes access, defers destruction during active operations and detaches
all borrowers before freeing any graph. Hookshot's part/capture borrowers need
their actual rig reservation and cleanup; Sheik/GumStrings do not require that
extra reserved-part feature.

## Smallest functional batch: static arrow trails

1. Prepare indexed immutable roots for Link/Young Link Arrow from offsets 36/40,
   using the exact owned importer and candidate-only publication.
2. Complete admission for their real descriptors and material environment.
   `native_article_model_demand.c` covers the validated custom TEV allocation
   shapes, but these four roots still report MaterialEnvironment (128).
   `sysdolphin/baselib/mobj.c:337` can splice global toon/shadow textures. Validate
   actual default methods/effective environment and exact compile legality;
   allocation bounds alone are not packed-register legality. Do not assume an
   empty environment or accept by fighter/root identity.
3. Reserve construction and lazy matrix-scale demand at the explicit scene
   transition, then execute the real HSD loader under its bounded claim. Include
   replacement headroom. No tick heap growth or precreated graph army.
4. Connect literal model slots, transforms and draw controls to those independent
   graphs; preserve absent-joint defaults without substituting the primary graph.
5. Prove load, replacement, destruction, allocator reuse and broad-page restore
   against the actual four imported roots before accepting metadata.

The existing arrow descriptor and custom-TEV evidence is recorded in
`docs/native_source_arrow_descriptor_census.md`. It does not prove complete graph
construction. Boomerang needs additional animation-allocation/cleanup demand;
collections need ItemLink/GObj/borrower ownership and their typed resource API.
They should not be admitted merely because arrow loading works.

## Reservation, history and package retirement

`native_hsd_reservation.h` explicitly does not register snapshot regions. Its
metadata, backing, allocation/free order and active registry must be captured,
along with actual allocator counters, ID table and mutable graph/material state.
`native_rollback_regions.c:51` captures active main heaps and `.mstate`;
package malloc memory is not automatically such a region. Use captured native
scene backing before history initialization, or explicitly register real backing
with the history owner. Actor snapshots alone do not contain graph state.

`native_source_package.c:78` prepares separate candidate-owned imports and code;
failed preparation discards only that candidate. `CanRelease` at `:124` checks
leases, published programs and actor references. New graph/collection resource
owners must also remain pinned through live allocations and retained history.
Restore resurrects captured allocations/state without replaying constructors,
shader compilation or authored callbacks. Failed staging must never mutate an
accepted package's resources.
