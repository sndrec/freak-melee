# Bounded rendered compact resimulation

The additional corrected-input and repeated-correction experiment is documented
in [native_compact_input_correction.md](native_compact_input_correction.md).
It compares both corrected intervals against a fresh corrected-input run rather
than requiring a changed timeline to reproduce its predicted endpoint.

Configurable fifteen-frame and repeated later-boundary evidence is documented
in [native_compact_history_experiment.md](native_compact_history_experiment.md).
The original eight-frame receipts below describe their historical checkpoint.

`--resimulate` in the offline snapshot/stage runners enables an eight-tick
experiment using the shared compact capture/load API. The game advances normally
from input sample 240 to 248, preserves the current compact and broad endpoints,
restores only the earlier compact owners, rewinds the existing replay cursor,
then runs the original input, simulation and HSD draw path back to sample 248.
Resimulation suppresses presentation/sidebar updates through the existing hidden
render path; it does not suppress HSD drawing or its RNG/state changes. The broad
endpoint is not used to repair the world before or during those eight ticks.

At catch-up, every compact owner must recapture byte-identically. A separate raw
report compares the complete broad endpoint, and the runners require zero differing
bytes. Exact reference/resimulation trace brackets compare every continuously
traced actor/companion pose, hit/hurt state, inputs, physics, item/resource/event,
RNG and action/damage transition entry over all eight samples. The preserved broad
endpoint is recovered afterward and compared again before normal play continues.
This safeguard is diagnostic recovery, not the compact state representation.

Audio uses the existing production rollback intent journal, keyed by the replay
sample count. Source sound starts reuse their original instance handles. Initial
experiments lacked this hook and differed only in four Fighter.x2160 handles:
seven/eight raw bytes. Those fields were not masked or removed. Same-input tests
require zero new/canceled starts and record actual reuse. The action logger's host
transition history is checkpointed separately, because otherwise the rewind itself
is printed as a reverse action/damage transition. This observer checkpoint is not
included in game state or used to change simulation; all intermediate trace entries
remain compared, without dropped first-frame events.

Early stage experiments exposed two real data/ownership gaps:

- Dynamics computed three Euler angles into a local Quaternion and copied its
  uninitialized W lane into nine Euler joints. Dumped class-piece differences
  were exactly native HSD_JObj.rotate.w (offset 68), with Euler flags 0x9. The
  temporary now starts at zero. XYZ arithmetic and matrix decisions are unchanged.
  A related helper's Vec3 temporary was receiving a full four-component rotation;
  it now uses proper Quaternion storage and Vec3 views for the unchanged Euler math.
- Fountain differed in light reference counts and ordinary SList allocation order.
  The current-light list remained on the newer timeline while its allocation and
  light payloads had rewound. Component 37 now restores the actual lighting list,
  active array/count and masks alongside the ObjAlloc/class companions. Source
  add/delete/reference-count behavior is unchanged after direct private names.

Mismatch evidence is retained as full expected/current private owner records in
`<resim-raw-path>.parts`: uint32 owner index, two uint64 payload lengths, then
expected/current bytes. This is a current-build diagnostic artifact without
format versioning or compatibility. Streams are opened before gameplay; dump
writes and broad comparisons are not part of a rollback performance benchmark.

The lighting owner is 504 bytes, claims ordinary SList root backing and known
ordinary-class light spans/types, and requires the current default-class binding
unchanged. It restores heads directly without add/delete/ref calls. Full saved
list membership/termination/reference topology is still aggregate work; physical
span/type claims are not liveness proof. Tests use actual source add/delete methods
and real allocator/class companions with controlled light construction; malformed
claims, overlap and active-boundary refusals are exercised without source callbacks.

The experiment is exclusive to offline replay and rejects production netplay/page
probe/census/benchmark combinations. It tests one same-input eight-tick interval,
not a production history or arbitrary candidate loader. Longer/repeated history,
changed-input correction, other lifecycle/asset boundaries, output reconciliation
and the final 15-tick eight-actor performance target remain required. Production
continues to use the broad page oracle. Do not treat these results as completion
of the entire snapshot/rollback goal.


Final evidence (2026-10-12), A:/FreakMelee-test-archives/:

- compact-resim-oct12-final-eight: stock and interpreted eight-actor runs at
  1,200 samples. Both restore 37 owners, resimulate all eight samples through the
  real game/render path, match every intermediate trace and every compact record,
  and reproduce the complete broad endpoint with zero byte differences. Four
  audio starts reused in each case; zero new/canceled starts. Current endpoint
  recovery and diagnostic-on/off continuation traces are exact.
- compact-resim-oct12-final-stages: Fountain, Green Greens and Corneria each run
  4,800 samples with the same eight-tick compact experiment. All 37 records and
  intermediate traces match, all six raw regions compare identically, and audio
  reuse counts are 1 / 0 / 4 with no new/canceled starts. Old/new continued traces
  match. The resimulation actually runs before broad recovery, not after it.
- compact-resim-oct12-light-owner-b: actual source light add/delete methods, real
  ordinary allocation/class companions and controlled light construction; 33
  added assertions and 86,867 total. Reference counts and list nodes rewind
  together, malformed claims/overlap/activity refuse. No source destructor fires.
- compact-resim-oct12-final-gm: connected original loop/control/player/camera
  fixture passes; optional compact/audio leaves abort if reached in that fixture,
  whose mode getters are false. The live cohorts supply actual journal evidence.
- compact-resim-oct12-euler-body-verification.json: all 23 dynamics and 7 matrix
  helper function arithmetic bodies unchanged after normalizing the documented
  storage initialization/type corrections. Final independent verification checks
  unchanged lighting bodies after private renames, source/executable pins,
  intermediate traces, all37 recaptures and exact complete raw endpoints.
  Four native build targets and focused allocator/pool/GObj-consumer CTests pass.

Earlier failures remain diagnostic evidence: eight-a found sound-instance drift;
eight-b had exact raw endpoints but stale transition-observer history; the initial
stage cohort found unused Euler W and unowned lighting heads. Euler-only stage
runs fixed Green Greens/Corneria but left Fountain allocator/refcount differences.
Those were fixed by ownership, not excluded from comparison. Final sources are
covered by the final-prefixed receipts, not the earlier debugging runs.

Across the five final cases: 5,180 normal and 185 older Load/recapture pairs,
plus 185 catch-up owner comparisons over 40 actually resimulated game ticks.
A further null-report reset Load is explicitly exercised before each replay.
The broad endpoint is 193,228,836 bytes; compact captures remain approximately
2.6-2.8 MB plus the 504-byte lighting control component in the eight-actor cases.
This is a first same-input bounded proof, not a worst-case size/performance bound
or proof of arbitrary late-input/history/lifecycle correctness.
