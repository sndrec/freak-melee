# Model rest-edit checkpoint — 2026-10-17

Rest edits now compensate rigid attachments and ordinary skinned polygons in
an asset transaction. The release client and content tool build successfully.
Ordinary rigid/skinned rest edits pass the current viewport and workflow checks.
Morph-target compensation remains open.

## Behavior

Changing a bone's rest translation, rotation or scale updates its descendants'
inverse binds. Rigid polygons receive compensating positions and normals. The
same geometry compensation covers HSD's single-bone envelope path, which can
skip inverse binds, and the model-node matrix used by non-root mesh owners.

The transaction clones affected polygon chains and vertex data. Other meshes
retain their shared buffers. Positions and normals become direct floats;
colors, UVs, topology, materials, matrix-slot assignments and mesh indices stay
intact. Normal compensation uses the inverse transpose, including under
nonuniform scaling. Shared inverse-bind records are copied before editing.

The existing costume undo history stores the complete expanded asset. Failed
compensation publishes no asset changes. The caller restores its initial bone
edit. Affected morph targets reject the operation pending their own support.
Inherited animation clips still need review after changing the rest rig.

## Verification

- `native_model_rest`: 648 combinations and 2,158,492 checks pass. Independent
  world-space evaluation covers direct/index8/index16 attributes, signed
  quantized/float components, XY/XYZ positions, XYZ/NBT/NBT3 normals, hardware
  attribute order, both rigid slots, root/non-root envelope owners, single/blend
  envelopes, shared buffers/chains/binds, parent/child edits, translation,
  rotation, nonuniform scale, reopening and atomic morph rejection.
- `native_skin_weights` also passes.
- `review_native_rest.py` imports an asymmetric rigid mesh into an isolated
  Mario draft, saves and reopens three rest edits, and rejects an invalid scale
  without changing saved bytes. Native asset import validation runs on save.
- `.local/native-rest-review-2/passed.json` records native Apply, Save,
  byte-exact Undo/Redo of resized assets, Pose/Animation modes, Test Character
  and return to the editor. Whole-window PNGs were reviewed. This run predates
  the final handling of an unchanged singular envelope matrix.
- The final build passes the same unit suite. Its native review at
  `.local/native-rest-review-3` passes saved-asset checks, but stops at the
  stronger before/after viewport comparison: 8,098 pixels differ by more than
  eight color levels. The screenshots show Mario retaining his rest shape and
  the rigid quad retaining its position, with a small apparent viewport shift.
  The cause is **not established**. Investigate camera/preview refresh and
  confirm geometry rather than relaxing the assertion. Later Undo/Redo and
  practice steps were not reached in this final run.

## Refresh fix and current acceptance — 2026-10-18

The current reproduction in `.local/native-rest-camera-review-1` identifies
the shift: the frozen fighter drops from Y=-0.095 to Y=-0.190, and the camera
target follows by the same amount. FOV, aspect and orbit are unchanged.
The comparison reports 6,619 changed pixels.

Preparing a cached preview reselected the already-selected family member,
releasing its frozen pose. Requesting an asset refresh also resumed its
processes before scene teardown. One physics tick then ran during the refresh.
Selecting the same member now retains the pose. Pose release explicitly chooses
whether gameplay should resume; asset refresh leaves the outgoing actor paused.
The camera target and orbit settings are retained through that refresh.

`.local/native-rest-refresh-review-2/passed.json` passes saved rotation, scale
and translation edits, invalid-scale rejection, native Apply/Save, byte-exact
Undo/Redo of the expanded asset, Pose/Animation modes, Test Character and editor
return. The before/after comparison has **zero changed pixels**. Actor position,
camera target, eye, interest, FOV and aspect match exactly in the captured
`surface-images.json` reports. Viewport and practice PNGs were inspected.

The actual pose/companion release fixture passes 2,755 checks in
`A:/FreakMelee-test-archives/rest-refresh-pose-2`, including a release that leaves
both actors paused. The existing model-rest and skin-weight CTests pass in
`.local/native-rest-refresh-tests-1.log`. Native build:
`.local/native-rest-refresh-build-2.log`.

An additional older headless Action/Demo clip fixture could not run against the
current editor: its extractor still names the removed Select visible button,
and its standalone build lacks newer editor boundaries. It is not counted as
passing evidence here. Its pose-release stub was updated for the explicit
resume argument; broader fixture maintenance remains separate.

## Remaining work

`review_native_rest.py` saves camera/actor reports in `surface-images.json`
alongside its full PNGs. Test clients are hidden, muted, isolated and terminated
by the harness. User usability review and inherited-animation retargeting remain
separate from the rest-geometry acceptance above.

Morph targets, inherited animation retargeting, extra bones and source-aware
reimport remain separate tasks. Rebuilt records currently retain old storage;
DAT compaction is still in the Model backlog. All changes here happen while
editing assets, with no added per-tick allocation or gameplay behavior change.
