# Compact snapshots across match start

This checkpoint established 38 compact components. Index 37 owns the actual pending
stage-start callback list and its started flag. The stage component and callback
component must agree on the phase and canonical list head. Callback targets are
checked against the saved GObj generation/occupancy view before publication.

Stage, hazard, generator and spawn leases bind during scene loading and retain
one epoch across the original READY/start callbacks. Loaded and started stage
definitions are admitted separately; start does not retire loaded history.
Ending/rebinding a scene still retires its captures. Generator snapshots preserve
the dormant source table without following its inactive pointer bits, and admit
the actual driver when the source constructor runs. Spawn definitions admitted
during loading remain available after start. No stage callback is moved earlier.

The versus-control owner also saves the READY/status records, their GObj claims,
cached numeric HUD positions, visibility flags, and crowd-reaction counters.
The source clears the six forced nametag-visibility flags at match start. Omitting
them made replay skip pre-start nametag position updates: gameplay matched, but
36 bytes in model transforms did not. Restoring the flags resolves that mismatch.
Omitting READY's already-fired bit prevented the stage callback from firing again.

Debug text has a lazily created camera. Its pointer and typed class claim are
saved too, including the null pre-first-draw state. Loading a cold snapshot must
clear the later cached pointer before source drawing recreates that camera;
otherwise drawing dereferences its rewound allocation. The camera's class bytes
come from the class companion, not a duplicate payload.

## Evidence

Receipts and full output are under `A:/FreakMelee-test-archives/`.

- `stage-phase-oct12-transition-f`: stock and interpreted Ice Climbers, eight
  actual fighters, 480 inputs each, history captured before every input starting
  at zero; restore boundary 78 and resimulate 15 frames across match start.
  All 38 components, intermediate/continued traces and complete raw endpoints
  match. The fixed history captures 496 boundaries and restores twice per case.
- `stage-phase-oct12-cross-source-b`: Final Destination, Fountain and Green
  Greens, 480 inputs each, mixed Ganondorf/Zelda/Ice Climbers/Game & Watch roster.
  Earlier/current executable traces match. Eight-frame rendered resimulation,
  all 38 compact components and complete raw endpoint comparisons pass.
- `stage-phase-oct12-fountain` and `stage-phase-oct12-greens`: earlier checkpoint
  before the later lazy-camera/crowd additions; stock/interpreted eight-fighter
  pre-input history and 15-frame match-start resimulation pass on both stages.
- `stage-phase-oct12-owners-h`: 86,913 controlled owner/allocator assertions,
  including loaded-to-started lease restoration, pending-to-constructed generator
  restoration, READY once flags, nametag visibility, numeric HUD positions,
  lazy-camera null restoration, crowd counter restoration and malformed inputs.
- `stage-phase-oct12-gm-e`: connected production match-loop/controller/player/
  camera fixture; 2,018 loop checks, 31,797 camera checks and 16 replayed ticks.
- Focused CTests `native_match_history` and `native_stage_start_snapshot` pass.

## Remaining limits

These are correctness tests with real drawing retained and hidden/muted
presentation, not the final 15-resimulation-tick performance benchmark. The
production rollback path still uses broad page snapshots. Diagnostic history
reserves sixteen 8 MiB slots; this is not the final production sizing policy.

`stage-phase-oct12-zero` is a retained frame-zero rewind crash. The debugger
identified the lazy debug-text camera. `stage-phase-oct12-zero-b` no longer
crashes, and gameplay traces match, but one class-backed texture-chain link
still differs (six nonzero address bytes, originally misidentified as a JObj
parent). That receipt remains a failed acceptance case. The subsequent fix and
39-component frame-zero acceptance are documented in
[Material texture roots](native_material_texture_snapshot.md).

The original `stage-phase-oct12-cross-source` identified an omitted crowd
cooldown: all compact records matched but one raw counter byte differed. The
source/PDB correlation and later corrected receipt are retained. Earlier failed
transition receipts show the missing READY and nametag state; none are suppressed.

Complete candidate-wide prevalidation, ordinary resource/graph lifetimes,
additional stages/rosters, output reconciliation, production integration and the
final performance deadline remain open. The full project goal is incomplete.
