# Explicit Action/Demo pose transfer

```powershell
python tests/native/fighter_pose_editor/run.py --output A:/FreakMelee-test-archives/pose-editor-demo-review
```

Actual production Pose Capture/Paste, animation editor, DAT/figatree reader, FObj interpreter and spline code run against synthetic independently named action/demo blobs. Three grouped cases transfer demo→action, action→demo and demo→demo. Each checks interpolated capture and source bank/index identity, root translation exclusion, selected-joint paste, missing-channel counts, structural growth, and a later-channel failure after an earlier channel has modified the private transaction. Failed paste retains both original DAT buffers and does not publish write counts. Empty demo capture/paste rejects without changing the source pose snapshot.

The fixture restores the exact paired buffers supplied to structural history and verifies subsequent sampling. This is a direct buffer-restoration check, not a claim that the UI undo dispatcher or disk transaction ran. Workshop key insertion retains the existing dual-buffer history callback; its selected clip and gesture identity now include Action/Demo explicitly. Existing CLI pose command continues to select Action explicitly. No new motion state is introduced for preview.

CMake: test.c + native_fighter_pose_editor.c + native_fighter_animation_editor.c + lbanim_dat.c + dat.c + fobj.c + spline.c; native console includes src/extern dolphin and MELEE_NATIVE. Allocator stubs abort because sampling uses stack FObjs, not allocated engine objects. No ISO dependency.
