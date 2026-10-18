# Model editing workflow — 2026-10-18

## Current behavior

Imported skins can be painted in Edit, Pose and Animation modes. The brush uses
the mesh's current deformed vertex positions for mouse targeting. Each brush
center then uses the corresponding rest position, so it edits the stored weights
without moving the original mesh or writing animation keys. Radius is measured
on the rest mesh. Starting a stroke pauses clip playback.

Temporary local pose edits survive geometry refreshes, paint strokes and
switches between the three rig modes. Changing the joint count or stored joint
identities clears the retained pose. Reset pose sets the temporary pose to the
current rest rig. Saving a fighter saves its assets; the temporary pose remains
an editor-session preview.

Model mode chooses the stock `Wait1` idle clip when available. The previous
lookup only accepted `Wait`, then fell back to the first clip, which was
`WallDamage` in the Mario draft.

`native_shell_workshop_skinning.cpp` owns stroke projection and bind-space
centers. `native_model_view.c` supplies the same posed positions used by the
editor's surface drawing and picking. Its query uses caller-owned output and
adds no drawing or gameplay allocation. Direct positions and a position index
used at conflicting world positions are rejected as ambiguous mappings.

## Verification

The `native_model_view_positions` fixture passes 21 checks for deformed
positions, inverse binds, indexed order, near-1/exact-1 envelope behavior,
capacity, visibility and unsupported shape queries. The existing model-rest and
skin-weight suites also pass.

The release build and all three focused CTests pass in
`.local/native-model-euler-pose-build-1.log`. The hidden/muted native review at
`.local/native-model-workflow-11` passes four functional cases:

- Painting a translated pose changes the intended bind-space vertex. Other
  weights retain their values, and positions/normals retain their exact bytes.
  Save and byte-exact Undo/Redo pass.
- Starting a stroke pauses animation playback. An empty-space stroke preserves
  the costume. Painting the paused clip changes the intended weight and leaves
  the animation bank unchanged.
- A rest edit applies to the painted mesh, and Undo restores its complete asset.
- A fresh client reopens the saved fighter after removing the GLB, enters Test
  Character and returns to the editor. The final source/asset check passes.

The overall receipt is **PASS**, including the original strict projection
check. Untouched vertices retain exactly matching captured screen coordinates
across refresh. The captured local pose values, ancestor world matrices, actor
position and camera also match. Switching from Animation back to Pose restores
the same preview. Viewport PNGs and the live practice capture were reviewed.

## Quaternion/Euler correction

Initial runs detected about 0.17 pixels of movement in an untouched vertex
after painting. Local XYZ rotation values, actor position and camera matched,
but ancestor world matrices differed. Resolving the whole rig first did not
remove the discrepancy; that experimental change was removed.

The runtime capture in `.local/native-model-workflow-10/pose-refresh.json`
identified the cause: Melee animation blending left joints in quaternion mode
(`JOBJ_USE_QUATERNION`). The Workshop wrote three Euler rotation channels while
retaining that flag and the old quaternion W. Reloading a preview produced a
different W, so identical XYZ values produced different rendered rotations.

The live bone reader now converts quaternion rotations to equivalent Euler
angles using HSD's matrix functions. Reading preserves the joint and animation
state. The writer applies Euler channels, clears quaternion mode and initializes
the unused fourth component. This affects native Workshop pose editing; normal
gameplay animation code and saved fighter assets retain their existing behavior.

`tests/native/workshop_bone_pose/run.py` extracts the actual reader/writer and
HSD conversion bodies and links the native Aurora matrix library. Its 1,257
checks compare rendered orientations against independently calculated Euler
matrices, verify read-only queries, vary stale W values across identical edits,
preserve unrelated flags and SRT, and reject invalid input. Actor selection,
part count and joint dirty notifications are controlled boundaries. Receipt:
`.local/native-bone-pose-unit-2/receipt.json`.

`review_native_model_workflow.py --pose-refresh-only` captures the diagnostic
state and labels it `CAPTURED`; full workflow acceptance requires its normal
run. The on-demand inspection includes relevant ancestors, stored temporary
poses and runtime rotation flags for future rig/animation investigations.

## Scope

The brush currently snaps its center to the nearest visible projected vertex
within 48 pixels. Continuous triangle-surface hits and occlusion remain open.
Weight writing uses the imported envelope layout; arbitrary
stock layouts remain in the Model backlog. Morph-target painting, additional
bones, source-aware reimport and DAT compaction also remain separate work.
