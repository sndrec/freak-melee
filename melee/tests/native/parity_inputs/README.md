# Stock fighter parity inputs

These FMR files contain controller samples and an initial RNG seed, not game
assets. Run them with `--replay-inputs` so the same inputs can be applied to a
stock fighter and its packaged clone. The state digests recorded in the files
are intentionally not used by this comparison; current stock and clone traces
are compared against one another instead.

## Mario's scripted articles

The [Mario translation ledger](../../../docs/mario_script_translation.md)
records the full 13-case batch and its remaining coverage. The five Mario
interaction inputs below start from the corresponding Dr. Mario recordings,
remove the preliminary taunt and rejected early B press, and adjust contact
spacing for Mario's Fireball. The shield input keeps R held until contact.
All use two stocks, items off, Final Destination, and 520 samples.

| Input | P2 | Required evidence |
| --- | --- | --- |
| `mario-fireball-contact-inputs.fmr` | Zelda | Motion 343, item 48, 6% to P2 at sample 288 |
| `mario-fireball-shield-inputs.fmr` | Zelda | Motion 343, item 48, P2 shield-hit motion 181 at sample 289; no damage |
| `mario-fireball-reflected-inputs.fmr` | Mario | Motion 343, item 48 transfers P1 to P2 at sample 291 |
| `mario-fireball-clank-inputs.fmr` | Mario | Motion 343, item 48, two clank events at sample 307 |
| `mario-cape-reflects-pill-inputs.fmr` | Dr. Mario | Motion 345, item 49 transfers P2 to P1 at sample 292 |

The runner accepts `--expect-port-motion PORT MOTION` to require a transition
on either compared actor, in addition to `--expect-motion` for P1. Use
`--expect-port-motion 2 181` for the shield case so a Fireball that simply
passes under a shield cannot count as shield-contact coverage.

The [Captain Falcon / Ganondorf ledger](../../../docs/captain_ganon_script_translation.md)
records the source audit and fresh project-owned-parameter comparison batch.

The Falcon Kick cases use Captain Falcon as P1 and Zelda as P2 on Final
Destination, with one stock, one-minute match and items off. They also reach
the corresponding Wizard's Foot paths with stock Ganondorf and his packaged
clone. The intended P1 motions are:

| Input file | Required motions | Other expected event |
| --- | --- | --- |
| `fighter-kick-contact-inputs.fmr` | 357, 358 | Damage to P2 |
| `fighter-kick-miss-inputs.fmr` | 357, 358 | No contact |
| `fighter-kick-left-miss-inputs.fmr` | 357, 362 | Leaves platform |
| `fighter-air-kick-inputs.fmr` | 359, 360 | Lands without contact |
| `fighter-air-kick-hit-inputs.fmr` | 359, 360 | Damage to P2 on landing |

`fighter-kick-wall-bounce-inputs.fmr` uses Onett rather than Final
Destination. Repeated grounded kicks bring P1 against the first building's
wall. Captain Falcon enters bounce motion 363 at sample 316; Ganondorf enters
it at sample 317. Both then reach the common airborne landing path. Use
`--stage onett --expect-motion 363` with the parity runner for either clone.

The Falcon Dive recordings use the same match setup. The miss enters grounded
Falcon Dive (353), rises, and returns through Fall (35) without a grab. In the
catch recording, Zelda runs toward Falcon before the up-special; Falcon enters
Catch (355) at sample 243, Throw (356) at sample 262, and Zelda takes the
5% catch hit followed by the 12% throw hit. These are separate parity targets:
the miss cannot establish that capture callbacks or victim state work.

| Input file | Required motions | Other expected event |
| --- | --- | --- |
| `fighter-dive-miss-inputs.fmr` | 353, 35 | No contact |
| `fighter-dive-catch-inputs.fmr` | 353, 355, 356 | Damage to P2 |
| `fighter-air-dive-miss-inputs.fmr` | 354, 35 | Airborne launch, no contact |
| `fighter-dive-air-victim-inputs.fmr` | 353, 355, 356 | Zelda jumps before being caught |
| `fighter-air-dive-catch-inputs.fmr` | 354, 355, 356 | Falcon jumps before catching Zelda |
| `fighter-dive-interrupted-inputs.fmr` | 353, 80 | Zelda's dash attack hits P1 before a catch |

The interruption recording starts from the grounded catch inputs, but adds a
single Zelda attack press as she runs in. Zelda enters dash attack (53) at
sample 229, P1 enters grounded Dive (353) at sample 231, then takes 13% and
enters damage state (80) at sample 240. The same inputs reach this sequence
for both Captain Falcon and Ganondorf. Use `--expect-motion 353
--expect-motion 80 --expect-damage-port 1` to require the interrupted path.

`captain-side-contact-inputs.fmr` and
`captain-side-air-contact-inputs.fmr` place Zelda in Raptor Boost's path.
Grounded Falcon enters start state 349, then hit state 350; airborne Falcon
enters 351, then 352. Both deliver one hit to P2. Run with `--stocks 2`,
`--expect-damage-port 2`, and the corresponding two `--expect-motion` values.
The current scripted Captain Falcon clone matches stock across all 520 inputs
in each case. These fixtures exercise the transition caused by hurtbox
detection, which a side-special entry recording would miss.

`ganon-side-contact-inputs.fmr` and
`ganon-side-air-contact-inputs.fmr` exercise the corresponding Gerudo Dragon
fighter-contact transitions. Ganondorf starts left of Zelda and approaches
facing right in these recordings. The grounded path enters 349 then 350; the
airborne path enters 351 then 352, and each damages P2. The airborne input
omits Falcon's earlier taunt so Ganondorf's jump is accepted before the side
special. Run each with `--stock-fighter ganondorf --stocks 2`,
`--expect-damage-port 2`, and its two expected motions. The current scripted
Ganondorf clone matches stock over all 520 inputs in both cases; this checks
contact, not every branch of the move.

`fighter-left-neutral-inputs.fmr` taps left while P1 is idle to face away
from Zelda, then starts grounded neutral special with a centered stick. P1
faces left at sample 202 and enters Falcon Punch or Warlock Punch (347) at
sample 231. It checks the left-facing startup and animation path, with no
target hit. Both packaged clones match stock over all 380 samples; run with
`--expect-motion 347`.

`drmario-taunt-interrupted-inputs.fmr` uses Dr. Mario as P1 and Zelda as P2
on Final Destination. P1 enters taunt motion 341 at sample 171; Zelda's jab
damages P1 at samples 196, 200 and 204, interrupting the taunt and its
attached pill. With the authored seven-state vitamin article, taunt, and
neutral constructor, stock Dr. Mario and the packaged clone match over all
520 samples, including 1,038 fighter states, 29 actions, 24 item states,
four damage events and 519 RNG states. Run with `--expect-motion 341
--expect-damage-port 1`. This checks one damage-interruption timing, not
every article removal or interaction path.

`drmario-pill-contact-inputs.fmr` brings both fighters close, completes a
taunt, then throws Megavitamin at sample 271. The pill hits Zelda for 8% at
sample 284. Stock and authored clone match over 520 samples: 1,038 fighter
states, 26 actions, 79 item states, one damage event and 519 RNG states.
Run with `--expect-motion 343 --expect-damage-port 2`. The completed taunt
also reaches command 2 and removes its attached pill before action end.

`drmario-pill-shield-inputs.fmr` uses the same path, but Zelda shields before
the pill reaches her. She enters shield-hit motion 181 at sample 284 and takes
no damage. Stock and authored clone match over 520 samples: 1,038 fighter
states, 30 actions, 79 item states and 519 RNG states. Run with
`--expect-motion 343`.

`drmario-pill-reflected-inputs.fmr` uses Mario as P2. After repositioning,
Mario begins Cape at sample 282 while Dr. Mario throws a pill. At sample 292,
the pill changes owner from P1 to P2 and reverses horizontal velocity. Stock
and authored clone match over 520 samples: 1,038 fighter states, 32 actions,
152 item states, two damage events and 519 RNG states. Run with `--p2
human:mario --expect-motion 343 --expect-item-kind 49
--expect-item-owner-transfer 49 1 2`. The trace's owner, position and velocity
fields make the transfer visible; the exact item digest also checks the
remaining projectile state.

`drmario-cape-reflects-pill-inputs.fmr` reverses that interaction with Dr.
Mario on both ports: P2 throws a pill left and P1 starts Cape. The pill
transfers from P2 to P1 at sample 292, with its velocity reversing. Run with
`--stocks 2 --p2 human:drmario --expect-motion 345 --expect-item-kind 49
--expect-item-owner-transfer 49 2 1`. Stock and the authored Cape clone match
over 520 samples, including 154 item states and two damage events.

`drmario-cape-interrupted-inputs.fmr` starts Cape just before Zelda's jab.
The attached article is present at samples 189–195, then Zelda damages P1 at
sample 196 and the article is gone. Run with `--stocks 2 --expect-motion 345
--expect-damage-port 1 --expect-item-removed-on-damage 84 1`. Stock and clone
match over 520 samples, including four damage events. The generic removal
assertion checks that the tracked article disappears on a damage sample, not
merely that Cape was entered earlier in the recording.

`drmario-pill-clank-inputs.fmr` uses Dr. Mario in both ports. They throw pills
at one another; the pills meet at sample 289 and both invoke the shared item
clank callback. Neither fighter takes damage. Stock and authored clone match
over 520 samples: 1,038 fighter states, 32 actions, 89 item states, two item
clank events and 519 RNG states. Run with `--p2 human:drmario --expect-motion
343 --expect-item-kind 49 --expect-item-event clank 49`.

`drmario-death-respawn-inputs.fmr` uses **two stocks**. Dr. Mario walks off
Final Destination and loses a stock at sample 196, leaves his respawn platform
at sample 401, and throws a pill after respawning (neutral special at sample
471). Stock and clone match over 520 samples, including 1,038 fighter states,
26 actions, 36 item states and 519 RNG states. Run with `--stocks 2
--expect-motion 0 --expect-motion 343`. This reaches the scripted death hook,
but does not precharge Tornado or leave a Cape active before death.

The eight `drmario-special-*-inputs.fmr` files isolate the four special-move
entries on the ground and in the air. Ground inputs press B with a neutral,
right, up, or down stick at sample 100. Air inputs walk Dr. Mario off Final
Destination, then press B at sample 110. Run each with `--stocks 2` and the
corresponding expected P1 motion below. The installed scripted clone and stock
Dr. Mario match over all 520 samples in every case, including fighter states,
action transitions, item states, and RNG states. These entries do not establish
late-move contact, interruption, landing, or charged-Tornado parity.

| Special | Ground motion | Air motion |
| --- | ---: | ---: |
| Neutral | 343 | 344 |
| Side | 345 | 346 |
| Up | 347 | 348 |
| Down | 350 | 350 |

`drmario-tornado-repeat-inputs.fmr` starts grounded Tornado at sample 100,
taps B again at samples 104, 112, 120 and 128, then starts a second Tornado at
sample 200 with another tap at 204. The first tap occurs while command 2
permits the lift; P1's upward velocity changes at sample 105. Both uses
complete by sample 280. Stock and scripted Dr. Mario match across all 520
samples. Run with `--stocks 2 --expect-motion 350`.

The three `drmario-{up,side,down}-contact-inputs.fmr` files reuse the close
fighter positioning and taunt setup from the pill-contact recording, then
start a different grounded special at sample 270. All three stock-versus-clone
traces match over 520 samples. Up special hits Zelda five times at samples
273–299 for 14% total; Cape hits once at sample 282 for 12%; Tornado hits
five times at samples 278–318 for 14% total. Use `--stocks 2`,
`--expect-damage-port 2`, and expected P1 motion 347, 345, or 350,
respectively. These are contact paths, not complete move acceptance.

The grounded `drmario-up-contact-inputs.fmr` also reaches Luigi's Super Jump
Punch when P1 is selected as Luigi. Reusing it with `--stock-fighter luigi`
matched stock and the scripted Luigi clone over 520 samples, including two
damage events. The corresponding `drmario-up-air-contact-inputs.fmr` reaches
Luigi's airborne Super Jump Punch and likewise matches over 520 samples with
two damage events. These are contact checks; steering, reversal, ledge,
interruption, and landing alternatives still need their own recordings.

The grounded and airborne `drmario-down{-air}-contact-inputs.fmr` files
reach Luigi's Cyclone when P1 is Luigi. Both match the scripted clone over
520 samples with contact damage. The `drmario-tornado-repeat-inputs.fmr`
recording also matches Luigi over 520 samples, covering repeated B taps.
Run with `--stock-fighter luigi --stocks 2`; the grounded motion is 357.
The authored clone explicitly clears Cyclone charge on load/death, which is
an intentional change outside these uncharged-start recordings. See the
[Cyclone translation](../../../docs/scripting/cyclone.md) for the transition
flags and remaining coverage.

The four `drmario-{neutral,side,up,down}-air-contact-inputs.fmr` files add a
P1 jump at sample 262 to the same close-position setup, then start an airborne
special at sample 270. Stock and clone match all 520 samples in each case,
including the item and damage traces. Run with `--stocks 2`,
`--expect-damage-port 2`, and the expected motion below.

| Air special | P1 motion | Zelda damage |
| --- | ---: | --- |
| Megavitamin | 344 | 8% at sample 284 |
| Cape | 346 | 12% at sample 282 |
| Super Jump Punch | 348 | Five hits, 14% total by sample 299 |
| Tornado | 350 | Five hits, 14% total by sample 318 |

Two later Final Destination recordings reach the ledge during airborne Dive.
They need separate inputs because Ganondorf's jump trajectory differs from
Captain Falcon's. Falcon enters motion 354 at sample 236 and grabs the ledge
(252) at sample 293; Ganondorf enters 354 at sample 221 and grabs the ledge
at sample 280. Run with `--expect-motion 354 --expect-motion 252` and the
corresponding stock fighter/package. Earlier jump and ledge-grab attempts
that happened before Dive were discarded.

| Input file | Fighter | Required motions |
| --- | --- | --- |
| `fighter-dive-ledge-falcon-inputs.fmr` | Captain Falcon | 354, 252 |
| `fighter-dive-ledge-ganon-inputs.fmr` | Ganondorf | 354, 252 |

After building and installing a clone package in an isolated content store,
run, for example:

```powershell
python tools/run_fighter_parity.py --iso "C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso" --inputs tests/native/parity_inputs/fighter-air-kick-hit-inputs.fmr --stock-fighter captain-falcon --package-id PACKAGE_ID --content-root .local/captain-kick-store --expect-motion 359 --expect-motion 360 --expect-damage-port 2
```

The runner saves both traces under `.local/parity-runs/` for diagnosis. A match
proves parity on only that recorded path. Other unrecorded branches remain
separate acceptance work.

`luigi-fireball-shield-inputs.fmr` records a grounded Luigi Fireball against
Zelda's shield. P1's old taunt and early neutral-special presses are cleared
from the contact setup so Zelda can enter guard at sample 279. The Fireball
hits her shield at sample 287 and is removed in that frame, so there are no
end-of-frame item samples. Stock Luigi and the scripted clone match over 520
controller samples, 1,038 fighter states, 27 action transitions, zero damage
events, and 519 RNG states. Run with `--stocks 2 --expect-motion 341` and the
Luigi script package. This checks one shield timing; other shield conditions
and repeated contacts still need recordings.

Four Green Missile recordings pin down charge and misfire before those stock
handlers are translated. They all use two stocks, P1 Luigi, P2 Zelda, and 520
samples. Stock and the current clone (script-owned Fireball and fighter
events, inherited Green Missile) match every traced fighter/action/item/
damage/RNG record. These matches are **baseline evidence**, not proof that
Green Missile has been translated.

| Input file | Required P1 motions | Distinguishing path |
| --- | --- | --- |
| `luigi-green-missile-held-release.fmr` | 343, 344, 347, 351 | Hold B through sample 329; release enters normal launch at 331. |
| `luigi-green-missile-max-charge.fmr` | 343, 344, 347, 351 | Continue holding B; the charge limit forces launch at 359. |
| `luigi-green-missile-misfire.fmr` | 343, 344, 348, 351 | Seed 3 reaches the misfire launch at 331. |
| `luigi-green-missile-air-held-release.fmr` | 349, 350, 344, 347, 351 | Start airborne, land into grounded Hold at 302, then release. |

The existing `captain-side-contact-inputs.fmr` supplies a quick-release path
for Luigi (343 → 347 → 351). See the
[Green Missile translation ledger](../../../docs/luigi_green_missile_translation.md)
for the corresponding stock source decisions and still-needed scripting API.

The fourteen `purin-{pound,sing,rest}-*-inputs.fmr` recordings use Jigglypuff,
two stocks, human Zelda as P2, items off, and Final Destination. Each has 520
samples. Left-facing variants turn with negative horizontal input at samples
160-163; aerial variants jump at sample 200. The special starts at sample 230.
Pound left keeps horizontal input negative at entry; air-up/down add vertical
steering at samples 231-245. All fourteen current recordings match the
script-owned Pound, Sing and Rest template after named parameter import.
They cover entry, travel and completion/landing without contact damage.

| Input family | Required motion |
| --- | --- |
| `purin-pound-ground-{left,right}-inputs.fmr` | 363 |
| `purin-pound-air-{left,right,up,down}-inputs.fmr` | 364 |
| `purin-sing-ground-left-inputs.fmr` / `purin-sing-air-left-inputs.fmr` | 365 / 366 |
| `purin-sing-ground-right-inputs.fmr` / `purin-sing-air-right-inputs.fmr` | 367 / 368 |
| `purin-rest-ground-left-inputs.fmr` / `purin-rest-air-left-inputs.fmr` | 369 / 370 |
| `purin-rest-ground-right-inputs.fmr` / `purin-rest-air-right-inputs.fmr` | 371 / 372 |

For example, after building and accepting the independent Jigglypuff package:

```powershell
python tools/run_fighter_parity.py --iso "C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso" --inputs tests/native/parity_inputs/purin-sing-air-right-inputs.fmr --stock-fighter jigglypuff --package-id PACKAGE_ID --content-root CONTENT_ROOT --stocks 2 --expect-motion 368
```

See the [Jigglypuff ledger](../../../docs/jigglypuff_script_translation.md)
for coverage limits.

### Jigglypuff Rollout

`purin-rollout-{short,full}-ground-{left,right}-inputs.fmr`,
`purin-rollout-short-air-{left,right}-inputs.fmr`, and
`purin-rollout-turn-ground-{left,right}-inputs.fmr` contain 1,200 samples.
The initial B hold starts at sample 230; short ground release is 285,
full release is 650, and air release is 253. Turning input starts at 286.
Air cases jump at 224. Left-facing cases turn at 160-163.

Require motion 350 for ground release, 358 for air release, 349 for full
charge, and 351 for turns. Rightward ground charges additionally require
P2 damage and P1 motion 362. These checks prevent neutral or missed-branch
recordings from being accepted as Rollout evidence. All eight compare
stock versus the fully scripted move using the same seed and inputs.

The manual [Jigglypuff costume batch](../purin_costume_parity.py) uses
`purin-rollout-short-ground-right-inputs.fmr` for all five stock costumes and
fresh independent skin packages. It also compares attachment pose/material
hashes and requires a real P1 attachment for every non-default costume.

### Fox and Falco Reflector

`fighter-reflector-{ground-hold,ground-turn,ground-jump,air-hold-land,air-turn,reflect-pill}-inputs.fmr`
contain 1,200 samples each. Grounded Reflector begins at sample 100; aerial
inputs jump at 85 and begin the move at 110. Hold ends at 300. Turn input
starts 35 samples after entry; jump-cancel input starts 50 samples after entry.
The pill case holds Reflector at 260 and has Dr. Mario fire a pill at 270;
require a real ownership transfer of item kind 49 from P2 to P1 and motion 362.

`fighter-reflector-platform-drop-inputs.fmr` is the Falco Battlefield case.
`fox-reflector-platform-drop-inputs.fmr` jumps left onto a side platform instead:
Fox's grounded vertical jump cannot reach the center platform. Both require
motions 361 and 366 so failing to stand on a platform cannot pass as coverage.

The [manual suite](../fox_falco_reflector_parity.py) imports source defaults into
owned parameters, packages each script and compares fourteen stock/clone paths
with rendering enabled. Run it with `--iso PATH_TO_YOUR_ISO` after a Release build.

### Fox/Falco side dash

`fighter-side-{ground,air,short}-{right,left}-inputs.fmr` covers startup,
travel, recovery and the authored illusion/phantasm article. Shortening presses
B during travel; its grounded recovery starts at sample 143 rather than 144.
`fighter-side-contact-right-inputs.fmr` first closes spacing and waits through
RunBrake, then starts side-B at input sample 150. It must record the trail kind
56/57, motions 347/348/349 and damage to P2 (Zelda): 7% at sample 173 for Fox,
171 for Falco. Two stocks, Final Destination, items off, 520 samples.

Run `tests/native/fox_falco_side_parity.py --iso PATH` to compare both stock
fighters against fresh independent, owned-parameter packages. The trace includes
the second trail model. See the [ledger](../../../docs/fox_falco_script_translation.md#side-dash-checkpoint--2026-09-28)
for evidence and remaining coverage.

### Fox/Falco up special

`fighter-fire-{ground,air}-{up,left,right,diagonal}-inputs.fmr` enters up-B
at sample 120 with B+up, then steers during charge through sample 180.
Air cases jump at sample 100. The ground left/right cases must enter grounded
travel (355); other cases must enter aerial travel (356). All require their
charge motion (353/354). Rightward ground travel exercises grounded recovery;
rightward aerial travel exercises landing during aerial recovery.

`fighter-fire-ground-default-inputs.fmr` returns the stick to neutral during
charge and requires aerial travel/recovery. `fighter-fire-air-down-inputs.fmr`
jumps first, steers down after charge starts, and requires rebound (359).
These Final Destination recordings contain 520 samples, two stocks, items off.

`{fox,falco}-fire-platform-down-inputs.fmr` jumps onto a Battlefield platform,
starts grounded charge at sample 180, then steers down through sample 240.
Each has 1,200 samples and requires grounded charge followed by aerial travel,
covering the soft-floor skip operation. The two fighters use different jump
inputs because Fox cannot reach the center platform with the same jump.

The [manual up-special suite](../fox_falco_fire_parity.py) imports owned defaults,
builds fresh independent packages, and compares all 22 stock/script recordings
with rendering enabled. `--extra-only` limits a run to default-direction,
rebound and platform cases. Expected motions prevent a missed move from being
accepted as parity evidence.

### Fox/Falco radio taunts

`fighter-radio-{right,left}-inputs.fmr` has 1,200 samples, two stocks and items
off. The left recording turns at samples 90..93. Down is pressed at sample
120 and released at 121; a second down-release at 650/651 tests the ordinary
entry gating while the first conversation is in progress. Required motions
are 370/372/374 for right and 371/373/375 for left. Both use human Zelda P2.

The [manual radio suite](../fox_falco_radio_parity.py) builds fresh owned-value
packages and compares both directions on Corneria and Venom for both fighters.
Run with `--iso PATH`; optional `--fighter Fox|Falco` and
`--stage corneria|venom` restrict the batch. Audio-dependent dialogue completion
uses normal pacing; this is intentionally not an accelerated replay suite.
See the [ledger](../../../docs/fox_falco_script_translation.md#radio-and-lifecycle-checkpoint---2026-09-28)
for evidence, native asset fixes and coverage limits.
