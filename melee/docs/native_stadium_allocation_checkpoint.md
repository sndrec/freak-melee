# Pokemon Stadium transformation allocation — 2026-10-17

Status: reproduced and traced. The runtime fix remains open.

During the stage-page acceptance work, both clients in
`.local/native-quick-stage-singles-2` aborted with:

```text
[os-heap] allocation refused during retained history: heap=1
```

The earlier note called stage index 4 Battlefield. In the Quick Play stage table,
index 4 is Pokemon Stadium. Its first transformation explains the delayed abort;
the jump inputs used by the test are incidental to that allocation.

## Reproduction and caller

`review_native_quick_rematch.py` now accepts `--next-stage`, `--rematch-seconds`
and `--jump-in-place`. `.local/native-battlefield-allocation-1` is the original
directory name for the traced run; despite its name, the requested rematch stage
was index 4, Pokemon Stadium. Two hidden, muted clients finished the first match,
used the native stage page, and entered the same-room rematch. The test requested
95 seconds of gameplay with repeated jump/landing inputs.

CDB attached to the first test client with the current native PDB and a breakpoint
on `ucrtbase!abort`. The second client exited with code 3; the first stopped at
the same heap guard. Its stack in `allocator-cdb.log` establishes this call path:

```text
grStadium_801D1520
  grStadium_UpdateTransformation
    grStadium_TryImportTransformation
      grDatFiles_801C6478
        grDatFiles_ImportNativeAssets
          lbHeap_80015BD0
            HSD_MemAlloc
              OSAllocFromHeap
                historyMutation
```

This is an asset import during gameplay, after the OS heap has been locked for
retained snapshots. The guard is protecting a real ownership boundary. It should
remain enabled. The successful shorter singles/doubles stage-page checks did
not exercise a full Stadium transformation.

## Required fix

Prepare all four transformation DATs during scene construction. Their bytes and
parsed descriptors need stable scene-owned storage. Runtime transitions should
activate those resident assets. Releasing a transformation archive should clear
its active registration while retaining resident storage until scene teardown.

Review both `grdatfiles.c` and `grAnime_801C65B0`, which currently imports and
frees a transformation's native arena on each switch. Also review Stadium's
asynchronous scratch-file loading and its `xC4_b1` completion flag: resident assets
should not depend on file I/O replaying during rollback. Preserve the stage's
selection, transition timers, model/collision behavior and RNG decisions.

Asset descriptors should remain outside copied gameplay state wherever they are
immutable. Any live descriptor mutations and archive registrations must retain
explicit snapshot ownership. Verification needs real transformations, return to
the normal layout, another form, and restore/resimulation across the transition.
Do not treat removal of the allocation abort alone as complete acceptance.

No runtime fix has been applied by this checkpoint. Test clients and their
debugger exited. The original failed logs and traced reproduction remain local.
