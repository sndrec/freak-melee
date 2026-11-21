# Reserved model parts

`model_graphs.install(graph_slot, omission_index) -> bool` fills a reserved part
in both the fighter's visible model and its alternate animation pose. It uses
the current package's copied assets and placement metadata. It never selects a
stock fighter's load handler or borrows another fighter's runtime graph.

[Costume attachments](model_attachments.md) | [Bone transforms](combat_visuals.md) |
[Fighter presentation](presentation.md) | [Asset ownership](../fighter_asset_ownership.md)

## Scene setup

Call this during `on load`. Graph creation uses the normal HSD scene heap;
it is not a per-frame operation. Ordinary owned parameters can give descriptive
names to the asset slot and placement entry:

```text
parameter int accessory_graph = 6
parameter int accessory_placement = 0

action CostumeSetup:
    on load:
        let bool installed = model_graphs.install(attrs.accessory_graph, attrs.accessory_placement)
```

`graph_slot` is an integer slot in the fighter DAT's decoded joint-graph table.
The immutable asset layout must declare it as a joint graph. Article slots and
joint graphs may share an archive table, but this operation accepts only graphs.
`omission_index` selects copied placement metadata defining the reserved target
part, parent part, insertion mode and source descriptor ordinal. These are asset
facts, separate from editable move behavior and physical skeleton count.

The descriptor ordinal selects one joint in preorder. The value 255 selects the
root. The selected joint becomes a single reserved part. Its mesh and constraints
must work independently. For example, if its vertices use weights from another
donor bone, installation returns `false`. Use a costume attachment for an
accessory that needs several bones. [Costume attachments](model_attachments.md#attachmentsloadslot-root-bone-visibility_entry-visibility_costume)

## Insertion and failure

Placement modes 0/1 insert as a child; modes 2/3 insert as a sibling. Mode 0 places
the part above the existing children; mode 1 places it alongside them. Mode 2
adopts existing siblings as children; mode 3 keeps them alongside the new part.
Both current and alternate poses use the same placement. The engine registers
the new part's depth, visibility/material flags and drawing objects. Drawing
objects append to the costume's existing list.

An occupied reserved part, missing graph/parent pose, invalid index or ordinal,
unsupported particle/spline descriptor, or insufficient drawing capacity returns
`false` without changing the rig. If the second pose allocation fails, the first
allocation is released before insertion. Repeating a successful installation
returns `false`; it does not allocate another copy. Fighter destruction releases
the merged graphs through the ordinary model lifecycle.

This operation does not synthesize bones, enlarge the physical skeleton, bind
item callbacks or animate an accessory. Its corresponding rig and animation
mapping must already be owned by the package.

The interpreter exposes this directly for the current source-owned Fighter.
Selectors use the native signed Int ABI; floating-point selectors are rejected.
It retains the exact Fighter generation, physical pool slot, selected program
and storage, along with the owned package, part/omission tables, visible and
alternate roots, parts array, graph table and drawing-list allocation. Native
installation may append drawing entries and populate the reserved part. A
changed retained identity prevents Bool publication; native work already
completed is not rolled back.

Use an explicit source setup transition for the allocation. `on load` is the
intended callsite, but the operation does not invent or enforce a load-only
runtime phase. Duplicate installations return false before allocation.

## Validation boundary

The console comparison executes original two-pose insertion and the native
installer across all four placement modes and root/child/sibling selections.
It checks invalid reservations, absent assets, allocation failure, duplicate
installation and drawing-list append. HSD allocation and part registration are
controlled dependencies; it does not render a costume or tick gameplay. Both
script source forms call the same Boolean host operation. The connected
`tests/native/script_model_graph` fixture also executes the actual interpreter
catalog/compiler, Fighter actor/VM and owned SourcePackage branch. Its allocation
and native completion observations are explicit test boundaries, not authored
callback or renderer acceptance claims.
