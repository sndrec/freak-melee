# Pokemon Stadium transformation allocation — 2026-10-17

Status: fixed and verified locally. Physical multi-PC testing remains separate.

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

## Implementation

All four transformation DATs are now decoded during scene construction. The
stage archive specification lists its resident fragments and import options.
Their source bytes and parsed descriptors retain stable scene-lifetime addresses.
Runtime transitions activate a prepared fragment. Releasing its archive clears
the active registration while retaining its assets until the scene heap resets.

`grdatfiles.c` owns the bounded resident directory and active registrations.
`grAnime_801C65B0` retains scene-owned assets when it clears an archive slot.
Stadium stores a pending fragment index instead of a scratch-file buffer and
length. The asynchronous file callback and 320 KiB scratch reservation were
removed. Its existing load phase now activates the resident on the next tick.
Selection, RNG decisions and transition timers retain their existing logic.

Resident metadata stays outside copied gameplay state. Descriptor preparation
flags are applied before admission. The four active archive slots are now an
explicit arena root, so rollback restores which fragment each slot references.
Runtime HSD objects continue to use the existing captured pools. The heap guard
remains enabled. Scene reset uses the existing HSD heap destruction/recreation
and clears the resident directory before the next stage is constructed.

## Verification

Both native executables built. `native_stage_data`, `native_stage_start_snapshot`
and `native_confirmed_checkpoint` passed. The stage-data test imports all four
transformation files. The standalone stage-start test also gained its missing
arena implementation in CMake; its admission function already used that owner.

`.local/native-stadium-resident-delay-1` ran two hidden, muted clients through
four minutes of a real Stadium rematch. Both observed the stable sequence:

```text
normal (5) -> rock (6) -> normal (5) -> fire (3) -> normal (5)
```

The reference client observed the first four stable states at frames 23, 4389,
6425 and 10647. The reconstructing client observed them at 38, 4403, 6393 and
10615, with 10,559 restores and 21,118 replay ticks between the first and fourth
observations. More than ten thousand peer checksums had been compared by the
second form. Neither client reported a checksum mismatch, heap guard or panic.
Reviewed PNGs show the rock geometry, transition smoke and returning platform.

`stadium-observations.json` retains phase and checkpoint counters;
`stadium-validated.json` records the captured-state audit and native binary hash
`c76466813e9735f7f1c17ac1d100e0b800b24acc103bc1e982ea4abf471ff08d`.
The original harness incorrectly required both clients to reconstruct by the
second form. One was a non-predicting reference at that point, so that assertion
failed after the full capture. The corrected audit requires matching form order
and checksum progress on both clients, plus reconstruction on at least one peer.
It passes on the retained capture. The requested delay argument is not used as
proof of a fixed prediction depth; the recorded counters establish actual work.

`.local/native-stadium-resident-roundtrip-2/roundtrip.json` passes the scene
lifecycle check in room `AED7E6AE6021`: sessions `6491fbfe7be087de` ->
`de6626a744c3f7f9` -> `cf8ef5745d64806d`. The second and third games both use
Stadium. It finishes two four-stock games with ordinary movement/jump input,
confirms Results, selects the next stage and readies into another match. The
second stage choice uses the existing control command. The previous attempt
tried the pause-menu quit chord, which Quick Play disables; the harness now uses
the supported match-end path.

Both Stadium constructions reported 1,115,126 copied checkpoint bytes and
21,350,824 reserved checkpoint bytes. The decoded model arenas are not copied
into each checkpoint. All test clients exit. Build and focused test logs are
`.local/native-stadium-resident-observe-build.log` and
`.local/native-stadium-resident-tests-fixed.log`.

This establishes the native resident lifecycle and reconstruction through the
observed rock/fire transitions. Grass and water decode in the stage-data test;
this run does not claim their full live transition/render acceptance. The
original failed logs and debugger reproduction remain local.
