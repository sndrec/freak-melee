# Workshop: current capabilities and remaining work

Updated: 2026-10-13. This is a current summary, not a chronological log.
Use the [project plan](../../PROJECT_PLAN.md) for priorities and the
[approved UI](native_character_workshop_ui.md) / [Model design](native_model_workspace_design.md)
for the agreed layout. Historical bring-up is preserved in the pre-cleanup Git
commit and [dated evidence](checkpoint_index.md); do not restart it as new work.

## Implemented baseline

The sidebar Workshop opens a full-window draft browser; Collection lists local
drafts separately from installed packages and can open them directly. Both
entry points create/open independent fighter projects. Copied data, clips,
costumes, facts, parameters, behavior and entity resources belong to the draft.
Source provenance is not a live clone-behavior dependency. Save, frozen preview,
Test Character, cache and installation remain distinct operations. Opening a
draft uses its current in-memory source/parameter values to prepare the frozen
preview automatically, without requiring installation or a prior Build. Test
Character enters ordinary grid practice from CSS or frozen editing. Invalid
candidates leave the existing preview intact.

Attributes exposes the entire typed common-attribute record (97 values), grouped
by purpose, including integer fields and the throw mask. Unknown stock fields
are labelled as undocumented. Scripting has wider sidebars, collapsible typed
parameter groups, separate source defaults/overrides with Reset, syntax coloring
aligned to the real text input, and right-click symbol lookup into the bundled
reference. Editing an override affects the current preview/Test candidate; Save
parameters persists it to the draft.

New custom CSS portraits/tile names, home heading and results names/banner are
GX scene content, following the menu camera/anchor transforms. Workshop and
sidebars remain native Rml UI. Context-specific atlases replace the old static
Rml labels; reconstructed glyph coverage and typography remain user visual
review, not a claim of pixel-perfect matching. See [font inputs](../assets/fonts/README.md).

| Workspace | Current tools |
| --- | --- |
| Action Editor | Source-linked subaction/event list and expanded timeline, per-hitbox lanes/properties, playback flags, frame seeking, frozen-pose preview, hit/hurtbox displays and viewport picking. |
| Scripting | Separate move files and editable fighter-state source, parameter/variable controls, typed diagnostics, source checking and bytecode-backed behavior publication. |
| Animation | Bone hierarchy, shared curves, keys/tangents, playback/scrubbing, pose transfer, G/R/S transforms, pivot/axis/plane choices and explicit key insertion / Auto Key off by default. |
| Model | Mesh/material/skeleton/skinning contexts, Edit/Pose/Animation modes, cgltf import review, rigid/generated/transfer binding, initial painting and skinned rest-bind compensation. |

Model usability additions include mesh hover highlighting, viewport selection,
editor-only eye/H/Alt+H visibility, always-visible skeletons, weight heatmaps and
named render/blend/depth/alpha controls. These exist; they are not a substitute
for the missing import/rigging capabilities below or user usability acceptance.

Frozen editing uses a character-centered void preview with axes and camera
controls, without match HUD. Test Character restarts normal grid practice with
the edited fighter and Sandbag. Edits must not partially replace active match
resources. Paired projects use a member dropdown and own separate programs.
The recorded Popo/Nana preview smoke selects actual source-owned motions;
both meshes still appear in frozen preview, a remaining presentation task.

## Interpreter and template status

Ordinary Check/Build invokes the direct source validator and prepares owned
assets/metadata; it does not emit a behavior DLL. Programs are pinned during
gameplay and replaced coherently at scene transitions. The separate generated-C
exporter is a comparison oracle only. The ordinary source build tool still needs
to be present beside the game, including in prepared releases.

All 26 supported stock forms excluding Kirby prepare through the normal
source-package route. CPU, special-entry, selected follow-through/interactions,
paired-family and results comparisons have recorded coverage. See
[roster status](SCRIPT_PORT_CHECKPOINT.md), not the old initial-Mario task list.
Rook has an interpreted project and actual jump/snack gameplay evidence; her
interactive edit experience and complete recipe presentation remain separate
acceptance. Preserve the user's deliberately unfinished character behavior.

## Required implementation gaps

1. Compatible source skin-weight import.
2. Extra-bone authoring with coherent rig facts, references and gameplay roles.
3. Rigid attachment rest-edit compensation.
4. Source-aware reimport retaining deliberate project edits.
5. Wider painting support and storage suitable for useful imported models.
6. Selected-member-only frozen preview for paired fighters.
7. Complete release payload and isolated source-only edit/install/sharing flow.

Arbitrary foreign armature conversion, mesh modeling and whole-move copying are
not gates for the current Model/editor pass. Keep native animation and rig-role
constraints explicit; inverse-bind updates alone do not retarget old clips.

## Collaborative acceptance

The user will review the interface with real projects, especially Rook. Check a
complete clone/edit/test/save/reopen/install session, including undo/recovery,
simultaneous/looped hitboxes, source diagnostics, bone posing, curve edits,
root motion, attachments, import/painting and Blender animation interchange.
Keep source and destination ownership and untouched tracks intact.

No historical fixture or synthetic mockup proves the full user workflow.
Do not turn every scoped correction into a lengthy broad runtime campaign;
use focused checks where necessary and let the user evaluate usability.
Discuss genuine engine/mockup mismatches before changing the approved layout.

## First recording feedback: ready for review

The eight actionable areas from the 2026-10-13 recording now have working
implementations: automatic draft preview and live testing, the sidebar Workshop
browser, Collection drafts, input and resource labels, complete common
attributes, typed parameter groups, script colors and symbol lookup, and
scene-rendered custom menu visuals.

Focused checks used an isolated local-data directory and a fresh, uninstalled
Ganondorf draft. They covered opening from the sidebar and entering a frozen
pose, switching to Scripting, syntax colors and right-click `command` lookup,
common-attribute controls, parameter editing, and Test Character entering grid
practice with the sandbag. Collection's local-draft section was also checked.
The attribute tool's 97-value read and typed-write checks preserved unrelated
DAT bytes and rejected invalid numeric values.

Menu text and portraits draw during Melee's translucent menu pass and follow
the scene's camera and joint transforms. The CSS heading was checked in the
actual game. CSS tile labels now use the supplied angular SVG outlines. Font
checks covered atlas records, printable ASCII, letter counters and punctuation
placement. Results-label placement, the reconstructed results typefaces, and
the complete interactive workflow remain part of the user's next visual and
usability pass.

The scripting reference now starts with writing moves and using the Workshop.
Aerial actions and command variables explain their behavior and values in plain
language. Future reference changes follow the
[documentation guidelines](documentation_guidelines.md).

Local build and check artifacts are under
`C:/FreakMelee-review/2026-10-13-workshop/`. The Release native client,
content tool and control tool build successfully. Tests were hidden and muted;
test projects and screenshots are outside the repository.

## Second recording feedback: ready for review

The [round-two checklist](native_workshop_feedback_2026_10_13.md) records the
feedback and focused verification. Add Event inserts at the playhead, including
actions with no existing script. Meaningful commands have names, descriptions
and editable fields. Timeline markers clip their labels and show full names
on hover; the inspector follows the current selection.

Animation pose values update the preview as they change. Each channel has a
Key button, and I over a numeric property inserts that channel's key. Enter
applies clip length. Graph G/R/S transforms preview continuously and support
confirmation, cancellation and axis constraints. The graph grid follows its
frame/value coordinates. Undo and redo restore data and the preview, including
temporary poses. Gizmos use solid arrows, rotation rings and scale handles,
with world/local controls.

Model Edit/Pose modes show every mesh allowed by the editor eye controls,
including normally hidden accessories. Animation playback and Test Character
use authored visibility. Materials expose lighting colors, render flags,
blending/depth settings and attached texture layers, including texture
transforms, filtering and custom color/alpha operations. Common attributes
have readable labels, units and descriptions. The CSS heading uses the supplied
FolkPro face at a smaller size.

This pass is ready for the user's next usability review. Authoring IK remains
planned separately. Focused tests used an isolated Ganondorf draft and hidden,
muted clients; user projects were preserved.

## Evidence entry points

- [Source-owned member preview](native_source_workshop_preview_checkpoint.md).
- [Family authoring](native_source_family_authoring_checkpoint.md).
- [Interpreted Rook](native_rook_interpreter_checkpoint.md).
- [Fighter comparison coverage](SCRIPT_PORT_CHECKPOINT.md).
- [Current rollback](native_rollback_current.md), including authored state.
- [Scripting guide](scripting/index.md).
