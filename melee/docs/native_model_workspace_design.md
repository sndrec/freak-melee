# Model workspace: interface proposal

Status: Model layout endorsed by the user, 2026-09-26, with the Edit/Pose/
Animation mode revision below. The native client now has a working first pass
of this workspace. The requirements below remain the target where the current
implementation is incomplete.

Current native implementation: the four contexts and three rig modes share a
live fighter viewport with camera controls and bone selection. It edits
existing joint transforms, poses temporarily, and scrubs/plays inherited clips.
Meshes have editor-only eye controls, H to hide and Alt+H to reveal all;
hover highlights the actual surface light blue and viewport clicks select it.
Skeleton always displays bones. Shift-click selects multiple bones.

Model Pose Mode and the Animation workspace share move/rotate/scale gizmos
and G/R/S modal controls. Left click/Enter commits; right click/Escape cancels.
X/Y/Z constrains to an axis and Shift+X/Y/Z to the perpendicular plane;
changing the constraint always reevaluates from the transform's starting pose.
The default pivot is the selection center; active-bone and individual-origin
pivots and world/local axes are also available. Animation poses are temporary
with Auto Key off by default. Insert Key writes selected channels, creating
missing tracks when needed, and refreshes the practice preview. Keying is an
undoable asset transaction; failed encoding restores the animation bank.
Engine-controlled helper/root transforms still require care: their runtime
values include fighter placement, facing and scaling. They are not ordinary
deformation bones, and keying them does not bypass the game's overrides.

Materials expose named color/opacity sources, lighting, texture-layer, toon,
shadow, transparency and depth flags. Optional custom pixel-processing controls
cover blending factors/operation, logic operation, depth comparison and writes,
alpha comparisons/references, destination alpha, color/alpha writes and dithering.
Material changes update the affected joints' draw-pass flags without enabling
unrelated dormant geometry. PNG/JPEG texture replacement remains available.

The GLB/glTF flow converts one triangle primitive at a time into real HSD
costume geometry, with node placement, normals, UVs, base-color tint and texture.
It can add or replace a mesh. Keep source weights imports authored glTF skinning
onto matching named bones in the existing fighter rig. It checks the mapped
hierarchy and relative bind poses, and uses skin bind data for placement.
Automatic binding generates initial per-vertex
weights using the existing fighter rig. The brush supports up to 16 influences
per vertex in imported meshes, with add/subtract, radius and strength. Each
stroke rebuilds the mesh's envelope groups and participates in costume undo.
The brush targets the current deformed vertices in Edit, Pose and Animation
modes and writes weights in bind space. A stroke pauses clip playback. Temporary
poses survive mesh rebuilds and mode switches. The
[Model workflow checkpoint](native_model_workflow_checkpoint.md) records passing
saved-weight, Undo/Redo, reopen, practice and strict pose-refresh checks.
The surface displays the selected bone's weight from blue through yellow and
orange to red; the old palette-number editor and sampled vertex dots are gone.
Internally, an HSD envelope palette groups bone/weight combinations; it does not
inherently quantize them. Automatic binding starts with the two nearest deform
bones. Painting adds influences while preserving the others' relative weights,
using floating-point values throughout saving and replacement transfers.
The 16-influence cap belongs to the current editor, rather than the renderer.
Edit Mode preserves skinned meshes and rigid attachments when changing the
rest rig. It updates inverse binds and compensates vertex positions and normals
for HSD paths that also require geometry changes. Shared polygon chains and
vertex buffers remain intact for other meshes. Shape-animated meshes also
preserve their base shapes, target shapes and additive offsets. Inherited
animation clips still need review after rest edits.
See the [rest-edit checkpoint](native_model_rest_checkpoint.md). A replacement
can transfer weights using nearest bind-space vertices.

Validation: the release build and animation round-trip test pass, including
missing-channel creation at frame zero and later frames without changing other
channels. Native UI checks exercised mesh hover/hide, surface weight rendering,
pose transforms/cancellation, explicit key insertion and material apply/reload.
This is ready for the next usability review, not final Model milestone acceptance.

Still open: painting
arbitrary stock layouts, continuous triangle-surface brush picking, additional bones,
wireframe rendering, multi-mesh import,
placement preview, source-aware reimport, and advanced texture/UV controls.
Shape-animated PObjs currently use the original renderer and are not surface
picked/highlighted/heatmapped by the editor override. DAT reconstruction during painting and rest edits retains orphaned records; larger projects need compaction or a
project-owned model representation. These are functional gaps, not claims of
support inferred from the visual study.

The [floating-weight checkpoint](native_skin_weights_checkpoint.md) records
saved-DAT, replacement, capacity and live rendering checks for the current brush.
The [source-skin checkpoint](native_source_skin_checkpoint.md) records compatible
GLB imports, atomic rejection checks and the native import wizard acceptance.

The fourth mode in [the workshop study](mockups/workshop-modes.html) extends
the existing Action Editor, Scripting, and Animation modes. It imports and
prepares existing geometry. It does not create or sculpt meshes, edit topology,
unwrap UVs, or paint source textures.

## Rig boundary

Start with the cloned fighter's compatible Melee rig, including its existing
bone-role mapping and required hierarchy. Do not promise arbitrary imported
armatures will work simply by renaming joints.

The current native validation in `src/melee/ft/native_fighter_runtime.c`
checks semantic part indices including TopN, TransN, XRotN, YRotN, HipN and
TransN2. In addition, `ftAnim_8006FB88` in `src/melee/ft/ftanim.c` directly uses
`x108_costume_joint->child` for the TransN animation path. `ftparts.c` assigns
special handling to several of these roles. A name dictionary alone would
not cover those structural and animation assumptions.

The first implementation should support:

- Unrigged GLB/glTF geometry bound to the project's existing compatible rig.
- Existing skin weights retained when the imported joints, hierarchy, bind
  transforms, and joint roles can be mapped to that rig without ambiguity.
- Adjusting body proportions and bind/rest transforms, with an explicit
  rest-editing mode and a review of effects on inherited animations.
- Extra deformation and attachment bones for tails, ears, accessories, and
  other custom anatomy. Their placement must preserve required engine
  relationships and all affected references must be updated together.
- Automatic weights as a starting point, transfer from the previous mesh,
  rigid attachment to a bone, and manual weight editing.

Protect required helper structure and roles. Do not impose one universal
humanoid skeleton or require every character to use Mario's rig. Preserve the
compatible structure of the actual chosen base fighter. The mockup bone list
is illustrative, not an authoritative inventory of Rook's real joints.

Automatic conversion of arbitrary rigs is deferred. An incompatible incoming
armature can still provide geometry to rebind; it should not silently replace
the fighter's rig. Import must report unmapped influences and unsupported
transforms before applying changes.

## Workspace layout

Use one top-level **Model** work mode with four tool contexts:

| Context | Left outliner | Viewport | Right inspector |
| --- | --- | --- | --- |
| Meshes | Costume meshes and visibility | Shaded model, wireframe, editor visibility | Placement, material assignment, skin/attachment, replacement |
| Materials | Meshes and their materials | Same model and camera | Texture, tint, lighting, transparency, UV set, advanced rendering properties |
| Skeleton | Hierarchical bone tree and protected helpers | Bones over the model; rest-editing mode | Bone role, parent, rest transform, references, extra bones |
| Skinning | Bone influences, with target mesh picker | Weight display and brush | Add/subtract/smooth, radius, strength, symmetry, normalization and binding |

Keep the viewport and camera stable across contexts. Maintain per-context
selection. Outliners and long inspectors scroll independently in the desktop
layout; the viewport and bottom rig-mode controls remain in place.

Use the established middle-mouse orbit, Shift+middle-mouse pan, and wheel zoom.
Model selection happens in the viewport or outliner. Bones remain individually
selectable. The material inspector shows which meshes share the edited material.

The viewport has three explicit interaction modes shared by all four tool
contexts. This replaces the original Deformation preview strip:

- **Edit Mode:** move, rotate and scale the rest rig while preserving the mesh
  geometry in place. Recompute the corresponding bind transforms. Show the
  rest rig while editing, and retain any temporary pose separately. Structural
  operations such as adding/removing bones are available here only, subject to
  the compatible-rig constraints.
- **Pose Mode:** manually transform bones and see the mesh deform. These are
  temporary pose offsets relative to the rest rig, not rest edits or animation
  keys. Provide Reset pose. Switching modes retains this pose until reset.
- **Animation Mode:** select a clip, play/pause and scrub it on the rig. Clip
  playback drives the transforms instead of layering on the temporary manual
  pose. Manual posing and structural edits are disabled here. Edit animation
  links to the existing top-level Animation workspace for actual authoring.

The selected mode is explicit in the viewport and bone inspector. Edit and
Pose expose separate transform values; Animation exposes clip playback.
Skinning can be inspected on the rest rig, a manual pose, or a paused clip.
Weight painting modifies bind-space weights rather than animation keys. The
implementation must transform brush hits back into the binding domain and
preserve that distinction while previewing deformed geometry.

Rest edits must not silently apply a temporary pose as the new rest pose or
rewrite inherited clips. Keeping the mesh fixed while editing its rig does
not guarantee that inherited animation still fits the edited proportions.

Save, undo and Test Character retain their workshop-wide meaning. The actual
test applies a coherent draft on a practice reset. Do not hot-swap partially
rebuilt skeleton/model buffers into a running match.

## Import and reimport

Use a reviewable three-step flow:

1. **Meshes:** inspect source objects, choose which to include, and choose
   add versus replacement per mesh. Show source materials and skin availability.
2. **Placement:** review unit scale, axes, facing and origin. Preview the result
   against the current character and rig. Choose material reuse or creation.
3. **Binding:** retain compatible weights, transfer weights, generate weights,
   rigidly attach, or leave unbound. State which rig is retained. Offer to open
   Skinning immediately after import.

Reimport uses the same review, retaining project-owned material choices,
attachments and weight edits unless the user explicitly chooses to replace
them. Keep source-to-project identity per mesh and material, rather than
relying solely on object names or source array order.

Transforms need one explicit convention for axes, units, inverse bind matrices
and facing. Validate it with a known asymmetric mesh and pose before building
the broader importer. Imported glTF materials need a documented conversion to
the actual supported renderer properties; a PBR material is not automatically
equivalent to a Melee material. Preserve existing native materials when editing
unrelated properties. Unsupported material effects should be reported rather
than silently discarded.

Internal bone identity must survive display renaming or file ordering changes.
Update skin bindings, animation tracks, attachments, hit/hurtbox references,
fighter bone roles, and script bone references as one operation. Rest-pose
changes can affect inherited motion: an unchanged animation may no longer fit
changed proportions, and bind-matrix correction alone is not full animation
retargeting. Preview those changes rather than promising automatic parity.

## Study interactions and limits

The local study demonstrates the four contexts, sample geometry selection,
visibility and isolation, camera navigation, material/tint changes, rest-position
changes, extra bones, weight brush feedback, independent temporary poses,
clip scrubbing/playback, import
steps, add/replace selection, and local undo. Inspector width is available as a
design alternative through the preview's design controls.

The faceted character, geometry, poses, weights and materials are synthetic.
Choosing a local file reads its filename only; it does not parse a GLB/glTF or
modify any fighter assets. Pose and brush behavior demonstrate the interaction,
not production skinning, automatic weighting or material fidelity. Some advanced
properties are represented as editable selections rather than a renderer
implementation. Test Character does not launch the native game from this study.

The user endorsed the overall Model workspace and requested the explicit
Edit/Pose/Animation distinction. Existing approved workshop modes retain their
layout; this addition does not authorize an unrelated redesign of them.
