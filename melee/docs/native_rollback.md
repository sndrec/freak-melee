# Native rollback snapshots

> Historical page-based rollback implementation, retained as a diagnostic reference. Production uses admitted mutable arenas and confirmed checkpoints; see [current rollback](native_rollback_current.md). The page mechanism below is not the production path.

Approved direction, 2026-10-07: replace broad page capture as the production
mechanism with explicit snapshots of audited mutable match state. Retain the
page system as a development correctness reference while proving the new
boundary. See `native_interpreter_transition.md` for scope, measured costs and
acceptance. This document describes the currently implemented page mechanism;
its existence does not establish a suitable final state size or throughput.

`native_rollback_pages.c` provides the low-level storage primitive for selected
game-owned state regions. The write census measures that boundary, and the
`--rollback-probe` path now restores and resimulates live gameplay against
recorded input.

## Storage model

The caller registers explicit, non-overlapping page-aligned regions and small
exact-copy regions at an explicit state transition. Initialization allocates a
fixed ring of frame records, a fixed number of dirty-page copies per frame,
one generation counter per tracked page, and storage for every exact region in
every frame. No allocation occurs while capturing or restoring frames.

At the start of a simulated frame, the region becomes read-only. The first
write to each page reaches a Windows vectored exception handler, which copies
the page's start-of-frame bytes into the active ring slot and makes that page
writable. Further writes to the same page do not copy it again. Ending the
frame restores read-write protection and accepts the snapshot only when its
dirty-page capacity was sufficient.

Each completed record therefore transforms the state at the start of frame
`N + 1` back to the state at the start of frame `N`. Restoring an older frame
applies those records in reverse order. Applied records are invalidated because
the caller must resimulate and replace that timeline. Restoration is rejected
before modifying memory if any required record has overflowed, expired from
the ring, or otherwise does not identify the requested frame.

Exact regions cover mutable metadata that cannot be page-aligned without also
capturing neighboring host-owned memory. The current set contains only the
Aurora OS-heap descriptor for each tracked game heap, the current-heap selector,
and texture/palette identity counters. It deliberately excludes descriptors for
live host heaps such as audio. Exact bytes are copied at every frame boundary
and restored after the reverse page walk. This keeps allocator ownership and
native renderer object identity synchronized with the game heaps.

Only write access violations in a registered region, on the thread that
initialized the tracker, are handled. Unexpected reads, instruction fetches,
foreign-thread writes, and addresses outside the region continue through the
normal exception chain.

## Intended game boundary

Whole MEM1 capture is not the intended boundary. Aurora's 128 MiB arena also
contains audio, graphics, platform state, immutable assets, unused capacity,
and other data that does not determine the next gameplay frame. Page tracking
will be used only for selected game-owned heaps where restoring allocations and
allocator metadata together preserves internal pointer graphs.

Slippi provides a useful reference boundary. Its `SlippiSavestate` does not
save all emulated RAM or a full Dolphin machine state. It copies about 7.7 MiB
from four selected Melee ranges in a typical match: writable game data/BSS, one
game-managed region, and the active main heap. It subtracts known AX/DSP sound,
XFB/VI, and related presentation ranges, then preserves its live online-control
buffers across a load. The native port can express this ownership directly
rather than relying on retail addresses.

Mutable gameplay globals outside registered heaps must be identified and
migrated into owned match-state structures or registered as small explicit
copy blocks. Copying the executable's complete writable section is not a valid
shortcut because it also contains operating-system, window, renderer, audio,
input, replay, and diagnostics handles.

`tools/audit_native_rollback_state.py` makes that overlap visible from the
native PDB. It attributes writable PE contributions to their object modules,
reports pages shared by separate linked owners, and writes the current audit to
`docs/native_rollback_state_inventory.md`. Regenerate it after a native build:

```powershell
python tools/audit_native_rollback_state.py `
  --pdb build/native/melee_native.pdb `
  --output docs/native_rollback_state_inventory.md
```

This remains an inventory rather than a generated snapshot manifest. The
native build now places mutable globals from game/HSD translation units in the
dedicated `.mstate` PE section. Zero-initialized globals are emitted there as
initialized data so a begin/end sentinel pair provides one exact contiguous
range without PDB parsing at runtime. `nativeRollbackState_GetGlobalRegions`
exposes that range to the snapshot layer.

Original HSD synth/AX-driver state and the native mixer cursor/effect state use
a separate `.maudio` section. The HSD audio heap is also separate from the
match heaps returned by `lbHeap_GetActiveMainMemoryRegions`, so neither audio
engine storage nor audio globals are rewound with gameplay. The source split
keeps crowd behavior and sound-emitter GObjs in game state; only the machinery
that owns live voice IDs, decoded-voice cursors, PCM effect tails, and loaded
sound banks stays on the live audio timeline. This is the native ownership
equivalent of Slippi excluding AX/DSP ranges from its savestate.

Sources that own other host resources remain explicitly outside `.mstate`:
native audio output, replay files and diagnostics, debug control and
screenshots, asynchronous file transfer, logging callbacks, and movie
wall-clock state. HSD's wall-clock performance counters are also host
diagnostics and stay outside the snapshot. `.maudio` is visible in the PDB
inventory for diagnostics but has no runtime registration API and is not part
of the restore manifest.

The sections are ownership mechanisms, not a complete external-effect policy.
Sound starts now use fixed per-frame intent logs. A repeated intent on a
resimulated frame reuses its live AX instance; a newly produced intent starts a
new instance, and an instance from the discarded timeline is stopped when the
corrected frame no longer produces it. Rumble scheduling and list ownership are
restored as game state, while physical motor commands remain on the host
timeline. Motor output is suppressed across hidden resimulation and each local
port receives only the final corrected status at the endpoint. File writes
still need an equivalent policy. Input history stays outside `.mstate` so newly
received remote confirmations survive loading an older game state, matching
Slippi's preserved online-control buffers. The PDB inventory remains useful for
reviewing which translation units moved into each section and which host-state
exclusions remain in `.data`.

## Input ownership and prediction

`native_rollback_input.c` provides a fixed 16-sample, four-port history with no
simulation-time allocation. The session or probe owns that history on the live
host timeline. A local-slot mask supports every required shared-PC layout,
including more than one local player. Each local slot records its own sample. A
missing remote slot repeats its most recent confirmed sample, or starts neutral
when no confirmation exists. Remote confirmations may arrive before their frame
is prepared.

When a confirmation differs from the sample used by a prepared frame, the
history records the earliest affected frame and updates later predicted samples
until the next confirmed input. Repeated confirmations must agree. Following
Slippi's comparison, analog trigger values from zero through 42 are equivalent;
buttons, sticks, trigger values above that deadzone, analog face buttons, and
controller status fields compare exactly. Expired confirmations and attempts to
replace locally owned slots are rejected.

The controller queue exposes one native raw-input filter immediately after
`PADRead`. The exact live probe records four local slots, and resimulation
obtains those samples from the fixed history after the replay file rewinds. The
correction probe instead treats P1 as remote, deliberately withholds one input,
then submits a different confirmed input seven frames later. That exercises the
same earliest-correction request and restore path that the network receiver will
use. Network transport is not connected yet.

`native_rollback_session.c` owns the production orchestration around that input
history. It brackets each simulation/draw pair with page and audio capture,
accepts remote confirmations without placing them in restored memory, restores
the earliest reported mismatch, tracks the abandoned timeline's endpoint, and
suppresses physical rumble until hidden catch-up reaches that endpoint. The
session reports restore and catch-up transitions to the match loop; transport,
replay input, and diagnostics remain callers rather than becoming dependencies
of the rollback core. The diagnostic probe now uses this session instead of
maintaining a separate correction state machine. A focused test covers the
normal frame, mismatching confirmation, restore, corrected input reuse,
catch-up, output suppression, and a deliberate second replay of the interval.

The live `--rollback-census` tracks `.mstate` alongside the game heaps and
reports its dirty pages separately. It also reports OS-allocator allocation
count and payload bytes inside the tracked game heaps at match-loop entry and
exit. That number is only the allocator's live payload, not the required
snapshot size: allocator descriptors, free-list cells, fixed `lbMemory` heaps,
and any pages dirtied during a frame must still be restored together.

A one-stock Final Destination smoke run after separating audio state reached
the results path and sampled 4,412 simulation/draw pairs without page-capacity
overflow. The OS allocator held about 81.4 MiB of payload at match-loop entry,
while simulation changed 261.48 pages per frame on average (24.86 in
`.mstate`) and drawing changed 200.99 pages (12.97 in `.mstate`). The maxima
were 327 total/34 global for simulation and 269 total/14 global for drawing.
This confirms the section is live and bounded.

## Restore-and-replay probe

`--rollback-probe` is an executable integration check. It requires a direct
match and `--replay`, automatically uses accelerated input timing, and forces
one simulation tick plus one complete draw per tracked frame. The snapshot
begins before controller sampling and ends after draw submission and the normal
game frame counters. This captures the input queue and clock together with the
simulation and game-owned state changed during rendering.

At or after frame 240 the probe waits until its most recent eight-frame window
contains a sound start, with frame 600 as a hard fallback. It hashes every byte
in the registered game heaps, `.mstate`, and exact metadata regions, restores
the start of that window, rewinds the replay file to the controller-sample
count saved at that exact boundary, and resimulates eight frames. Intermediate
frames execute the ordinary HSD/GObj draw traversal and submit Aurora's GX work
to the GPU. Aurora does not acquire or present a swapchain image for those
frames. The endpoint hash must match before the process exits successfully.

```powershell
$matchArgs = @('--iso', 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso', '--match', '--stage', 'battlefield', '--p1', 'human:ganondorf', '--p2', 'cpu:zelda:1', '--p3', 'cpu:ice-climbers:1', '--p4', 'cpu:mr-game-and-watch:1', '--mode', 'stock', '--stocks', '1', '--items', 'off')
& .\build\native\melee_native.exe @matchArgs --replay .\build\native\last-match.fmr --rollback-probe
& .\build\native\melee_native.exe @matchArgs --replay .\build\native\last-match.fmr --rollback-correction-probe
```

`--rollback-correction-frame N` selects a later target frame from 240 through
591 for the correction probe. The default remains frame 240. This lets a
recorded move's cue or state transition fall inside the corrected eight-frame
window; the probe still applies the same late P1 A-button change and compares
two full-draw resimulations of the corrected timeline.

For an A/B gameplay comparison across different fighter packages, use
`--replay-inputs FILE` with the new match's own package selection and
`--trace-actions`. This local diagnostic reuses the recording's initial RNG
seed and every controller sample but deliberately skips the match-signature,
state-digest and per-frame RNG comparisons; normal `--replay` remains strict.
It can run with `--fast` and either rollback probe, but not netplay. In an
input-comparison probe, the recorded state digests and RNG timeline are never
used as validation oracles; the probe compares its own restored/resimulated
state within the newly selected fighter package.

The trace labels the new simulation's motion and damage changes, so a first
divergence can be compared at the same input sample without misreporting the
altered build as a valid deterministic replay.

A 720-sample input fixture entered scripted Mario Cape at sample 226. With
`--replay-inputs --rollback-probe`, the active-Cape interval from frame 232 to
240 restored and reproduced all 134,840,356 tracked bytes, reconciling two
sound starts. `--rollback-correction-probe --rollback-correction-frame 240`
then introduced a late P1 A-button correction while Cape was active, restored
frame 240 from 248, and reproduced the corrected endpoint on two full-draw
passes. Both probes exited successfully. This is same-process local evidence;
it does not prove cross-machine determinism or every Cape article transition.
The correction probe now also observes Mario Cape item count, custom ownership,
article pointer and fighter-held handle. With scripted Cape starting at input
sample 242, a correction at frame 240 reproduced the Cape's creation in its
eight-frame interval (0 to 1 item, owned by P2, no binding mismatches). A
correction at frame 270 reproduced normal Cape removal as the action ended
(1 to 0 item, no mismatches). Each corrected endpoint matched on two full-draw
passes. Damage/death cleanup later in Cape is a separate, untested transition.
A third fixture timed stock Mario's Fireball to hit the scripted Cape user one
frame after Cape entry. The item count fell from 1 to 0 in the corrected
frame-240 window, the fighter took 6% damage, and the two corrected full-draw
passes matched with no article-binding mismatch. Without correction, this
720-sample fixture matched stock Mario on all 1,438 logged fighter-state lines.
Later damage and death transitions still require coverage.

The first sound-containing live probe on that four-player lineup restored frame
235 from frame 243, reproduced all 134,602,788 tracked bytes, and exited
successfully. It reused the one AX sound start in the replayed interval without
starting or canceling another instance. The process-local endpoint hash was
`b60e5defc0476339` on that run. Raw full-state hashes differ between processes
because this diagnostic deliberately covers heap bytes containing
ASLR-dependent pointers. It proves equality for the exercised eight-frame
interval within one process.

`--rollback-correction-probe` withholds P1 sample 241, first simulates frame 240
from a neutral prediction, and supplies an A-button correction at frame 248. It
restores frame 240, runs the corrected interval with complete draw callbacks,
captures the endpoint, restores the same frame again, and requires a second
corrected run to reproduce every tracked byte. The Battlefield diagnostic did
so for all 134,602,788 bytes while reconciling 11 stable and 7 newly introduced
sound starts. A separate 2v2 Final Destination replay with level-nine CPUs and
very-high items also reproduced its corrected endpoint, reconciling 3 stable
and 1 newly introduced sound start.

The first corrected run exposed the rumble node pool as mutable state outside
the snapshot. Its linked-list heads were restored, but the fixed pool had been
defined by `gmmain.c`, the executable entry-point object, and therefore remained
in ordinary `.data`. The second pass selected different nodes. The controller
queue and rumble pool now live in a game-state translation unit in `.mstate`;
the corrected endpoint then matched exactly. This is not yet broad rollback
acceptance across fighters, items, stages, deaths, match transitions, longer
delays, or different machines.

The probe also computes an address-normalized diagnostic hash. It replaces
recognized pointers into tracked pages, exact metadata, the executable image,
and registered immutable assets with stable region identities and offsets.
`PlCo.dat` is currently the only immutable allocation outside the game heaps;
its source bytes and imported arena are registered from a content-derived
identity. `FREAK_MELEE_ROLLBACK_HASH_MANIFEST` writes per-page and per-exact-
region digests for comparison. `FREAK_MELEE_ROLLBACK_HASH_WORD_REGION` and
`FREAK_MELEE_ROLLBACK_HASH_WORD_OFFSET` can add word-level evidence for one
page.

This normalized byte scan is deliberately not called a portable gameplay
checksum. Two clean runs of the same Battlefield replay each reproduced their
own corrected endpoint exactly, and all exact metadata digests agreed, but 203
paged digests still differed between the processes. The first isolated value
was inside `HSD_TObj.mtx`, not an external pointer. Normal execution can reach
an endpoint after a different number of presentation passes, and whole arena
pages also contain padding, free blocks, and draw-owned cache fields. A future
cross-peer checksum therefore needs a semantic manifest of state that affects
future simulation, or a proven normalization for each opaque native field.
Blindly ignoring renderer-owned bytes would discard the camera, offscreen, and
other draw-phase state that this probe has already shown must be replayed.

The integration still needs to:

- finish reviewing the `.mstate` inclusions and host-state exclusions against
  Slippi's sound, input, and online-buffer handling;
- finish the portable schemas for fighter motion state beyond the common
  locomotion, fall, squat, landing, basic attack-control, damage, and guard
  families plus knockdown, tech, air-dodge, rebound, platform-drop, and ledge
  state plus item pickup/throw, parasol-open, heavy-item lift, standard grab,
  fighter throw, grabbed wait/damage, standard thrown state, carried victims,
  cargo throws, and the special common capture/item-transition families,
  item-specific state, stage-controller state, allocator pool topology, and
  pending gameplay events; the digest already covers common fighter physics,
  collisions, combat, pose, bury/hammer tail state, persistent per-character
  state and object references, items, grounds, object relationships, joint
  animation, camera state, and RNG;
- repeat restore/replay equality across fighters, items, hazards, deaths,
  match transitions, longer rollback depths, and supported machines;
- exercise corrected timelines where sounds are removed, validate rumble state
  transitions with real local-port routing, and reconcile file writes and other
  external effects;
- return timed-out matches to the future lobby and add NAT/relay connection
  establishment to the direct UDP session.

Slippi's implementation is a useful optimization reference, not evidence that
Melee's drawing phase is cosmetic. Its rollback loop branches back before the
ordinary render, then explicitly runs camera updates and
`Player_SetOffscreenBool` because those operations normally occur after the
main update loop and affect subsequent state. It also carries targeted input
and sound handling around resimulation. Our native port can begin from the
more conservative full-callback boundary, where fidelity is already measured,
and optimize later without changing the original game callbacks.

Full hidden rendering also creates native texture and palette identities. The
game-owned `HSD_TObj` cache embeds those IDs, so replaying draw calls while
leaving Aurora's counters on the abandoned timeline changed four tracked bytes
despite identical gameplay. Those counters are now exact rollback regions.
This handles creation identity for the current proof; rollback across renderer
resource destruction still needs explicit lifetime validation before online
acceptance.

Slippi also excludes its enumerated sound ranges from memory restoration. Its
online code keeps pending and stable per-frame SFX logs, reuses the event log
when a frame is replayed, and stops instances produced by a timeline that the
corrected replay no longer produces. It separately prevents music-file alarms
and avoids restoring Dolphin audio-device state. That is the relevant model for
native external effects: keep SDL audio-device ownership outside `.mstate`,
record game sound intents by frame, and reconcile committed output rather than
rewinding a live host stream. Native rumble now follows the same ownership
split. File writes still need an equivalent policy.

`native_rollback_pages_test` exercises page deduplication, exact-region copies,
multi-frame restoration, capacity overflow, and ring expiry on a small
virtual-memory fixture. `native_audio_rollback_test` exercises stable sound
reuse, corrected new starts, canceled starts, ordering, and bounded overflow.
`native_rollback_input_test` exercises mixed local/remote ownership, neutral and
stale prediction, queued confirmations, earliest mismatch reporting, prediction
propagation, trigger-deadzone equivalence, contradictory confirmations, ring
expiry, and local-slot protection. `native_rumble_output_test` verifies that
host motor commands are suppressed during hidden resimulation and resume for
the committed endpoint.
