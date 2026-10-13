# Native legacy archive scratch no-access proof

The runner mechanically extracts current normal/demo setup, direct native clip
selectors/Nana fallback, actual ftAnim_8006F4C8 track loop and full Fighter unload.
Scratch pointers reference PAGE_NOACCESS mappings: native payload reads/writes
would terminate the console fixture. The actual interpolation joints remain
separate and the same attachment/cleanup calls are observed before/after cleanup.

After the approved native buffer removal:

```powershell
python tests/native/fighter_animation_scratch/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler --output A:/FreakMelee-test-archives/fighter-animation-scratch-check
```

`--expect-legacy-buffers` was used only for the preserved pre-cleanup source
receipt. It is intentionally incompatible with the cleaned-up current source.

Receipts:

* `A:/FreakMelee-test-archives/fighter-animation-scratch-before/receipt.json`:
  73cases/744 assertions,16 buffer allocations/frees.
* `A:/FreakMelee-test-archives/fighter-animation-scratch-after-preprocess/receipt.json`:
  73cases/680 assertions,zero buffer allocation/free, no native legacy allocator
  symbol in preprocessed output. Both have128 track attachments and8 interpolation
  cleanup calls.
* Historical fixed Fighter owner receipt:
  `A:/FreakMelee-test-archives/fighter-animation-scratch-owner-final/receipt.json`,
  73cases/650 assertions,zero scratch allocation/free and the same128/8 calls.

Imported descriptors, allocation, track installation and dependent cleanup
primitives remain controlled boundaries. This is no-access/source ownership
proof, not a real animation decoder/render test or complete Fighter snapshot.
See `docs/native_fighter_snapshot_ownership.md` for real skeleton dependencies
and the future fixed actor/satellite/interpreter owner proposal.

No game is launched. CREATE_NO_WINDOW and fresh A: artifacts preserve commands,
body/source hashes, preprocessed source and console output.

The constructor-cleanup cohort binds the fixture's GObj storage to the actual
NativeMatchPool as well as the actual Fighter owner pool. Full current Unload
checks those original physical references; its unrelated Item/resource/effect
leaves stay controlled here. The fixture retires its GObj explicitly after
Unload, rather than claiming this slice executes actual HSD destruction.
