# Direct interpreter damage, status, effects and RNG

This is the source-checked family in `native_script_binding_combat.c` and the
typed native binding catalog. Operations require a live source with the
retained generation and use the existing native engine primitives directly.
No generated C, authored C callbacks, stock-special handler or legacy host
adapter participates in execution. Arguments marshal to the listed formal
types before the native call; Float is binary32. This family does not complete
every stock script or establish live game parity.

## Damage and hurt status

| Fighter expression/call | Result | Access and behavior |
| --- | --- | --- |
| `damage.song_immune` | Bool | Read/write `Fighter.x2222_b2`. |
| `damage.armor` | Float | Read/write general armor `dmg.armor0`. |
| `damage.jump_armor` | Float | Read/write jump armor `dmg.armor1`. |
| `damage.in_hitstun` | Bool | Read-only `x221C_b6`. |
| `fighter.self_damage(amount: Float)` | Void | Existing self-damage primitive; non-positive amounts do nothing. Positive amounts preserve native percent cap, metal/collision-health changes and player bookkeeping. Boss kinds are rejected before mutation to avoid boss stamina delegation. |
| `damage.heal(amount: Float)` | Void | Existing heal primitive; subtracts amount, floors percent at zero and publishes native HP/healing credit. Negative amounts retain the native increase behavior. |
| `Hurt.normal`, `Hurt.invincible`, `Hurt.intangible` | Int | The three source-defined hurt-state values 0, 1 and 2. |
| `hurtboxes.status` | Int | Read-only aggregate body status from `ftColl_8007B868`, including star/status priority. |
| `hurtboxes.set_all(status: Int)` | Void | Sets every configured hurt capsule, clears its position-update skip, and updates the fighter's aggregate capsule flag. |
| `hurtboxes.set_body_status(status: Int)` | Void | Sets the body status and applies the native color-animation channel. |

Status setters accept only 0..2. The capsule count must fit the fighter's fixed
15-capsule storage. Fields and setters retain their separate meanings: body
status includes native color behavior, while setting all capsules does not
rewrite body-status timers. HP and bonus helpers retain their existing integer
bookkeeping conversions; calculations and Float arguments are not promoted
through an extra Double stage.

Damage reaction transitions, boss actions, hitbox solvers/setters, recipient
damage and named motion resolution remain outside this batch. Unsupported
operations fail; they do not dispatch a stock fighter special.

## RNG

These methods are available for fighter, article and model-effect sources:

| Call | Result | Native draw behavior |
| --- | --- | --- |
| `random.float()` | Float | One `HSD_Randf` seed update, with the exact native binary32 result. |
| `random.int(upper: Int)` | Int | One `HSD_Randi(upper)` draw when upper > 0; otherwise returns 0 without advancing the seed. |
| `random.int_excluding(upper: Int, first: Int, second: Int)` | Int | Selects from the ascending allowed values with one `HSD_Randi(allowed_count)` draw. Out-of-range exclusions are ignored, duplicate exclusions count once. Returns -1 without a draw when upper <= 0 or no value is available. |

The native RNG is the engine's existing seed stream. This binding owns no
separate seed or heap. `int_excluding` maps the chosen rank through at most two
excluded values in constant time. It preserves the original algorithm's
result and final seed, including empty and duplicate cases.

Positive upper bounds above 32768 fail **before seed advancement**.
`HSD_Randi` multiplies a signed bound by a native 16-bit draw before dividing;
32768 is the largest bound for which that product cannot overflow for any
draw. Inputs first receive the language's normal Int conversion. These calls
mutate RNG state, so read-only callbacks reject them.

## Existing effect control

All of these are fighter-only zero-argument Void calls:

| Call | Behavior |
| --- | --- |
| `effects.destroy_all()` | Destroys owned effects and clears matching effect parameter entries; preserves `effects.active`. |
| `effects.clear_all()` | Clears `effects.active`, then performs native destruction. |
| `effects.pause()` | Marks effects belonging to this fighter as entering pause, preserving their async bit. |
| `effects.resume()` | Clears pause state while preserving the async bit. |
| `effects.pause_during_hitlag()` | Clears authored PreHitlag/PostHitlag bindings, then installs the existing native pause/resume primitives in those slots. Requires an attached owned actor. |

Hitlag pause uses native primitives as engine hooks, not authored C bodies.
It intentionally supersedes authored handlers in those two slots, matching
the existing source operation. Authored handler installation and ownership
rules are documented in `native_script_setup_bindings.md`.

The actual shared `efLib_DestroyAll` function now snapshots a registered
fighter/article/model-effect generation and checks it after native
Cancel/Free destructor boundaries. Retirement or reuse stops destruction
before continuing or reading the parent object again. Scene objects outside
those registries retain the prior behavior. The public binding also checks
the source before execution and before output publication; effectful VM calls
stop later authored statements when the source retires.

Effect spawning, joint/name selectors, wind, async/model recipe construction
and model-effect script receivers remain separate unresolved families.

## Focused proof

`tests/native/script_combat/run.py` builds the real direct source index,
statement compiler, catalog, bytecode and VM, plus the actual native RNG.
The fixture mechanically extracts source bodies for self damage, healing,
hurtbox status, effect clear/destroy/pause/resume and the original RNG exclusion
algorithm. Player bookkeeping, color animation, JObj traversal and effect
Free/Cancel leaves are controlled; no gameplay helper body is replaced with
dummy arithmetic.

Evidence includes exact RNG selection and seed comparison across small,
duplicate, invalid, exhausted and maximum-bound cases; native damage/status
comparisons; Float argument marshalling; and actual destruction on both effect
lists. Cancel/Free retirement and generation replacement stop later authored
writes and result publication. This is isolated contract evidence, not a live
game run or full effect-spawning/rollback integration proof.
