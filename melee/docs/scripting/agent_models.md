# Model authoring with an agent

The model tools edit a fighter's owned costume, skeleton and skin weights.
They import GLB/glTF meshes, embed PNG/JPEG textures and preserve source
information for reimport. Saved-draft tools start with `models.`; live tools
start with `session.models.` and work on the selected member's unsaved assets.

See [Agent tools](agent_tools.md) for revisions, live sessions, Save, Build,
practice and export. Use the installed tool catalog for the complete schemas.

## Inspect the model

Call `models.read` with a project path and a `view`. In a live session, call
`session.models.read` with the selected `instance` instead. Lists accept
`offset` and `limit`, and return `next_offset` for the following page.

| View | Contents |
| --- | --- |
| `info` | Bone, mesh and material counts, capacities, units and editable material fields. |
| `bones` | Names, hierarchy, roles, stable bone identities and local rest transforms. |
| `meshes` | Mesh names, owning bones, material assignments and import information. |
| `materials` | Surface colors, lighting, transparency, depth and blending settings. |
| `textures` | A material's texture layers, mapping, transforms, filtering and color combiners. Supply `material`. |
| `vertices` | Bind-space positions and normalized bone weights. Supply `mesh`. |
| `sources` | Imported meshes, source paths, source identities, scale and binding choices. |
| `texture_sequences` | A material's animated texture sequences and their image counts. Supply `material`. |
| `reimport_plan` | Source primitives matched to existing meshes. Supply `mesh`, `path` and optional `whole_source`. |

Names can be filtered with `query` in the bones, meshes, materials and sources
views. Vertex positions use game units in the model's bind space. Local bone
rotations use radians; scales use multipliers. Weights range from zero to one.

The `id` values for bones, meshes, materials, vertices and texture sequences
identify entries in the inspected revision. Resolve them again after importing,
changing the hierarchy or assigning a different material. Each bone also has a stable `bone_id`, used by the
`rig` operation. Keep these two identities distinct when authoring a skeleton.

## Import and reimport

Use `models.scan` with a local GLB/glTF `path` to list its primitives. Each
primitive reports its source identity, vertex/triangle counts and available
normals, UVs, skin and texture data. A glTF source may reference local buffers
and images; those must be present when importing.

Use `models.edit` with `operation: "import"`, the source `path` and an
`imports` array. Each entry names a scanned `primitive`, a target `bone` and
an optional `replace_mesh`. The complete array is applied together.

```json
{
  "operation": "import",
  "path": "C:/my-fighter/body.glb",
  "imports": [
    {"primitive": 0, "bone": 0, "binding": "automatic", "scale": 1}
  ]
}
```

Add the inspected `project` and `revision` to a saved edit. Live edits use
`instance`, `revision` and a short `summary`. The source geometry and images
are embedded into the fighter's owned assets when the import succeeds.

| Binding | Behavior |
| --- | --- |
| `automatic` | Use compatible source skinning when available, otherwise attach rigidly. |
| `rigid` | Attach the primitive to the chosen bone. |
| `source` | Use source skinning with matching target bone names. |
| `transfer` | Transfer weights from the replaced mesh onto the imported geometry. |
| `keep` | Preserve the replaced mesh's authored binding and weights when its source topology permits it. |

Inspect a `reimport_plan` before using `operation: "reimport"`. Supply the
existing `mesh`, updated source `path` and, optionally, `whole_source: true`.
Matching uses source identities, so reordered source primitives retain their
destinations. Reimport uses `keep` binding by default. `replace_material: true`
also takes the updated source material; the default preserves edited material
settings. Missing, ambiguous or incompatible matches return a diagnostic.

## Rest transforms and extra bones

Use `operation: "rest"` with a `changes` array. Each entry has a bone `id`
and any of `position`, `rotation` or `scale`, each with three components.
The native rest editor compensates skin bindings and rigid geometry so the
mesh retains its bind-space shape. The whole array is rejected if a transform
cannot be represented safely. This changes the reference skeleton; use the
[animation tools](agent_animation.md) to change animated poses instead.

Use `operation: "rig"` to add or edit authored extra bones:

| `rig_operation` | Arguments and result |
| --- | --- |
| `add` | Supply a stable `parent_id` and a unique `name`. Returns `result_bone_id`. |
| `reparent` | Supply stable `bone_id` and `parent_id`. Keeps the bone's world rest transform. |
| `remove` | Supply stable `bone_id`. Reparents children while preserving their rest positions. |

Required base bones remain part of the fighter's skeleton. Removing an extra
bone requires clearing its asset and behavior references first. Successful rig
edits update the matching fighter, animation, costume and rig facts together.

## Paint skin weights

Use `operation: "paint"` with `mesh`, `bone`, `centers`, `radius`, `strength`
and optional `subtract`. Centers are three-component bind-space positions.
One call applies the entire brush stroke and returns `affected_vertices`.
The native skin editor normalizes the result and encodes its influence groups.
Use the `vertices` view to inspect the resulting weights and positions.

## Materials and textures

Use `operation: "material"` with a material `id` and changed `properties`.
The `info` view includes the complete editable property schema. `materials`
returns their current values.

- `ambient`, `diffuse` and `specular` are four-component byte colors.
- `alpha` controls opacity from zero to one. `shininess` controls highlights.
- Named lighting, depth and transparency flags match the Model inspector.
- `color_source`, `opacity_source` and eight `texture_layers` choose inputs.
- `pixel_engine.enabled` enables custom blending, alpha and depth settings.
  Its other fields use the native GPU enum values listed in the schema.

Rendering changes update the material and its owning joints' draw passes.
The no-depth-write render flag requires a translucent surface. For custom
opaque depth settings, enable `pixel_engine` and change `write_depth` there.
The reply includes `requested_properties` and `applied_properties`.

Use `operation: "mesh_material"` with inspected `mesh` and `material` IDs to
assign an existing material. Use `operation: "texture_image"` with a
`material` and PNG/JPEG `path` to embed an image. `layer` defaults to zero.
For an animated texture, inspect `texture_sequences`, then supply its sequence
`id` as `sequence` and a zero-based `image` index.

Material assignment preserves texture animations and fighter expression
references. If the assignment would remove a referenced texture, edit the
current material's properties or images instead. Reinspect material IDs after
assignment; the list contains the materials currently used by meshes.

Use `operation: "texture"` with `material`, an optional zero-based `layer`
and a `texture` object containing changed fields. The `textures` view returns
the current properties, and `info.texture_fields` contains their schema. The
reply includes `requested_texture` and the complete `applied_texture`.

| Field | Meaning |
| --- | --- |
| `animation_id` | Identity used to match this layer's texture animations. |
| `coordinate_source` | Position (0), normal (1), binormal (2), tangent (3), UV sets (4–11), generated coordinates (12–18), or vertex colors (19–20). |
| `mapping` | UV (0), reflection (1), highlight (2), shadow (3), toon (4), gradation (5), or backlight (6). |
| `rotation`, `scale`, `offset` | Three-component texture transform. Rotation uses radians. |
| `color_operation` | Disabled (0), alpha mask (1), RGB mask (2), blend (3), multiply (4), replace (5), pass through (6), add (7), or subtract (8). |
| `alpha_operation` | Disabled (0), alpha mask (1), blend (2), multiply (3), replace (4), pass through (5), add (6), or subtract (7). |
| `blend` | Amount used by the texture operation. |
| `wrap_u`, `wrap_v` | Clamp (0), repeat (1), or mirror (2). |
| `repeat_u`, `repeat_v` | Texture repeat counts, from 1 to 255. |
| `magnification` | Nearest (0) or linear (1) sampling. |
| Contribution flags | Select diffuse, specular, ambient, extended, shadow or bump use. |

The nested `lod` object enables custom mipmap filtering with `enabled`. It
contains `minification`, `bias`, `anisotropy`, `clamp_bias` and `edge_lod`.
Minification values 0–5 select nearest, linear, nearest/nearest mip,
linear/nearest mip, nearest/linear mip and linear/linear mip. Anisotropy values
0–2 select 1×, 2× and 4×.

`animation_id_editable` on each texture row reports whether its animation ID
can change. Animated costumes keep their existing IDs, which the engine uses
as render slots. Replace their images or edit material properties to change
their appearance. Texture-sequence inspection returns both the animation ID
and its matching layer index.

The nested `combiner` object enables custom texture expressions with
`enabled`. `color_expression` and `alpha_expression` activate its two
expressions. Each has an operation, bias, scale, clamp flag and four inputs.
Operations are add (0), subtract (1), or native comparison operations (8–15).
Bias selects zero (0), add half (1), or subtract half (2). Scale selects
1× (0), 2× (1), 4× (2), or 0.5× (3).

`color_inputs` contains four input IDs: texture RGB/alpha (8–9),
one (12), half (13), zero (15), constant RGB or its
R/G/B/A channels (128–132), or texture registers 0–1 RGB/alpha (133–136).
`alpha_inputs` selects texture
alpha (4), zero (7), constant
R/G/B/A (64–67), or texture registers 0–1 alpha (68–69).

`constant`, `register0` and `register1` are four-component byte colors.
The twelve `overrides` flags select their individual RGBA channel overrides
in that order. Values in a disabled `lod` or `combiner` use the engine defaults
until the corresponding custom block is enabled. UV mapping requires the
referenced UV set on every mesh using that material; reflection mapping can
use geometry without UV coordinates.

## Choose the Model view

Use `session.models.view` to read the current Model workspace. Its `model_view`
contains the selected mesh, material and bones, hidden meshes, preview mode,
playhead and IDs of temporarily posed bones. Add `include_pose: true` to read
their local transforms too. These values describe the open editor.

Use `session.models.select` with the live `revision` to change them:

| Field | Meaning |
| --- | --- |
| `context` | `meshes`, `materials`, `skeleton` or `skinning`. |
| `mode` | `edit` for the rest rig, `pose` for a temporary pose, or `animation` for clip playback. |
| `mesh`, `material` | An inspected ID, or `-1` to clear that selection. Selecting a mesh also selects its material. |
| `bones` | The complete selected-bone list. An empty list clears it. |
| `active_bone` | The bone shown in the inspector and used for skin painting. |
| `hidden_meshes` | The complete list of meshes to hide in the editor. An empty list unhides them. |
| `bones_visible` | Show bone markers. Skeleton context always draws them; `bones_drawn` reports the result. |
| `action_clip` | An available action ID from the fighter's action list. |
| `frame` | Integer frame in the selected clip, clamped to its available range. |
| `playing` | Play or pause the selected clip in Animation Mode. |

Selecting a new clip or frame pauses playback unless you also send
`playing: true`. Playback advances the pose at 60 frames per second while the
practice simulation stays frozen. Its advancing playhead leaves the editor
revision stable, so a pause request can use the revision inspected during play.
Manual selection, seeking and visibility changes produce a new revision.

Selection and hidden meshes belong to this member's editor view. They leave
fighter files, installed packages and the costume's authored visibility alone.
Use `session.animations.preview` to read the actual actor's applied clip, frame
and bone transforms after the view reaches it.

Seeking clamps to the last whole frame inside the clip. During playback the
playhead advances while the view revision stays stable, so you can pause using
the revision you inspected. Changing selection, seeking, pausing or editing a
temporary pose updates the revision.

## Make a temporary pose

Use `session.models.pose` with a `changes` array. Each entry identifies a bone
and any of its local `position`, `rotation` and `scale`. Positions use game
units, rotations use radians, and scales use multipliers. One request applies
the whole batch; invalid or repeated bones reject the whole batch.

This opens Pose Mode and changes the frozen character. Send `reset: true` to
return to the rest pose. Temporary poses affect the view and its revision;
they leave the saved rest rig and animation keys unchanged. Use the rest-edit
or animation-key tools when you want to author that change into the fighter.

## Names, history and saving

Use `operation: "label"` with `category` (`bone`, `mesh` or `material`), the
inspected `id` and a `name`. An empty name restores the derived label.

Each edit stages a complete candidate and validates its assets before applying
it. Failure preserves the original draft. Saved edits hold the draft lease and
write through the family recovery journal. Live edits create one Model Undo
step, preserve saved files and request a frozen-preview reload. Check
`preview_status`, then inspect the session until preview loading finishes.

Model history restores matching assets and source metadata together. Undo
refuses to overwrite an intervening edit from another workspace. Save or build
to persist live changes, then install the resulting package when ready to play.
