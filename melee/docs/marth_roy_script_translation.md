# Marth and Roy scripted translation

[Marth template](../examples/marth_wip.fms) |
[Roy template](../examples/roy_wip.fms) |
[Movement API](scripting/movement.md) |
[Weapon ribbons](scripting/trail.md) | [Guard volumes](scripting/guard.md)
| [Common action policies](scripting/common_policies.md)
| [Owned CPU profiles](scripting/cpu_profiles.md)

Both fighters can be copied into independent Workshop asset projects. These
now script all four specials, weapon ribbons, item/knockback events, death
reset, common landing reset, jab-chain policy, shield-entry presentation and
dynamic-bone wind policy and body-attack rebound permission. The selected
CPU and Peach-counter recordings match stock. Both templates now declare
independent behavior; their source-kind-free loader and versus acceptance are
recorded below. Broader interaction and rendering coverage remains bounded.

## Assets and owned parameters

The ISO costume graphs and common part tables agree on 90 joints for Marth
and 92 for Roy. Both bases have 327 action entries and 14 demo entries. The
project owns its fighter, costume, and animation archives; source identity
does not make it share those mutable assets with the original character.

Import the template's stock defaults once:

```powershell
native_content_tool.exe clone-stock GAME.iso Marth MY_PROJECT "My swordsman"
native_content_tool.exe import-parameters MY_PROJECT examples/marth_wip.fms OWNED.fms
native_content_tool.exe script-build MY_PROJECT OWNED.fms
```

The importer writes `fighter_parameters.fmp` and removes source-offset
declarations from the converted script. Ribbon color bytes become ordinary
owned integer parameters. Move timing, damage, momentum, steering, fall
physics, and ribbon appearance can then be edited without a donor DAT lookup.
The source templates are import drafts; compile the converted source.

## Source and authored decisions

| Area | Script behavior | Original source |
| --- | --- | --- |
| Neutral entry | Momentum reduction, motion-change charge reset | `ftMars/ftmarsspecialn.c` |
| Neutral start | Friction/gravity, charge-loop transition, family-specific color | `ftMars/ftmarsspecialn.c` |
| Charge loop | 30-tick charge seconds, wind timing, release/full-charge decision | `ftMars/ftmarsspecialn.c` |
| Neutral release | Damage in whole charged seconds, wind, finish, air/ground preservation flags | `ftMars/ftmarsspecialn.c` |
| Neutral accessory | Root effect choice, hitlag pause/resume, one-shot callback removal | `ftMars/ftmars.c` |
| Up entry | Command reset, momentum, throw-cue reset | `ftMars/ftmarsspecialhi.c` |
| Up interrupt | Steering threshold and angle, permitted turnaround and root rotation | `ftMars/ftmarsspecialhi.c` |
| Up physics | Animation motion, facing alignment, air-start multiplier, descent detection, gravity and drift | `ftMars/ftmarsspecialhi.c` |
| Up collision | Ledge/ground checks, delayed descending collision, special landing lag | `ftMars/ftmarsspecialhi.c` |
| Side-special entry | Ground/air momentum, one air lift per landing, command reset | `ftMars/ftmarsspecials.c` |
| Side-special chain | Early-input lockout, A/B taps, directional branches, attack-stat ordering | `ftMars/ftmarsspecials.c` |
| Side-special phases | Friction versus animation motion, air gravity, phase-preserving collision | `ftMars/ftmarsspecials.c` |
| Counter window | Subaction command activation, owned guard volume, contact hitlag, position refresh | `ftMars/ftmarsspeciallw.c` |
| Counter reaction | Contact facing/damage, wind/effect, reaction motion, fixed Marth versus scaled Roy damage | `ftMars/ftmarsspeciallw.c` |
| Lifecycle | Visual group reset, air-boost reset on death and normal landing | `ftMars/ftmars.c`, `ftEmblem/ftemblem.c`, `ftCommon/ftCo_Landing.c` |
| Jab follow-up | Marth restarts jab 1 when the chain requests jab 3; Roy uses the default policy | `ftCommon/ftCo_Attack1.c` |
| Shield entry | Marth's immediate visibility selector and sound after normal/powershield setup | `ftCommon/ftCo_Guard.c` |
| Dynamic wind | Both fighters opt dynamic chains into the shared global-wind override | `ftdynamics.c` |
| Body-contact policy | Permit an opposing body attack to rebound during the authored counter window | `ftPeach/ftpeachspecials.c` |
| Item and knockback events | Shared engine primitives with explicit group values | `ftMars/ftmars.c`, `ftEmblem/ftemblem.c` |
| Weapon ribbons | Script-owned joint, distances, fade, RGB and alpha | `ftafterimage.c`, `ftMars/types.h` |

No stock special handlers are called by any of the four authored specials. Roy
uses the same decision structure with his own imported values, color and
effect IDs. Charge time is an ordinary integer fighter-state variable. The
engine additions are generic root-motion delta reads, ordinary gravity reads,
friction-aware air acceleration, guard volumes/contact callbacks and semantic
bone-index queries. They contain no fighter-specific policy.

## Comparison runner

`tests/native/marth_roy_special_parity.py --iso GAME.iso` builds real owned
projects, packages them, and runs the same initial seed and player inputs
against stock and clone. It compares action transitions, pose/hitbox/hurtbox
digests, physics state, damage events, and RNG traces. Rendering stays enabled.
The runner checks that expected special motions were actually reached; matching
idle recordings do not count as special coverage.

### Neutral/up checkpoint - 2026-09-28

The preceding neutral/up checkpoint passed its native build and 25 focused
checks (`.local/sword-build.log`, `.local/sword-tests.log`). Its evidence comprises 46 complete
stock/script comparisons: 24 neutral paths, 18 up-special paths, and four
contact paths, across both fighters. Neutral paths cover both facings, ground
and air entry, short/partial/full charge, and landing during charge. Up paths
cover ground/air entry and default, vertical, horizontal and diagonal steering.
Contact paths record real damage to Zelda, including seven damage events in
Roy's up-special trace. Roy's full-charge self-damage is preserved.

The comparison report is `.local/sword-acceptance.json`. Its traces come from
`.local/sword-parity-1` (up paths), `.local/sword-neutral-accepted` (24 isolated
neutral paths), and `.local/sword-contact-final` (four corrected contact paths).
The generated behavior matches across these batches; only exported script
source identity changed when a comment was corrected. Preliminary recordings
that did not reach their intended motions/facings or hit the target are excluded
from acceptance. The checked-in runner includes the corrected inputs and
coverage requirements.

### Side-special/counter checkpoint - 2026-09-28

The current native build and 26 focused checks pass
(`.local/sword-complete-build-final.log`, `.local/sword-complete-tests-accepted.log`).
The guard fixture covers both compiler forms and rejects a missing reaction
callback. Parameter import checks cover literal defaults, type/range errors,
non-finite values and duplicate names.

Thirty additional complete stock/script recordings match: 20 side-special
paths and 10 counter paths. Side paths cover ground/air high, neutral and low
routes, all 18 phase motions, early-input lockout, A-button continuation and
landing during a chain. Counter paths cover ground/air misses and real Fox
laser interception, plus a Zelda melee attack that triggers the response and
receives counter damage. The pose, hitbox/hurtbox, physics, action, damage,
article and RNG traces agree, including Roy's scaled counter damage.

`.local/sword-side-counter-acceptance.json` lists the exact selected traces.
They come from `.local/sword-side-final` (ground chains),
`.local/sword-air-complete` (air chains except the low route),
`.local/sword-low-accepted` (corrected low-route inputs),
`.local/sword-counter-contact` (miss/laser paths), and `.local/sword-melee-final`
(melee reactions). The report rechecks each full comparison and confirms
identical generated behavior per fighter across those batches, ignoring only
the exported source hash. Preliminary inputs that missed a required motion or
target are excluded. The preceding 46 neutral/up recordings were not rerun
with this expanded template.

Automated comparisons now default to hidden windows while drawing remains
enabled. A Windows visibility check observed the running game window hidden
as its match continued ticking and drawing. `--show-window` is reserved for an
explicitly requested visual inspection; this is not a drawing-disabled fast path.

Rook rebuilt from unchanged source with 285 own-rig clips and replaced her prior
installation with
`13aa8ed59a6cdce53d786273e1ca53afb5a28bf5cf8dc5cd779df9afa481cd01`
for the guard host layout (`.local/rook-lifecycle-host-refresh.json`). This verifies
package registration and replacement, not a new Rook gameplay comparison.

### Common-policy checkpoint - 2026-09-28

The current build and 27 focused checks pass (`.local/sword-common-build-2.log`,
`.local/sword-common-tests.log`). The new compiled fixture exercises both script
forms, all queried motion IDs from 0..400, a state-dependent wind decision,
both shield events and invalid current-visibility selectors.

Eight complete additional stock/script recordings match: jab inputs, ordinary
shield, powershield and charging during Corneria's global wind, for each fighter.
The jab gate requires Marth to reach jab 2 and then restart jab 1. Stock Roy has
one jab, so its gate requires jab 1 rather than an unreachable follow-up. The
wind gate requires actual global-wind activation in both traces; local charge
wind effects on Final Destination do not satisfy it. Traces now compare saved
and current visibility selectors and the global wind mode as well as their
existing pose, collision, physics, action and RNG fields. Audio arguments are
checked in the compiled fixture, not by an audio-output comparison.

The selected recordings and rechecked gates are listed in
`.local/sword-common-policy-acceptance.json`: six Final Destination paths from
`.local/sword-common-parity` and two Corneria paths from
`.local/sword-common-wind-accepted`. Generated behavior agrees between these
batches. Earlier wind recordings without global wind and the initial Roy gate
are not accepted as proof of those branches. The earlier neutral/up and
side/counter matrices were not rerun for this policy-only expansion.

Rook rebuilt from unchanged source and retained 285 own-rig clips, replacing
her installation with
`f00cc138ce340b7527b0d07f1a7e8f05a00a7d314f380317605a0d62fdc4fd49`
(`.local/rook-common-policy-host-refresh.json`). This is package/host validation,
not another Rook gameplay acceptance.

### Cross-fighter/CPU checkpoint - 2026-09-28

The build and 27 focused checks pass (`.local/sword-cross-contact-build.log`,
`.local/sword-cross-final-tests.log`). The common-policy fixture checks the
new boolean `body_rebound` query in both script forms across motion IDs
0..400. Its implementation lets an intercepting fighter author whether an
opposing inert body attack may rebound; it contains no stock-counter policy.
Marth/Roy scripts permit this during their counter-window motions, matching
Peach's original stock-kind exception.

Six full stock/script recordings match: each fighter as a level-nine CPU
against a stationary human Zelda on Final Destination and Battlefield
(2,400 samples each), plus Peach side-special into each fighter's counter
(400 samples each). The CPU recordings require actual damage to Zelda; they
are not idle CPU initialization checks. The counter recordings require an
inert body contact while the counter window is active and Peach's rebound
motion, in both stock and clone. Contact is traced when its callback is
processed because the engine clears the transient flag before the ordinary
end-of-frame trace. Both contact events and subsequent state agree.

`.local/sword-cross-cpu-acceptance.json` rechecks the six comparisons and
their coverage gates, and verifies matching generated behavior across
`.local/sword-cpu-final` and `.local/sword-cross-contact-final`. Earlier
recordings that missed contact or required the wrong rebound motion are
excluded. CPU traces preceded the contact-diagnostic change; no fighter
behavior changed between these batches. Earlier special/common-policy
matrices were not rerun. All game windows stayed hidden with drawing enabled;
Windows again reported the running test window hidden.

The CPU source audit found no Marth/Roy-specific decisions in the shared
planner. Their seven attack-table families and distance threshold are copied
into fixed per-match clone storage, but are still sourced from the stock
profile at activation. This checkpoint establishes the selected CPU behavior,
not package-owned editable CPU profiles or every CPU decision.

Rook rebuilt from unchanged source with 285 own-rig clips and replaced her
installation with
`38bd95cea3716cf03856c61ff8578572a3931e35f1ef4e2beebedb2020c51a64`
(`.local/rook-body-rebound-host-refresh.json`). Package registration and
host compatibility pass; this is not a new Rook gameplay comparison.

### Package-owned CPU checkpoint - 2026-09-28

New stock projects now import `cpu_profile.fcp` once from the disc. Packages
carry this editable text file, installed clones restore it, and matches parse
it into fixed per-match storage. The donor-table lookup at activation has
been removed. A missing/empty authored profile produces empty tables rather
than selecting a stock fallback. The explicit `import-cpu` command can copy
defaults into an existing draft; editing candidates requires a package rebuild,
not a script recompile. There is no CPU-table Workshop panel yet.

Build and 28 focused checks pass (`.local/owned-cpu-build-accepted.log`,
`.local/owned-cpu-final-fixture-build.log`, `.local/owned-cpu-tests.log`).
The common-assets fixture checks all 32 stock profiles against the existing
typed importer and round-trips their text with exact binary32 values, including
two stock rows with negative weights. Malformed profiles are rejected.
Installed-package cloning retains the CPU file after the author project is
deleted; recipient edits do not change the exported original package.

Four fresh level-nine CPU recordings against stationary human Zelda match
stock on Final Destination and Battlefield (2,400 samples each). Each requires
actual damage. `.local/sword-owned-cpu-acceptance.json` rechecks the traces and
records each packaged profile's hash. All windows stayed hidden with drawing
enabled. Earlier special/contact recordings were not rerun for this data-
ownership change. No claim is made about every planner branch or CPU level.

Rook rebuilt with unchanged behavior source and 285 own-rig clips, preserving
her previously empty CPU candidate tables in `distance 0`. Her installation
was replaced with
`2940aeac3f2c28d279ec5d1208d193b951d9da348d48a1fb69684d34d5708f06`
(`.local/rook-owned-cpu-refresh.json`); the Collection lists one installed Rook.
This confirms package validation and replacement, not another gameplay run.

## Closure checkpoints

### Closure source checkpoint — 2026-10-02

Both copied bases now carry their exact immutable semantic bone tables from
retail v1.02 `PlCo.dat`. Marth has 90 physical/runtime joints and Roy has 92;
neither has omission records. Each table has 52 valid semantic bindings.
Marth's ThrowN/TransN2 map to 86/88; Roy's map to 88/90. The unnamed semantic
index 53 maps to 89/91. It must remain index 53 rather than the unrelated
`FtPart_56` enum value. The read-only owned costume graphs independently
confirm the physical counts.

The new console fixture opens a fixture-only independent candidate generated
from each complete current template, against the actual stock motion-table
initializers and actual updated copied base. All 32 special rows retain their
animation/flags and replace all four stock callback fields. Original unchanged
death/item/knockback bodies execute with the real common item/knockback inline
decisions. Their recorded presentation calls match generated callbacks for
heavy/light items, hold kinds 0..5 and both event flags. Death/landing reset
ordinary owned air-boost state; disagreeing legacy union values remain untouched.
The fixture also covers the known jab/rebound/wind queries over motions 0..400,
shield presentation and ordinary integer charge reset at neutral entry.

The coherent baseline17 batch passed **3,094 source comparisons**:
`.local/marth-roy-closure-baseline17-complete/acceptance.json`. Input hashes,
imported FMP/source hashes and generated C hashes are recorded there.
`source_inventory.json` also records the costume graph hashes and actual
seven-family CPU commands/counts. The first fixture compile rejected a wrong
`s32` sound-stub signature; the second run lacked the synthetic fighter's
visibility-group count. Both fixture defects were corrected before this
successful fresh-output batch. Those initial outputs are not accepted evidence.

Reproduce with a matching frozen SDK/compiler and read-only stock projects:

```powershell
python -B tests/native/marth_roy_closure_source_compare.py --sdk .local/baseline17/sdk --tool .local/baseline17/bin/native_content_tool.exe --clang .local/baseline17/compiler/clang.exe --output .local/marth-roy-closure-recheck
```

The runner accepts `--marth-project`, `--roy-project` and `--common-dat`
overrides. It imports parameters into fresh output and adds the independent
declaration only to that fixture candidate. **The shipped templates remain
unpromoted and both copied bases retain source kind.** The fixture calls the
generated DLL entry directly; it does not replace actual loader, lifetime,
renderer, animation scheduling or gameplay acceptance. Presentation setters
and exact generation/class liveness are explicit fixture boundaries. Retail
undefined initial charge is not a promised deterministic behavior.

The current source census found no additional Marth/Roy-specific CPU planner
branch, taunt/results policy, article lifecycle or private-union model-effect
updater needing a new API. The existing owned CPU importer handles the table
data. Ordinary taunt and results constructors remain shared; both bases retain
their own four demo archive names. The fighter tables for landing, user-data
removal, absorption and auxiliary callbacks are NULL for both kinds.
The reachable versus exceptions are already authored at
`ftCo_Attack1.c:275`, `ftCo_Guard.c:350/984`, `ftCo_Landing.c:66`,
`ftdynamics.c:591/662`, `ftpeachspecials.c:77` and `ftafterimage.c:31`.
Their stock specials and lifecycle originals remain in `ftmars.c`,
`ftemblem.c` and the four `ftmarsspecial*.c` files.

At this source checkpoint, the next acceptance was the source-kind-free loader/rig path plus coordinated
hidden, muted, drawn gameplay on the current SDK. Reuse the existing special,
common, CPU and contact inputs while adding interruption/death/repeated-air-boost
and results paths. The shared parity runner already enforces mute and defaults
to hidden windows. This source batch does not replay the earlier special matrix
or accept every costume, rendered ribbon or CPU level. No new generic API is
proposed by this closure audit.

### Independent versus checkpoint - 2026-10-02

Both shipped templates now declare `behavior independent`. The final original-C
source batch passes **3,094 comparisons** against baseline18, including all 32
extended motion rows and lifecycle/common policies:
`.local/marth-roy-independent-source18/acceptance.json`. The source runner now
requires a shipped independent declaration by default; `--candidate` explicitly
permits an unpromoted fixture source. It no longer silently rewrites a shipped
fighter declaration.

The coordinated drawn batch accepts **52 stock/independent pairs**, comparing
**30,000 controller samples and 60,000 fighter states**. Each fighter exercises
full/partial/short neutral charges, ground/air up special and contact, all three
ground/air side-special routes, early-input rejection and continuation, laser
and melee counters, jab/shield/powershield, actual Corneria wind, Peach's inert
body contact, and active level-nine CPUs on FD and Battlefield. Two additional
normal-input trajectories per fighter prove fresh/consumed air boosts across
common Landing and real death/rebirth; the post-rebirth boost occurs before a
landing can reset it. No positions, stock losses or states were injected.

The first lifecycle inputs reached ground side-special rather than the intended
offstage boost, and Roy landed during his second side-special instead of common
Landing. Those traces matched stock but failed their coverage gates. The corrected
inputs hold a full jump, double-jump near its apex, and consume the offstage boost
only after walking off. All four corrected trajectories pass. The initial coverage
misses remain recorded and are excluded from the accepted matrix.

Eight final shipped-package comparisons recheck melee counters, active CPU play
and both lifecycle paths: **8,800 samples and 17,600 states**, all matching.
Final generated C is identical to the accepted candidate after excluding only
the exported source-hash value; owned parameter and CPU files are byte-identical.
No additional gameplay change was made at promotion. The evidence receipt is
`.local/marth-roy-independent-acceptance18.json`; its source identity entries and
per-case receipts distinguish the full candidate matrix from final package checks.

Reproduce the current batch with a matching immutable SDK/compiler:

```powershell
python -B tests/native/marth_roy_closure_parity.py --iso '<ISO>' --tool .local/baseline18/bin/native_content_tool.exe --exe .local/baseline18/bin/melee_native.exe --include-lifecycle --out-dir .local/marth-roy-independent-recheck
python -B tests/native/marth_roy_closure_source_compare.py --sdk .local/baseline18/sdk --tool .local/baseline18/bin/native_content_tool.exe --clang .local/baseline18/compiler/clang.exe --output .local/marth-roy-independent-source-recheck
```

The runner uses `script-c` followed by direct compilation with the frozen headers:
the tool's ordinary `script-build` embeds live include paths and would not isolate
a snapshot while another shared API batch is in progress. Games are hidden,
muted and accelerated with drawing enabled. See the
[runner report](../tests/native/marth_roy_closure_parity.md) for coverage gates.

This acceptance does not claim exact rendered ribbons, every costume/CPU level,
all edge interruptions or every shared interaction. Results/demo archive names
and generic construction paths were audited, but no fresh real results scene
is claimed here. Wind restoration, held-item jab restarts and different scales
remain broader recording targets. Kirby cloning and copy integration remain
deferred by user decision; stock Kirby stays playable. No new fighter-specific
script API was needed for this closure.
