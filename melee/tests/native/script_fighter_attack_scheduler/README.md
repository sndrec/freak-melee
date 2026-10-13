# Attack and crouch scheduler

This bounded hidden console compiles actual frontend, PrepareProgram,
publication, typed VM/runtime, Fighter actor, GObj fixed pools and the current
attack/crouch entry bodies from the six production units. Colliding static
identifiers are renamed only in the private extraction. Native lifetime guards
and dispatch branches remain intact; the fixture calls those actual bodies.

```powershell
python tests/native/script_fighter_attack_scheduler/run.py `
  --compiler .local/baseline72/compiler/clang.exe `
  --output A:/FreakMelee-test-archives/script-fighter-attack-scheduler-fresh
```

Final receipt:
`A:/FreakMelee-test-archives/script-fighter-attack-scheduler-review5/receipt.json`:
1,015 checks / 102 cases / zero stock-handler dispatches.

Authored source declares all eight contracts: crouch_wait_enter, jab_enter,
rapid_jab_enter, down_tilt_enter, aerial_attack_motion, aerial_attack_enter,
forward_smash_start and forward_smash_enter. Three independently prepared
programs exercise complete declarations, Start without Enter, and absent attack
hooks. The actor's physical kind is Game & Watch, so missing source callbacks
would expose an unintended raw character switch.

Coverage includes:

- Both JabEnter paths replace the character decision; missing handlers take
  the actual common jab entry and respect pickup consumption.
- Rapid and down-tilt replacements run without an extra common pickup or motion
  inserted by the scheduler. The actual common rapid helper separately tests
  consumed pickup and pickup → motion → animation ordering.
- Aerial mapping receives the native C-stick-selected motion. Ten selections
  cover zero/common/owned motions, the common-to-owned gap, negative values,
  signed extremes and both owned boundaries. Invalid results retain the native
  selection. Entry notification follows motion and animation startup.
- Forward Smash Enter suppresses Start/common entry and receives the exact F32
  angle after facing is assigned. Start without Enter follows successful common
  motion and animation. Missing handlers use genuine common behavior.
- Both crouch entry variants preserve common motion, optional held animation
  setup and branch-specific common flags before the source notification.
- Every reached pickup/motion/animation/held/common boundary is interrupted by
  physical GObj deletion, a real nested VM error while the GObj remains live,
  and native generation advancement. Cached Fighter continuations stop at the
  interrupted boundary. Hook faults/removal also stop replacements and
  notifications without invoking raw stock handlers.

Ordinary pickup, motion, animation, held setup, common flags and graphics leaves
are controlled in the private console. Authored bodies, source registration,
storage writes and execution are the real VM; faults are produced by a real
source Tick callback hitting a rejecting ordinary position-write leaf. Native
engine leaf trace order is compared directly. The existing frozen scheduler
boundary is reused read-only; its private motion/position leaves are instrumented
in the generated copy. Compiler/source/header dependencies are hash checked.
Each process has a 120-second bound and uses `CREATE_NO_WINDOW`.

This proves the connected scheduler/ABI/ownership contracts. It does not prove
full Game & Watch source, assets, animation results, collision/physics, rendering
or gameplay parity. Current direct bindings still do not implement the source
`attack.enter_jab` / `attack.enter_rapid_jab` operations; no unknown import,
Definition or authored native callback substitutes for them here. No game,
global build or shared source beyond the six assigned production units is edited.
