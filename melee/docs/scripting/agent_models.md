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
| `vertices` | Bind-space positions and normalized bone weights. Supply `mesh`. |
| `sources` | Imported meshes, source paths, source identities, scale and binding choices. |
| `texture_sequences` | A material's animated texture sequences and their image counts. Supply `material`. |
| `reimport_plan` | Source primitives matched to existing meshes. Supply `mesh`, `path` and optional `whole_source`. |

Names can be filtered with `query` in the bones, meshes, materials and sources
views. Vertex positions use game units in the model's bind space. Local bone
rotations use radians; scales use multipliers. Weights range from zero to one.

The `id` values for bones, meshes, materials, vertices and texture sequences
identify entries in the inspected model. Resolve them again after importing or
changing the hierarchy. Each bone also has a stable `bone_id`, used by the
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

Advanced texture transforms, filtering, LOD and texture-combiner properties
remain available through the Model inspector; structured tools for those
properties are still being added.

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
