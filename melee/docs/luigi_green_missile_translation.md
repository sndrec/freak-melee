# Luigi Green Missile translation ledger

Green Missile's entry, charge, normal and misfire launch, flight, and end
callbacks are authored in [`examples/luigi_wip.fms`](../examples/luigi_wip.fms).
This ledger maps the source path and keeps the observed stock-versus-clone
parity boundaries explicit.
The source of truth for decisions is
`src/melee/ft/kinds/ftLuigi/ftluigispecials.c`; motion IDs and callbacks
come from `ftluigi.c` in the same directory.

| Phase | Ground motion | Air motion | Script-owned decisions |
| --- | ---: | ---: | --- |
| Start | 343 | 349 | Divide entry speed by traction, initialize charge and RNG in the one-shot transition callback, then enter Hold with the stock sound-preservation flag. Ground/air startup physics and collision differ. |
| Hold | 344 | 350 | Increment charge, launch on B release or the max-charge limit, clear effects and reinstall the accessory callback after animation loops, and preserve the matching motion across ground/air transitions. |
| Normal launch | 347 | 353 | Spawn the effect, recalculate active hitbox damage from charge, and enter Flight when command 0 fires. |
| Misfire launch | 348 | 354 | Use misfire velocity and effect path; enter Flight when command 0 fires. |
| Flight | 345 | 351 | Apply launch velocity, gravity/deceleration, landing and wall collision, and on-damage transition. The shared facing-aligned velocity callback is installed. |
| End | 346 | 352 | Apply end friction, finish animation or landing, and leave for ordinary wait/fall. |

The stock entry registers `ftLg_SpecialS_SetVars` in `x21EC` immediately
before changing into Start. `Fighter_ChangeMotionState` invokes that one-shot
callback during the **Start entry** and then clears `x21EC` (see `fighter.c`,
near the motion-change callback dispatch). Charge initialization and the
`HSD_Randi` misfire roll therefore happen on the initial side-B press,
not when Start enters Hold. Moving the random call to Hold would change the
shared RNG sequence. The stock code also reads the horizontal smash-input
timer `x673` to choose its charge-rate start and sets the shared
`count_thrown_items` bit in that branch. Its damage and flight velocity depend
on the resulting charge frame count. These details need explicit script
state and generic engine operations, not a retained Luigi handler.

The script uses `callbacks.track_state_change()`, `random.int(...)`,
`input.special_held`, named DAT attributes, common fall/friction operations,
and per-motion `action` callbacks. The
[`state.switch_to_air/ground`](scripting/fighter.md#stateswitch_to_air-and-stateswitch_to_ground)
operations perform ground/air motion changes with the stock preservation
flags. The generic
[`input.horizontal_smash_age`](scripting/fighter.md#inputhorizontal_smash_age)
and [`stats.count_as_thrown_item()`](scripting/fighter.md#statscount_as_thrown_item)
APIs cover the entry's smash-input branch and statistic flag. The generic
[`hitbox.active/set_damage`](scripting/fighter.md#hitboxactive-and-hitboxset_damage),
[`callbacks.align_velocity_to_facing()`](scripting/fighter.md#callbacksalign_velocity_to_facing),
[`effects.spawn_on_bone()`](scripting/fighter.md#effectsspawn_on_bone), and
[`effects.install_hitlag_marker()`](scripting/fighter.md#effectsinstall_hitlag_marker)
operations cover the launch hitbox, shared velocity callback, and effect cue.
The [fighter scripting reference](scripting/index.md) documents their
signatures and links them to the callback lifecycle.

The scripted collision pairs use the stock masks: Start `0x0c4c5284`, Hold
`0x0c4c5286` (adds `KeepGfx`), and both launch variants `0x0c4c5086`
(retains `KeepGfx` but omits `KeepSfx`). Their generated callbacks switch the
ground/air condition before changing motion at the current animation frame.
Start and End script DAT-parameterized ground and air deceleration,
and their air callbacks apply the stock bounded fall behavior. Hold and the
normal launch use ground friction on ground and
[`air_gravity_and_friction`](scripting/fighter.md#physicsair_gravity_and_friction)
in air. Misfire launch calls ground friction in both variants, as the stock
source does. Flight physics, Flight-to-End transitions, End animation and
collision, and ten stock no-op interrupts are also scripted. In particular,
Flight collision calls the shared landing test, then checks
[`collision.wall_contact`](scripting/fighter.md#collisionwall_contact) from
the resulting environment flags, matching the source's two separate checks.
Start/Hold/Launch animation and Hold interrupts, along with entry, charge
setup, hitbox damage, and effects, are authored. Ground and air Flight use
[`state.enter_at`](scripting/fighter.md#stateenter_at) to preserve the current
launch frame and stock motion flags.

## Recorded acceptance paths

The four Green Missile input files are in
[`tests/native/parity_inputs/`](../tests/native/parity_inputs/). They use 520
samples on Final Destination with two stocks. With the full scripted
Green Missile implementation installed, stock Luigi and the clone match every
traced fighter, action, item, damage, and RNG record on these paths:

| Input | Key P1 transitions | Trace result |
| --- | --- | --- |
| `luigi-green-missile-held-release.fmr` | Start 271, Hold 288, release/normal launch 331, flight 335 | 35 actions, one damage event |
| `luigi-green-missile-max-charge.fmr` | Start 271, Hold 288, automatic launch 359, flight 363 | 34 actions, one damage event |
| `luigi-green-missile-misfire.fmr` | Start 271, Hold 288, misfire launch 331, flight 335 | 34 actions, two damage events; initial RNG seed 3 |
| `luigi-green-missile-air-held-release.fmr` | Air Start 271, Air Hold 288, ground Hold 302, launch 331, flight 335 | 38 actions, one damage event |

The existing `captain-side-contact-inputs.fmr` is the quick-release case:
Start 271, normal launch 288, flight 292. The airborne hold case specifically
tests an air-to-ground transition during charge. These are reference paths,
not evidence that every ground-to-air, wall, shield, interruption, and misfire
branch has been covered.
The airborne Hold recording transitions through Flight 351, Air End 352,
Ground End 346, and Wait; these phases match stock with their scripted
physics/animation/collision callbacks installed. The four Green Missile
recordings, the quick-release recording, and the Fireball contact regression
all match stock after the entry, charge, launch, and effect callbacks were
replaced.
Ground-to-air transitions, wall
impacts, and other less common branches need separate recordings.
