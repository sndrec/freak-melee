# Live same-boundary snapshot self-restore

Current implementation (2026-10-12): this diagnostic uses the 36-owner
[shared compact capture/load core](native_match_snapshot_shared_core.md).
The historical diagnostic-only loader named below was removed after moving its
direct implementation and adjusting callers; no forwarding layer remains.
The earlier checkpoint/evidence below is retained as history.


The offline component census has an optional self-restore diagnostic. It retains
all 34 captures in distinct eight-byte-aligned host slices at the same complete
post-draw boundary, requires every capture to validate, runs the actual component
Load methods in dependency order, then requires all recaptures to be byte-identical.
No simulation, script callback or rendering occurs between the capture and Load.
Normal HSD drawing continues between diagnostic boundaries.

The original observation mode remains the default. Enable the diagnostic through
`--self-restore` in `tests/native/native_snapshot_census/run.py`, or in the selectable
stage runner with `--snapshot-census --self-restore`. Those runners pass
`FREAK_MELEE_SNAPSHOT_SELF_RESTORE=1` only to the offline census client. Existing
netplay/census rejection remains in place; ordinary gameplay allocates no census
scratch and does not run these Loads.

The census reserves its capture and comparison buffers once before gameplay.
Both are fixed 64-MB diagnostic capacities, not serialized state or production
history; no host allocation occurs at a sampled tick. Current captured components
occupy only the used prefix. Load timing excludes log writes, but includes current
component validation and readiness checks. It is not a rollback benchmark.

`native_snapshot_self_restore.c` directly invokes each actual Load. HSD heap
selection/metadata and ordinary/reserved allocators precede pool objects; payload
and root owners precede script actor rebindings. No compatibility adapter or new
production save format is introduced. Host census storage is compiled in `.data`.
All inputs come from the census's own current-boundary captures. There is no
new command/file/network path for loading supplied candidates. The diagnostic is
not a general candidate loader, and its prevalidated-current-state assumption
must not be used for history or external data: cross-component graph validation
and safe ordered commit for those cases remain separate implementation work.

Final evidence under A:/FreakMelee-test-archives/:

- self-restore-oct11-eight-b: stock and independent interpreted eight-actor cases
  each pass all 34 captures/validations, 340 actual Loads and 340 byte-identical
  recaptures at ten boundaries. Observation/self-restore-off traces and prior
  accepted traces agree. Fighter, item and effect script owners all participate;
  item/effect script records are empty in this eight-Ice-Climbers cohort, so their
  active-actor resurrection still needs separate history coverage.
- self-restore-oct11-stages-a: Fountain, Green Greens and Corneria each pass
  4,800 rendered old/new samples, all 34 owners at forty boundaries, and 1,360
  actual Loads/identical recaptures. Stages use Ganondorf, Zelda, Ice Climbers and
  Game & Watch CPUs, with medium items. This is not every stage interaction.
- self-restore-oct11-independent-verification.json: final source/executable/log
  input pins, load-order coverage, prior/current eight-actor traces and host-only
  census section verified. Total passing Load/recapture pairs: 4,760. Four native
  build targets and focused ObjAlloc/GObj-consumer CTests pass.

Peak used capture prefixes are 2,608,144 bytes (stock) and 2,779,992 bytes
(interpreted) in the eight-actor cases. Across ten samples each, complete diagnostic
Load means are 1,141.30 / 1,158.32 us; maxima are 4,437.70 / 3,982.80 us. They are
same-boundary samples, not changed-state rewind timings or worst-case limits.
The active production rollback path still uses the broad page oracle. Missing
world/asset/HSD/HUD/resident ownership, candidate-wide graph closure, history
rewind across allocation/reuse, changed-input resimulation, graphics regeneration
and complete save/load/15-tick performance acceptance remain unfinished.


2026-10-11 actor preflight update: saved fighter/Ground/item records and saved
GObjs now have a bidirectional ownership check before diagnostic Loads. It reads
only candidate payloads and uses saved occupancy, so reused live slots do not
invalidate an older coherent graph. Other stage userdata-tag-3 roles remain
separate owners. See [actor graph preflight](native_snapshot_actor_graph.md) for
malformed/reuse tests and 140 passing live preflights with 4,760 Load/recapture
pairs. This adds one required aggregate relationship check; class/model/VM/other
candidate closure and genuine history rewind remain unfinished.


2026-10-12 class candidate update: caller-reserved indexes now expose saved class
payloads and candidate-prefix membership without per-tick allocation. Saved type
headers for main camera, stage/hazards/grid, effect queue and particle attachments
are checked before diagnostic Loads. See [class candidate views](native_class_snapshot_view.md)
for lease/index rules, later-block exclusion and 140 passing live preflights.
This does not prove class-piece occupancy or complete allocation/graph closure;
full history rewind/resimulation and rollback performance remain open.


2026-10-12 bounded older-capture update: the census can restore its own capture
after eight normal ticks, compare all 34 records, report raw target-oracle gaps,
and recover the complete current endpoint before continuing. Stock/interpreted
eight-actor and three stage cases pass 170 older Load/recapture pairs, including
creation/destruction/reuse, with unchanged continuing gameplay traces. See
[rewind diagnostic](native_snapshot_rewind_diagnostic.md) for the recovery-buffer
contract, raw coverage findings and limitations. This supersedes the earlier
current-boundary-only diagnostic precondition for this explicitly safeguarded
experiment; it does not add arbitrary candidate loading or run resimulation.
