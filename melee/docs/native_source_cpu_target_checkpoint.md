# Source CPU controls and captured target queries

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The interpreter now exposes the actual planner's pending-action Bool RW,
behavior I32 RW, previous behavior I32 R and destination X/Y F32 R fields.
`cpu.clear_target()` clears the real selected-target pointer. These operations
require the exact published Fighter package/program, storage and physical scene.

`cpu.queue` lowers a recognized literal command to its native opcode and exact
arity.51 named recipes cover zero/one/two operands. Valid operands preserve
integer conversion and native byte modulo, including negative stick values.
The complete1/2/3-byte footprint is checked against the256-byte buffer before
calling native append functions. Invalid pointers/capacity or undefined floating
conversion stop before mutation; the native partial-write/assert failure case
is deliberately not reproduced. No reset, clamping, guessed opcodes or alias to
`run_script` is used. Computed command kinds remain unsupported.

`cpu.nearest_enemy_at` calls the actual native eligible-opponent selector. Its
3D distance, first-eligible tie policy, filters and nonfinite-coordinate behavior
are preserved. Both binding registry walks are capacity bounded and validate
pool/class before userdata. It returns U32 creation ID or0 on the native false
result, without changing target, input queue or RNG. Caller lifetime errors do
not publish an absent-result substitute.

`targets.read_position(handle,x,y,z)` captures one physical Fighter in the
caller scene, then publishes the raw `cur_pos` Vec3 only after caller/target
guards. Zero/missing IDs return false without output changes. Captured retirement,
kind/userdata/native generation/class or physical reuse fails rather than
retargeting. `targets.airborne` and `targets.is_fighter` share that admission.
Foreign and stock Fighter programs are valid targets; only the caller requires
its owned source bank. No Definition, native pointer value or coordinate
sanitization is introduced. Output overlap/types are rejected at load.

## Evidence and boundaries

Private receipts under `A:/FreakMelee-test-archives/`:

- `script-cpu-control-oct08-root-final/receipt.json`:7249 checks,612 exact native
  queue-byte pairs across51 recipes and80 native selector comparisons. Actual
  command/selector bodies, binding/catalog/properties/pool/registry and typed VM
  execute. Owned source package/view and stage/eligibility leaves are controlled.
- `script-cpu-control-oct08-b/receipt.json`:also records the actual constants
  regression,6569 checks across1041 constants,8 functions/11 defaults/0 host reads.
  New Cpu rows were corrected to lexicographic placement before DbLKind rows;
  enum values were not altered. The initially failed lookup test was rerun.
- `script-target-queries-oct08-root-final/receipt.json`:475 connected source/
  actor/VM checks and64 native reference tuples. F/A callers, foreign Fighters,
  full32-bit IDs, NaN/Inf/signed-zero bits, missing/wrong-class output preservation
  and guard-time replacement/retirement are exercised. The mutation hook is a
  controlled guard observation, not a claim that native field reads reenter.

Client/content tool builds and36 selected CTests pass. These console fixtures
are not complete DAT/scene/gameplay acceptance. Zelda's actual owned-data
source-check advances from CPU380 to Article786; Sheik still stops at Chain1543.
The remaining attachment roster migration is recorded separately in
[its audit](native_source_attachment_roster_audit.md):8 templates/12 blocks,
47 typed helper sites, nullable defaults and unchanged loop/signal/state structure.
That source audit is not full roster runtime acceptance; unsupported aliases
and other lifetime/primitive closures remain.

No game, installed-content update or user draft migration occurred. Continue
generic Item initialization/physics/collision and child construction, Chain
resource closure and whole-form runtime checks. Explicit rollback state remains
open, and the full project goal stays active.
