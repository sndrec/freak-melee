# Interpreted explicit projectile launch checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Date: 2026-10-10. Ordinary fighter scripts execute through the typed
interpreter; generated C remains test-reference infrastructure only.

## Delivered

Two generic authored-Article launch calls now resolve and execute through the
interpreter:

- `articles.launch_at_position(kind,x,y,z,angle,speed,state,variant,group,facing)`
  retains explicit facing and all32 collision-group bits independently of its
  world-space launch angle.
- `articles.launch_projectile(kind,bone,offset_x,offset_y,offset_z,angle,speed,state)`
  transforms the bone-local offset and derives facing from the world-space angle,
  with variant/group zero.

Both use the existing `nativeScriptItemSpawn_Projectile` factory directly.
The recipe still controls lifetime, appearance entry, initial velocity/update
and automatic lifetime. Created and Launched hooks execute through the real
Item interpreter. No Bowser-specific API, alternate constructor or allocation
was added. Native/reference launch arithmetic and setup order remain unchanged.

The source resolver pins `kind` to a declared Article owner in the root Fighter
bank at preparation time, as with the other authored factories. Runtime kind
variables and recipient-qualified factories are not admitted yet. This boundary
is now explicit in the scripting documentation. Inputs require finite scalars,
valid native integer widths and an installed article state. Float collision
groups are rejected at compile time and direct runtime entry. The binding
requires current caller/item/program peers before publishing its Void result.

Supplied XYZ becomes the native constructor's previous position, with Z zeroed;
current input position is the fighter's ECB center. This matches the reference
projectile helper and stock `itKoopaFlame_Spawn` defaults. Article callbacks can
then place the projectile and compute their own velocity. These input positions
must not be confused with a full collision/model initialization result.

Bowser's source now advances past the launch call. Its two integer counters
previously narrowed the Float results of `max/min` implicitly. They now use the
same decrement/clamp and increment/clamp decisions as the original
`ftkoopaspecialn.c` loops, preserving Int counters without lossy conversions.
Other Float clamps remain unchanged.

## Evidence

- Connected source/compiler/VM/actor spawn cohort: **478 checks / 67 cases**.
  Fourteen direct-factory/binding pairs compare complete constructor inputs,
  setup order, velocity, position, facing, state, lifetime, launch angle/speed/
  origin and authored Launched output, with wrapped angles, negative/positive
  speed and initial velocity enabled/disabled.
- Both new method spellings execute from compiled source. The bone transform
  leaf records the exact joint and local offset; its translation is controlled,
  so real skinned matrix correctness is not claimed by that case.
- Compiler/runtime negatives cover dynamic kind selection, Float/wide groups,
  signatures, native widths, nonfinite values and absent states. Logical caller
  retirement during state, appearance, debug and initial-update setup leaves
  results unpublished and preserves the already-created source Item.
- All-factory regression: **3,150 checks / 24 cases**, including all seven
  factory families and 1,024 real constructor/free cycles.
- Positional constructor regression: **12,812 checks / 35 cases / 1,024 reuse
  cycles / 1,056 native frees**.
- Seven Windows targets build; `native_fighter_script` reference CTest passes.
- Read-only ten-source roster cohort retains six successful preparations.
  Bowser now reaches `capture.set_victim_capture_motion` at line 406; DK still
  reaches `capture.initialize_hold` at line 451. Fox/Falco retain their metadata/
  asset gates. This is not whole-fighter activation or match acceptance.

The older all-factory fixture initially crashed in `nativeMatchPool_Resolve`
from the actual Item constructor. A hidden CDB stack trace identified its
controlled creator helper, which left the newer physical creator reference
uninitialized. The runner now mechanically extracts the actual
`nativeItem_CaptureCreator` and `nativeItem_Creator` bodies. No production guard
was weakened. The source-binding fixture also supplies explicitly aborting
unselected article-release/class leaves for its newer provider dependencies.

## Receipts and limits

Private receipts under `A:/FreakMelee-test-archives/` were independently rehashed;
all consumed input pins remained unchanged:

| Receipt folder | SHA-256 | Pins |
| --- | --- | ---: |
| `script-projectile-launch-oct10-final-pinned` | `1ac15ae4d75a8d8b73f2192833b8b0d55549e9070b1f299e6bab9919df2ec264` | 1,702 |
| `script-projectile-launch-factory-oct10-b` | `fa149edb49bf220a1240c678db12fefac99c5a2b8b060d82b08e01611518f1d1` | 1,697 |
| `script-projectile-launch-position-oct10` | `bf177141255edf4411bb127b5e7db2be6361299c0fca9d187a1c73b13a5ccc8e` | 1,694 |
| `source-projectile-launch-oct10-b` | `586f2dc073c3a36a544e8772d1a01769d4eb9d58df8ca694ab8bec95125edeb7` | 27 |

Independent summary: `projectile-launch-independent-pins-oct10.json`.
Failed/partial fixtures and the CDB trace remain separate from final acceptance.

Immutable asset/spec publication, model construction, graphics/audio, transform
and collision leaves remain controlled. No installed assets changed and no
game window opened. Whole-fighter behavior, DAT/render parity and compact
whole-match rollback remain open; this checkpoint does not complete the goal.
