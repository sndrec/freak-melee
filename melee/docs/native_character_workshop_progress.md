# Workshop: current capabilities and remaining work

Updated: 2026-10-13. This is a current summary, not a chronological log.
Use the [project plan](../../PROJECT_PLAN.md) for priorities and the
[approved UI](native_character_workshop_ui.md) / [Model design](native_model_workspace_design.md)
for the agreed layout. Historical bring-up is preserved in the pre-cleanup Git
commit and [dated evidence](checkpoint_index.md); do not restart it as new work.

## Implemented baseline

Collection creates/opens independent fighter projects. Copied data, clips,
costumes, facts, parameters, behavior and entity resources belong to the draft.
Source provenance is not a live clone-behavior dependency. Save, frozen preview,
Test Character, cache and installation remain distinct operations.

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

## Evidence entry points

- [Source-owned member preview](native_source_workshop_preview_checkpoint.md).
- [Family authoring](native_source_family_authoring_checkpoint.md).
- [Interpreted Rook](native_rook_interpreter_checkpoint.md).
- [Fighter comparison coverage](SCRIPT_PORT_CHECKPOINT.md).
- [Current rollback](native_rollback_current.md), including authored state.
- [Scripting guide](scripting/index.md).
