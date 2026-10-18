# Model editing workflow — 2026-10-18

## Current behavior

Imported skins can be painted in Edit, Pose and Animation modes. The brush uses
the mesh's current deformed vertex positions for mouse targeting. Each brush
center then uses the corresponding rest position, so it edits the stored weights
without moving the original mesh or writing animation keys. Radius is measured
on the rest mesh. Starting a stroke pauses clip playback.

Temporary poses survive geometry refreshes, paint strokes and switches between
the three rig modes. Changing the joint count or stored joint identities clears
the retained pose. Reset pose
sets the temporary pose to the current rest rig. Saving a fighter saves its
assets; the temporary pose remains an editor-session preview.

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
`.local/native-posed-skin-build-5.log`. The hidden/muted native review at
`.local/native-model-workflow-8` completes four functional cases:

- Painting a translated pose changes the intended bind-space vertex. Other
  weights retain their values, and positions/normals retain their exact bytes.
  Save and byte-exact Undo/Redo pass.
- Starting a stroke pauses animation playback. An empty-space stroke preserves
  the costume. Painting the paused clip changes the intended weight and leaves
  the animation bank unchanged.
- A rest edit applies to the painted mesh, and Undo restores its complete asset.
- A fresh client reopens the saved fighter after removing the GLB, enters Test
  Character and returns to the editor. The final source/asset check passes.

The overall receipt is still **FAIL**. An untouched vertex's normalized screen
X changes from 0.414514840 to 0.414875746 after a paint refresh, about 0.17 pixels
at this viewport width. Switching back to Pose Mode reproduces the same
projection discrepancy. The test retains its strict check; the broader Model
workflow acceptance remains open.

`pose-refresh.json` records identical actor positions and camera reports before
and after. The retained TransN translation is exactly X=6 in both captures.
Weighted bones 35 and 44 also retain their local SRT values, while their world
matrices differ. Check their ancestors and derived matrix state next. Resolving
the whole rig before mesh access did not remove the discrepancy in run 7; that
experimental change was removed. Captured viewport PNGs were reviewed.

## Scope

The brush currently snaps its center to the nearest visible projected vertex
within 48 pixels. Continuous triangle-surface hits and occlusion remain open.
Weight writing uses the imported envelope layout; arbitrary
stock layouts remain in the Model backlog. Morph-target painting, additional
bones, source-aware reimport and DAT compaction also remain separate work.
