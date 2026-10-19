# Weighted morph meshes: implementation plan

Status: pending implementation. Morph evaluation and editor surface queries
are implemented; morph bone-weight painting remains unavailable.

## Required behavior

A weight stroke must preserve every complete shape or additive offset, normal
target, UV, color, material and animation reference. A vertex keeps its weight
across all morph targets. Separate vertices with equal base positions can have
different targets and must remain separate. Shared lighting vectors need private
entries when their vertices acquire different weights.

Keep one morph controller per authored polygon. Its animation object, blend
values and place in the polygon chain retain their meaning. Splitting a polygon
into skin palettes must not shift the animation table's polygon indices or
create independently ticking copies of that controller.

## Draw representation

Extend the native shape descriptor with an explicit reference to private skin
draw batches. Each batch uses ordinary envelope descriptors with at most ten
weight combinations. Their display lists address the controller's evaluated
position and normal indices. The parent retains the authored shape topology and
controller. A new shape flag identifies the added descriptor field; this is a
native data extension, with no format-version migration layer.

The renderer evaluates the shape once into its fixed scratch arrays, sets the
batch's existing envelope matrices and emits the evaluated geometry through
those matrices. This keeps skinning in the renderer's existing GPU path. Batch
construction and allocation happen during asset loading or an authoring
transaction. The drawing-disabled pose path prepares the same bone dependencies
in the same order.

Picking, highlighting and brush projection need the same binding for each
logical shape vertex as drawing. Keep that mapping explicit in the loaded
shape data. Position and normal indices can differ. A normal shared by vertices
with different bindings must be split together with every one of its targets.
Admission must prove all maps and batches fit the admitted scratch capacities.

## Authoring and ownership

The shape reader must retain complete target identity, not merge vertices using
only the base position. Coordinate conversion applies an affine transform to
complete/base positions and its linear part to additive offsets. Normals use
the inverse transpose. The existing envelope coordinate domains still apply
to root, deform-bone and helper owners.

The writer must publish the new target arrays, binding maps and batches as one
undoable costume transaction. Failed validation leaves every asset unchanged.
Save/reopen, package preparation, compaction and bone-reference traversal must
follow the added graph. Retired batches and arrays must be reclaimed.

Rest edits must compensate each logical vertex using its binding. The current
rigid shape compensation cannot be applied unchanged to a weighted shape.
Normals and their targets must follow the same binding-dependent compensation.
Adding/removing/reordering bones must keep all batch envelope references valid.

The current GLB importer emits base geometry and has no morph-target encoder.
Preserving imported GLB targets remains part of integrating the combined model
representation. It must use the same target and binding rules as painting.

## Acceptance

- Compare actual runtime posed corners and normals before/after a no-op rewrite
  across complete/additive modes, source formats, shape blends and bone poses.
- Paint individual and shared vertices; verify untouched surfaces and targets,
  all authored weights, normal seams and the original controller's lifetime.
- Exercise Save/reopen, byte-exact Undo/Redo, package loading, compaction and
  repeated strokes without unbounded retired storage.
- Exercise rest translation/rotation/scale and extra-bone changes after painting.
- Compare native drawing, hover, picking and brush projection at several blends.
- Check matching bone preparation with drawing enabled/disabled and restoration
  of the mutable controller through the current save/load system.
- Reject malformed maps, invalid ownership and capacity exhaustion atomically.
