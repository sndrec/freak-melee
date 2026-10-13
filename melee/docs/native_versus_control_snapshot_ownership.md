# VS, ambient item and damage-HUD control ownership

The owner also saves the next native Item serial (`it_804D6D10`). Constructors
advance this identity even if their items are removed before the next capture,
so occupied pool records alone cannot reconstruct it. Corrected-input rollback
exposed the missing ownership; the native increment behavior remains unchanged.

Longer repeated intervals additionally exposed item population/cap bookkeeping
(`Item_804A0C64`) and fighter reference counts (`ftData_CharacterReferenceCounts`).
Both full numeric control sets are now owned, with direct buffer-independence
checks and negative fighter-reference rejection. Their original counting behavior
is unchanged. See [native_compact_history_experiment.md](native_compact_history_experiment.md)
for the failed and accepted evidence and remaining aggregate boundaries.

The actual VS controller, three ambient item-selection globals and damage-HUD
record/control bytes now have explicit storage units. Source consumers directly
access the same data or their renamed private symbols; no live mirror or adapter
was introduced. Pre-existing gmvs/itspawn/ifstatus function bodies are preserved
apart from direct private-name replacements.

The 35th census component is 14,088 bytes. It captures the full VsSceneController
(timer, pause/end/respawn/statistics/rules fields), RandomItemSpawner countdown,
all three weighted-pick descriptors and active payload prefixes, all six HUD
player records and damage display/rounding bytes. Prepared rule callbacks/opaque
parameter pointers, HUD descriptor identities and pick count/backing identities
remain scene leases. Mutating these bindings requires retiring/preparing history,
not following unrecognized candidate pointers. Weighted table values and payloads
are restored separately from these bindings, with no source rebuilding/RNG calls.

HUD parent/marker roots carry GObj generation claims. Active damage-digit joints
carry ordinary-class span/type claims, checked against the saved class candidate
before diagnostic Load. When a HUD parent is absent, old joint pointer bits remain
dormant and carry no class claim or dereference. Load requires restored GObj/class
companions and unchanged current definition bindings before copying any control
state or table bytes. It runs no callbacks, draw, allocation or game logic.

Actual storage is compiled in .mstate$m; the transition-only lease is host .data.
Begin/EndScene are integrated into the GM lifetime. This is the complete named
control records, not all HUD modules, opaque descriptor payloads or aggregate
root/model/asset closure. Buffer independence excludes this owner's controls,
prepared table backing, active borrowed joint/type spans and GObj/ordinary
ObjAlloc backing. Aggregate buffers must also remain independent of other owners.

Evidence in A:/FreakMelee-test-archives/:

- versus-controls-oct12-owner-a: actual control storage/methods and real generic
  pools/classes/allocators; fifty added assertions, total fixture checks 86,801.
  Controlled rule/HUD/root construction, table contents and state mutations
  exercise exact restoration, seven malformed candidates, changed generations,
  output overlap, dormant pointer and retirement behavior. Source callbacks abort
  if invoked. This is not source gameplay or complete HUD graph resurrection.
- versus-controls-oct12-gm-b: connected production loop/control/player/camera
  fixture passes with this owner's lifetime hooks. Class graph and ObjAlloc
  endpoints abort on unexpected use: this fixture does not exercise its Save/Load.
  gm-a exposed missing test-only class candidate symbols; those endpoints were
  added as aborting stubs, not replaced with successful fake results.
- versus-controls-oct12-eight-a: stock/interpreted eight-actor clients pass all 35
  component captures/validations, 350 normal Load/recapture pairs and 35 older
  pairs each, with endpoint recovery and previous/current/diagnostic trace equality.
- versus-controls-oct12-stages: Fountain, Green Greens and Corneria each pass
  4,800 rendered samples, 1,400 normal and 35 older pairs, with exact old/new traces.
- versus-controls-oct12-independent-verification.json: all final source/executable
  pins, pre-existing source bodies, data/host sections and prior/current traces
  verified. Four build targets and focused pool/ObjAlloc/GObj-consumer CTests pass.

Raw target-oracle/PDB correlation in the eight-actor cases no longer identifies
VS controller, random spawner or HudIndex differences. Nearest-symbol attribution
remains a diagnostic candidate, not automatic semantic classification of every
remaining byte. Pool inactive tails/payload, collision/animation scratch, particle
display state and texture identity counters still need classification/ownership.
Complete candidate graphs/publication, rendered/changed-input resimulation and
rollback performance remain unfinished. Production retains the broad page oracle.

The later match-start checkpoint adds READY/status records, typed lazy debug-text
camera identity, cached HUD coordinates, HUD/nametag visibility and crowd counters.
See [Compact snapshots across match start](native_compact_stage_start_transition.md)
for the ownership contract, accepted cases and remaining frame-zero mismatch.
