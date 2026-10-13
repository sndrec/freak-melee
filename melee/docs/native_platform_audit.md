# Native platform integration audit

> Historical platform bring-up audit. Its initial unresolved-symbol counts and task list describe the recorded build, not current port status. Start with [current build instructions](native_build.md).

## Current boundary

The full game/HSD source tree compiles into Windows x86-64 objects. This is
not yet a game executable. The `melee_platform` target builds the supplied Aurora
libraries separately within the same CMake build so that dependency failures can
be distinguished from integration failures.

The object-symbol audit currently finds 333 external names after subtracting
definitions provided by other game/HSD objects. This includes ordinary CRT
imports and platform APIs, not 333 missing gameplay functions. The local evidence
is `build/native/object-symbols.txt` and `build/native/unresolved-game-symbols.txt`.
The count is a snapshot before linking Aurora and the Windows runtime.

## Integration work before first boot

- Connect Aurora startup and frame processing to the existing game entry point
  in `src/melee/gm/gmmain.c`. Retain the existing game initialization and scene
  progression rather than replacing them with a second gameplay implementation.
- Audit declarations and shared structures at the game/Aurora boundary before
  linking them. Matching symbol names alone cannot establish compatible argument
  widths, callback types, or data layouts.
- Replace console-only CPU context, interrupt, stack, and memory assumptions with
  explicit native behavior at their call sites. Do not fill missing functions
  with success-returning stubs just to obtain an executable.
- Supply the audio execution path. The object audit includes AI, AX, AXFX, and
  AR/ARQ references; Aurora's AR storage implementation does not establish that
  Melee's voice mixer and effects are operational.
- Repair pointer truncation in live allocations, object references, and loading.
  Treat serialized GameCube offsets separately from native pointers; widening
  every integer in the original asset layouts would corrupt those layouts.
- Decode original big-endian asset records into the native runtime structures.
  Font byte extraction is already validated but does not cover general DAT
  relocation, display lists, animations, or audio assets.

## Concrete ABI example

The decomp's `extern/dolphin/include/dolphin/ar.h` declares `ARQPostRequest`
source/destination arguments as `u32`. Aurora's `include/dolphin/ar.h` takes
`uintptr_t` there so a main-memory address can survive a 64-bit call. Their
callback declarations also differ: the decomp uses `struct ARQRequest*`, while
Aurora uses `uintptr_t`. The actual call sites and declarations need a consistent
native contract before audio transfers can work. A successful library build
cannot detect or repair this cross-translation-unit mismatch.

## Verification boundaries

Compile coverage, platform library builds, and the reporting unit test are build
evidence. First boot, local versus behavior, asset correctness, audio, frame
timing, determinism, and rollback still require their own runtime acceptance.

## Remaining graphics CPU cache calls removed (2026-09-12)

Removed the last game/HSD DCFlushRange calls in refraction texture generation,
snapshot decompression and the legacy debug framebuffer loop, plus locals and
the cache header made unused by those removals. No cache API stub was added.
Refraction finishes its CPU buffer before constructing/loading its texture.
Aurora resolves texture source bytes and uploads through its texture cache
(lib/gx/texture.cpp); console CPU cache maintenance does not submit a native
GPU upload. This change does not add dynamic texture invalidation or repair the
legacy debug framebuffer presentation path.

The default native build and all 22 native tests pass. A fresh full-client
link reports 11 unresolved symbols, down from 12, with DCFlushRange absent.
Evidence: build/native/cache-removal-build.log and client-link.log. The existing
tests do not verify snapshot display or refraction in a running match; full
client boot and those visual paths remain unverified.

## Movie clock no longer requires console alarms (2026-09-12)

The movie player's existing per-scene update now polls SDL's monotonic clock
and advances its original rate-table/frame-buffer logic at 60 Hz. Start rebases
the clock; inactive players do not advance. Catch-up is bounded to 60 ticks
after a long stall/suspension, discarding older presentation backlog. This is
a deliberate bounded movie-presentation policy, not a gameplay timing change.

Movie decoding, cursor updates and DevCom completions are owned by the game
thread, so the old interrupt guards were removed from that player. Close still
stops reads and drains the outstanding DevCom request before freeing buffers.
The unused alarm callback signature, console alarm object and anti-inlining
poll helper are gone. No source caller still needs OS alarms, so main no longer
initializes that console service.

The default build and 22 existing native tests pass. Full-client linking now
reports seven unresolved symbols (audio initialization/control and interrupt
guards elsewhere), down from eleven. Evidence: movie-clock-build.log and
client-link.log. Existing tests do not exercise movie timing/playback, looping,
or audio synchronization. These remain runtime acceptance gaps; native versus
boot is still the primary target and has not been reached.

## Game-owned preload and rumble state (2026-09-12)

Removed interrupt guards and their saved-status plumbing from lbdvd, lbheap,
lbfile's file-size query, and rumble commands. Preload entries and heap changes
run on the game thread; DevCom dispatches their file completions through its
game-thread poll. Rumble interpretation runs inside HSD_PadRenewRawStatus,
which the native game-thread input clock calls. No mutex or dummy interrupt
implementation was substituted. Preload request/cleanup and rumble algorithms
were left intact.

The default build and 22 existing native tests pass, including the established
DevCom owner-thread, native memory and input-timing tests. These are supporting
subsystem checks, not end-to-end versus preload or physical-controller rumble
acceptance. Evidence: preload-owner-build.log and client-link.log. The complete
client still fails to link with seven unresolved symbols: the remaining audio,
card, video and debug interrupt users require further ownership review.

## Synchronous native card control operations (2026-09-12)

Aurora card.cpp implements most async entry points with inline callbacks,
and CARDMountAsync returns success without invoking its attach callback.
The original lbcardnew code increased its pending count after submission: a
successful mount could wait forever, and an inline error callback could leave
a negative pending count interpreted as still busy. Console interrupt guards
cannot correct either native callback-ordering mismatch.

Mount/check/format/delete/rename now call Aurora's synchronous card operations
directly, preserve the existing error conversion and mounted-state rules, and
return with no pending operation. Removed the now-unused completion callback
and all lbcardnew interrupt guards. The rename declaration is const-correct to
match Aurora's implementation. The higher-level queued read/write/create/status
code in hsd_3A94.c still assumes deferred callbacks and remains unfinished.

The default build and 22 existing tests pass; a fresh full-client link still
has seven unresolved symbols. Evidence: card-sync-build.log and client-link.log.
These tests do not exercise card control or persistence. No real card files
were created, deleted, renamed or formatted during this change. The identified
control-flow stalls are removed at source level, but startup and card I/O need
runtime acceptance after the remaining client dependencies are implemented.

## Card command completion ordering (2026-09-12)

The HSD card queue now uses synchronous read/write/create/status operations
with its existing bounded busy retries. Each of the eleven submission sites
handles I/O failure through its existing error path, or calls the internal
completion routine explicitly after success. Completion clears the command
and busy state and advances the queue. No inline backend callback can run
before the queue sets busy and then have its result overwritten by the caller.
The unused callback channel argument was removed; its reused local byte count
was named tail_size. All interrupt guards in this game-owned queue are removed.

This does not finish card nativization. The module still contains raw 32-bit
command indexing, fixed offsets and pointer truncation, including accesses
through CMD_STATE/CMD_S32 and the secondary queue. Those are known runtime
blockers requiring typed native state or replacement by native persistence;
compilation is not evidence that save/load works.

The default build and all 22 existing native tests pass. A fresh full-client
link still reports seven unresolved audio/interrupt symbols. Evidence:
card-queue-build.log and client-link.log. No card queue runtime test or real
card-file operation was performed; complete client boot remains unfinished.

## Final interrupt dependencies removed (2026-09-12)

The synth/driver voice state and DevCom sound-bank completions are game-thread
owned. Removed their console interrupt guards and the unused interrupt-state
return from HSD_Synth_8038A000. The native frame scheduler must invoke the synth
callback on this owner thread, never from a device callback. Existing synth
loader/stealing tests no longer need interrupt stubs.

The separate audio-command DVD read is an exception: Aurora completes it on
its worker. Its result now crosses threads through release/acquire atomics;
both success and failure terminate the wait. Submission/short-read errors free
the read buffer and report failure, and zero-length files close the handle.
The raw command metadata still needs endian/native-pointer conversion.

Removed remaining guards from the native-owned VI state queries and obsolete
console context switching around diagnostic FPR data printing. Native video
has no CPU XFB producer. These changes do not restore the legacy CPU framebuffer
debugger or add movie/card runtime acceptance.

The default build and 22 native tests pass. A fresh full-client link reports
five unresolved symbols: AIInit, AISetStreamVolLeft, AISetStreamVolRight,
AISetDSPSampleRate and AXRegisterCallback. Evidence: audio-owner-build.log
and client-link.log. OS interrupt APIs have no remaining game/HSD source
references. Audio scheduling/device output and full versus boot are unfinished.
