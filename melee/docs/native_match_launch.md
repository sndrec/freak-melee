# Direct Versus launch

For background testing, pass `--hidden --mute`. The mute option sets host
output gain to zero while retaining normal mixing, synth callbacks and audio
queue timing. All automated Melee launches must be muted.

With `--hidden`, Aurora keeps the normal drawing and
simulation paths active but never shows the game window or pauses it for
missing focus. `tools/run_fighter_parity.py` uses this by default; pass
`--show-window` only for an explicitly requested visual inspection.

The Windows client can enter a local Versus match without navigating the
menus. Build `melee_native`, then run from the repository directory:

```powershell
.\build\native\melee_native.exe --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --match --stage battlefield --p1 human:falco --p2 cpu:luigi:1 --mode time --time 2 --items off
```

Omit `--match` and its match options to start at home character select. `--help`
prints the current option list.

For deterministic offline item comparisons, repeat
`--fixture-item <kind,sample,x,y>` to spawn up to eight ordinary common items.
The sample is the zero-based controller-sample index in `--replay-inputs`;
coordinates use game world units. For example, `--fixture-item 2,120,-55,10`
spawns a barrel near P1's Final Destination starting position at sample 120.
It follows ordinary item physics, pickup and collision. A failed spawn aborts
the comparison rather than silently testing a match without the item.

For CPU recovery comparisons, repeat `--fixture-airborne <port,sample,x,y>`
to place a main fighter at a world position and begin an ordinary fall with
all air jumps used. Ports are 1–4; up to eight placements can be scheduled.
For example, `--fixture-airborne 1,120,-175,-5` puts P1 left of Final Destination
at sample 120. Shared transitions reset movement and initialize the collision
box. The CPU chooses its destination and inputs normally; the fixture does not
inject a recovery queue or alter RNG. Missing fighters abort the comparison.
With `--trace-actions`, `[cpu-recovery]` records the sample, fighter position
and planner destination at each recovery decision.

Fixtures require `--match --replay-inputs` and are rejected with netplay or
rollback probes. They never run during normal gameplay or verified replay
playback. Use identical fixture arguments for stock and scripted comparisons;
`tools/run_fighter_parity.py` forwards them to both runs. Drawing stays enabled
with `--hidden --fast`.

| Option | Values |
| --- | --- |
| `--stage` | Any currently imported stage listed below, using lowercase words with optional hyphens; `fd` aliases `final-destination` |
| `--p1` through `--p4` | `human:<character>`, `cpu:<character>[:level]`, or `off`; CPU level is 1 through 9 |
| `--p1-profile` through `--p4-profile` | Profile ID from `%LOCALAPPDATA%\Freak Melee\profiles`; works for ordinary menu startup and direct matches |
| `--p1-color` through `--p4-color` | Free-for-all costume index beginning at 0; each fighter has its own valid range |
| `--teams` | Enable team battle |
| `--p1-team` through `--p4-team` | `red`, `blue`, or `green`; active fighters must occupy at least two teams |
| `--mode` | `time`, `stock`, or `coin` |
| `--time` | 0 through 99 minutes; 0 is allowed only for stock |
| `--stocks` | 1 through 99 |
| `--items` | `off`, `very-low`, `low`, `medium`, `high`, or `very-high` |

Character names are written without spaces or with hyphens: for example,
`captain-falcon`, `ice-climbers`, `mr-game-and-watch`, `young-link`, and
`dr-mario`. At least two slots must be active, and at least one must be human
(the practice room allows a single human).
The native stage importer currently runs Battlefield, Final Destination,
Dream Land, Green Greens, Jungle Japes, Yoshi's Story, Yoshi's Island, Flat
Zone, Yoshi's Island (64), Kongo Jungle (64), Fountain of Dreams, Rainbow
Cruise, Great Bay, Hyrule Temple, Onett, Pokemon Stadium, Brinstar Depths,
Fourside, Mushroom Kingdom, Mushroom Kingdom II, Poke Floats, Big Blue, and
Icicle Mountain. The
[Versus stage sweep](native_stage_sweep.md) records the current startup results
for all 29 selectable stages.

`--stage practice` launches the native grey-grid room with a separate Sandbag,
preserving all four normal player slots. Practice overrides the match rules:
untimed free-for-all, items off, unlimited rebirths, and no results screen.
Sandbag returns to the center at 0% if knocked beyond a blast zone. Use
`--p2 off --p3 off --p4 off` for solo practice. The usual pause/exit chord returns
to home CSS. Normal home START enters practice; its sidebar Start match
button leads to stage select with the locked characters. Practice is local only and does not replace any original stage.
The default direct match is Battlefield, Falco human in P1, Luigi CPU level 1
in P2, a two-minute time limit, four stocks, and items off. Stock matches have
no timer unless `--time` is supplied. An enabled item frequency currently
enables the full item set; selecting individual items is future work.
Team battles use Melee's character-specific red, blue, and green costume
mappings. Explicit costume indices are accepted only for free-for-all matches.
The match controller exposes item frequency as the signed range `-1..4`, with
`-1` meaning off. Its native accessor preserves that integer range so `off`
does not collapse to boolean true and enabled frequencies retain their exact
spawn-rate index.

Example four-player stock match:

```powershell
.\build\native\melee_native.exe --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --match --stage dream-land --p1 human:fox --p2 cpu:marth:5 --p3 cpu:falco:5 --p4 human:peach --mode stock --stocks 4 --items low
```

These options feed the existing Versus match setup. They are a development
shortcut; gameplay, character, and stage behavior still require runtime
validation. The named Battlefield/Falco/Luigi/time combination reached a
live match. One complete one-stock Battlefield smoke run used this lineup:

```powershell
lldb -- build/native/melee_native.exe --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --match --stage battlefield --p1 human:ganondorf --p2 cpu:zelda:1 --p3 cpu:ice-climbers:1 --p4 cpu:mr-game-and-watch:1 --mode stock --stocks 1 --items off
```

Under the debugger it reached Game, displayed results, and returned to
character select without a game panic. This verifies that specific full
match-end route; it does not establish general fighter, stage, or item fidelity.
Use one stock for subsequent development test rounds, and keep the debugger
attached while testing this character set.

An item-enabled Green Greens recording exposed a 64-bit import failure when a
Poké Ball selected Unown: its 26 serialized model pointers had been copied as
ordinary words. After typed imports were added for all relocation-bearing
`ItCo.usd` special-attribute blocks, the exact 666-sample crash replay reached
its normal replay end. A new `very-high` item-frequency run with the same
four-character lineup then remained active for two minutes and recorded a
longer deterministic segment before the unattended run was stopped. This is
targeted runtime evidence for the importer correction, not complete item
fidelity or a completed match-end run with items.

The Dream Land one-stock recording with Ganondorf, Zelda, Ice Climbers, and
Game & Watch completed 6080 samples and reached results. The match-end record
held the correct four character IDs, but the shared result/stock portrait frame
lookup left its base index uninitialized for early-roster characters and the
stock-icon helper failed to return the lookup value. Both are now corrected.
A same-build fast replay matched all 6080 samples, the portrait mapping test
covers the affected characters and a costume offset, and an in-game results
screenshot confirmed the four displayed names and portraits. This confirms
the observed four-player results issue, not every results layout or mode.

A second one-stock Battlefield run broadened the result roster to P1 Ganondorf
with CPU Mario, Fox, and Peach. Peach won after three KOs. The captured results
screen (`screenshots/melee-1789899917-0000.bmp`) showed the correct four names,
portraits, placements, KO/fall counts, and Peach winner model. The exact 22,982
recorded controller samples then replayed to their endpoint without RNG or
tracked fighter-state divergence. This adds those three result portraits and a
different winner presentation; it still does not cover every character,
costume, team layout, tie, or result mode.

A one-stock Final Destination team battle assigned Ganondorf and Ice Climbers
to red against Zelda and Game & Watch on blue, with very-high items. It reached
the team results layout with the correct blue-team winner, colored portraits,
placements, and statistics in `screenshots/melee-1790002399-0000.bmp`. All
30,048 recorded controller samples then replayed to their endpoint without RNG
or tracked fighter-state divergence. A separate Battlefield free-for-all
launched two Ganondorfs with explicit costume indices 1 and 2; the distinct
costumes appeared in the winner model and portraits in
`screenshots/melee-1790002683-0000.bmp`. These runs establish direct-launch
team assignment and costume selection, not every team composition or costume.

One-minute Dream Land matches also reached the original time and coin result
layouts. The time result displayed KO, fall, self-destruct, and total rows in
`screenshots/melee-1790002858-0000.bmp`. The coin result displayed the same
combat rows plus per-player coin totals and the correct winner in
`screenshots/melee-1790002961-0000.bmp`. These complete live result-layout
checks for the three supported rule modes; ties and sudden death remain separate
coverage work.

A level-nine Pokemon Stadium run used Ganondorf with CPU Samus, Ness, and
Mewtwo plus very-high item frequency. A temporary first-use trace confirmed
successful CPU selections from the ground, air, ranged/projectile, and
held-weapon attack tables. At sample 1,592, Samus's dash grab exposed raw
GameCube pointers in the grapple beam's model/animation attributes. After the
complete attribute block received a typed import, that exact crash recording
replayed to its endpoint with matching RNG and fighter state. A fresh run then
continued for 64,322 samples and crossed a scene reload before it was stopped.
Its playback later hit the already-open shared-RNG determinism problem at
sample 4,123, so the long segment is runtime stability evidence rather than a
deterministic replay pass.

A 99-stock Battlefield diagnostic selected human Kirby and Mario so the
named-pipe controller could exercise copy acquisition without keyboard input.
Kirby inhaled and swallowed Mario, rendered the imported Mario copy hat, and
fired the copied fireball while the client remained live. The hat and fireball
are captured in `screenshots/melee-1789902914-0007.bmp` and
`screenshots/melee-1789902929-0008.bmp`. This verifies the Mario copy runtime
path after the complete `PlKbCp*.dat` corpus received typed native imports; it
does not establish every copy ability or costume.

A Final Destination diagnostic then selected human Kirby and human Game &
Watch. Direct launch now writes its chosen stage and character/color entries
into the preload cache before scheduling the loads; the prior path reset that
cache and resolved every selected external character through Mario. The player
module now reads the typed `ftMapping_list` directly instead of casting the
neighboring `PdPm.dat` string globals into the GameCube linker layout.

The first swallow exposed two further host-layout assumptions. Kirby's merged
copy-model code advanced `HSD_DObj**` arrays in four-byte increments, and the
Game & Watch copy archive relied on a following relocated GameCube address
appearing negative when treated as a second visibility count. Native code now
indexes the pointer arrays by element and materializes that effective empty
visibility entry explicitly. The exact 6,479-sample crash recording replays to
its endpoint without RNG or tracked fighter-state divergence. In a fresh live
run, Kirby swallowed Game & Watch and rendered the black copy form captured in
`screenshots/melee-1789904680-0005.bmp`; copied neutral-B input then ran without
a crash, although this run did not visually confirm every Chef article state.

Two further item-heavy roster sweeps extend runtime coverage beyond the default
four-character diagnostic. Mario, Fox, Ness and Peach ran on Fountain of Dreams
and all 20,960 recorded samples, including the reset transition, replay without
divergence or panic. Samus, Mewtwo, Luigi and Marth then ran on Pokémon Stadium
for 30,162 recorded samples after the stage's undersized display-wrapper
allocation was corrected; a 10,000-sample slice replays to its endpoint. These
runs establish sustained initialization and ordinary CPU-driven gameplay for
the selected combinations. They do not prove every move, item interaction, or
character-specific mechanic.

The remaining two unexercised playable banks were then included in a Dream
Land run with Ganondorf, Young Link, Dr. Mario and Game & Watch. All 13,283
very-high-item controller samples replay to their endpoint without divergence
or panic. Every playable character has now appeared in at least one sustained
native roster run, but this is initialization and ordinary CPU-play coverage,
not complete per-character mechanics acceptance.

A Ganondorf/Zelda/Ice Climbers/Game & Watch control shows four normal percent
displays during sustained gameplay in
`screenshots/melee-1789908103-0000.bmp`. The later Stadium transformation
capture initially rendered `HP` for P2 through P4 despite all four gameplay
stamina flags and texture-animation clocks reading zero at that exact
deterministic frame. The percent HUD was reaching texture animations through a
pair of 32-bit layout casts: an `HSD_MatAnimJoint` was viewed as an
`HSD_AnimJoint`, then its `HSD_MatAnim` as an `HSD_AObjDesc`. Native pointer
widening made the second cast select the material AObj instead of its texture
animation. The HUD now names and traverses its typed animation arrays directly.
The same frame renders `48% / 57% / 7% / 71%` in
`screenshots/melee-1789908704-0000.bmp`, and the full 10,000-sample replay still
reaches its endpoint without random or fighter-state divergence.

# Native match recording and replay

Add `--record <file>` to a direct match to save the boot seed and every
four-port controller sample after Aurora combines physical and virtual input
and the native profile maps bindings and axes. Playback uses those mapped
samples without applying the current profile again. In netplay, recording
happens before the input filter resolves fighter ports, so playback can run
that filter again.
The file is flushed after each sample so it remains useful if the game crashes.
Use `--replay <file>` with the same `--match` options to feed the recorded input
back to the game. No controller input is needed during playback; playback exits
when the recorded samples end. A different set of match settings is rejected.

For the current four-character diagnostic lineup:

```powershell
$matchArgs = @('--iso', 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso', '--match', '--stage', 'final-destination', '--p1', 'human:ganondorf', '--p2', 'cpu:zelda:1', '--p3', 'cpu:ice-climbers:1', '--p4', 'cpu:mr-game-and-watch:1', '--mode', 'stock', '--stocks', '1', '--items', 'off')
& .\build\native\melee_native.exe @matchArgs --record .\build\native\last-match.fmr
& .\build\native\melee_native.exe @matchArgs --replay .\build\native\last-match.fmr
& .\build\native\melee_native.exe @matchArgs --replay .\build\native\last-match.fmr --fast
& .\build\native\melee_native.exe @matchArgs --replay .\build\native\last-match.fmr --rollback-probe
& .\build\native\melee_native.exe @matchArgs --replay .\build\native\last-match.fmr --rollback-correction-probe
& .\build\native\melee_native.exe @matchArgs --fast
```

The recorder reports controller **sample** indices through
`nativeReplay_SampleCount()` for debugger breakpoints. A sample is normally one
game frame, but boot sampling and pauses mean the indices should not be
treated as match frame numbers. Playback checks the random seed before every
sample and stops at the first mismatch. It also compares a portable semantic
gameplay digest before each sample. The digest covers match/player state,
fighter input, physics, collision, combat and pose state, common item and
ground state, and stage/item joint animation. The first nonfinite fighter
position or bone pose is logged with its sample index, player, action, and bone.
This is a local, same-build diagnostic format; character-specific variables,
item/stage-specific unions, and allocator pool topology remain outside the
digest, so it does not yet establish complete determinism of every game
subsystem. Fighter, item, and ground links use stable category-and-list-order
identities rather than native pointer values.
For cross-package comparisons, `--replay-inputs <file>` feeds the same seed
and pads while allowing different fighter content. `--trace-fighter-state`
prints each active fighter's motion frame, ground/air status, command 0,
stick input, launch angle, facing, velocity, position and animation movement
as raw float words for exact per-sample comparison. It requires `--match`
with recording or replay, produces a large stderr log, and is diagnostic only.
`--fast` works with any direct match, including one without recording or
replay. It advances input sampling using a virtual 60 Hz clock and disables
VSync. It still runs the normal frame update and rendering; its speed is
therefore limited by simulation, rendering, and shader compilation.
`--rollback-probe` also uses accelerated timing. At or after frame 240 it waits
for an eight-frame interval containing a sound start, restores that interval,
rewinds replay input to the saved frame boundary, runs the ordinary game draw
callbacks and GX submission without presenting the intermediate frames, and
requires the complete tracked-state hash to match at the endpoint. Stable sound
intents reuse their live AX instances during resimulation. The probe exits after
that comparison; see `docs/native_rollback.md` for its scope and current
evidence.
Both rollback probes accept `--replay-inputs` for cross-package local checks.
Recorded state/RNG digests stay disabled in this mode; the probe verifies its
own restored timeline. Netplay remains incompatible with `--replay-inputs`.
`--rollback-correction-probe` treats P1 as remote, simulates one predicted
sample, submits a different confirmation seven frames late, and restores from
the reported correction point. It runs the corrected interval twice with full
draw callbacks and requires every tracked byte to match at both corrected
endpoints. Both rollback probes suppress physical rumble during hidden frames
and apply the restored final motor status once when the endpoint is committed.
Direct UDP netplay bring-up uses `--netplay-session`, `--netplay-listen`,
`--netplay-local-peer`, `--netplay-local-mask`, and one or more
`--netplay-peer` assignments. `--netplay-delay` can hold outbound input frames
to exercise prediction and correction in direct or sidebar room matches; a
home boot accepts this diagnostic option without `--match`. See
`docs/native_netplay.md` for the
current command line, evidence, and missing connection features.
Rendering currently changes tracked fighter state during a star KO:
`ftDrawCommon_80080E18` projects the offscreen fighter through the camera and
writes `cur_pos`. In two controlled Dream Land replay trials, omitting only
scene drawing while retaining the backend frame calls and counters first
diverged at controller sample 4037; two full-draw controls completed all 6080
samples. A further trial skipped only fighter draw callbacks while still
drawing the stage and camera. It reproduced the same first divergence at
sample 4037. Zelda entered `DeadUpStar` there, whose draw callback writes her
camera-projected `cur_pos`; the star-KO animation subsequently reads that
position when setting velocity and spawning an effect.

For diagnosis, a second digest excluded star-KO fighters. The remaining
fighters' tracked state matched across all 6080 samples in the full-draw,
scene-skip, and fighter-skip runs. The shared RNG seed first differed at sample
4126 in both skip runs, and the full-draw seed was nine RNG advances ahead.
Seed changes occurred in the simulation step, not in the immediately preceding
draw call. This does **not** prove the rest of simulation is draw-independent:
the normal replay rejects the seed mismatch, and this digest does not cover
every game subsystem. The diagnostic skip/continue probes were removed after
the trials. The fast path still draws. Before simulation-only rollback, move
state updates needed by gameplay out of fighter rendering, separate cosmetic
randomness from gameplay randomness where appropriate, and test snapshot and
replay equality with drawing disabled.
