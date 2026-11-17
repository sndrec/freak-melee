# Bounded authored CPU history and destination controls

The optional Bool events `on cpu_mode_update` and `on cpu_output` run at the
original common CPU mode-dispatch and final output sites. Return `true` to
consume that phase. A missing event or a healthy `false` result continues the
shared native policy. Fallthrough still reaches the native mode-6 follower
and final Blizzard tails
when their native conditions hold. An authored follower must return `true` at
the phases it owns to prevent those native decisions. Both sites validate the
retained object, generation,
userdata, kind, fighter data, pinned program and live script storage after the
callback, including logical VM faults. The continuing CPU tick caller repeats
those checks after each of its five phases. Complete
independent follower scripts must consume the applicable phases; these events
do not automatically turn a native mode6 follower into an authored follower.

## Direct interpreter support

The direct interpreter supports all seven CPU history/planner methods below,
plus destination and scalar controls in [CPU policies](cpu_policies.md#destination-and-scalar-controls).
They operate on the current live owned Fighter, retain its physical identity,
pinned program and storage, and allocate no per-tick storage. `record_input`
requires an exact signed 32-bit Int; casting it to the native creation ID
preserves all bits. Zero or a missing input source returns false. A selected
stock or independently authored Fighter is retained through recording.

`read_history_pose` requires four distinct writable Float destinations and
publishes all four only after a successful native read and source validation.
False preserves every destination. `apply_recorded_input` changes controller
output only; it does not move the actor or choose a target.

The four planner calls run the unchanged common native mechanisms. They can
reenter ordinary authored CPU policies; their native lifetime fences and the
binding's retained source/target checks stop continuation or publication when
an actor, program or storage retires. They introduce no follower handler.
Cached target refresh and native random draws remain native policy.

`cpu.output.left_x/left_y`, `left_trigger/right_trigger` and `buttons` are
implemented read/write Int fields. Signed sticks accept -128..127; unsigned
triggers accept 0..255. Out-of-range byte assignments are successful no-ops.
Buttons preserve all 32 bits, exposed through the signed Int view. These calls
and fields are source-only in the language.

The target observations and stage CPU queries below are also direct interpreter
APIs. See [target observations](targets.md#native-target-observations) and
[stage queries](stage.md#floor-and-line-island-reads) for their live-identity and
output contracts.

## Native contracts

These methods are fighter-only. Creation IDs and stage lines use Int; they
never travel through Float. All vector/scalar output names must be distinct,
writeable scalar state fields or typed locals of the specified type. A failed
read preserves every output. Interpreted calls evaluate inputs in written order
and stop callback/helper continuation if an effectful argument or Host query
retires or recycles the source.

| Method | Result and behavior |
|---|---|
| `cpu.record_input(Int id)` | Bool; record one live target's controls, current position and facing in the receiver's existing30-record native history. |
| `cpu.read_history_pose(Float x, Float y, Float z, Float facing)` | Bool; read the current delayed sample. Reject invalid cursor or nonfinite pose/facing before publishing any output. |
| `cpu.apply_recorded_input()` | Bool; copy delayed left/right sticks, buttons and both trigger bytes into CPU output. It does not move the fighter or select a target. |
| `cpu.prepare_combat_plan()` | Set the common combat gates, retain original cached-enemy selection and update common/special item targets. |
| `cpu.prepare_destination_update()` | Bool; clear the destination latch on a behavior change and consume the common destination-update gate. Behavior4 returns false. |
| `cpu.update_recovery_destination()` | Run the actual common recovery-destination geometry; script still owns follower eligibility and recovery-lock state. |
| `cpu.plan_attack()` | Bool; actual common attack planner with its original result. Statements may discard that result. |
| `cpu.plan_without_target()` | Run the actual common no-target planner. |
| `cpu.set_destination(x, y, spacing)` | Numeric arguments converted to Float; nonfinite values leave the destination unchanged and return normally. Preserve the existing destination latch and actual common stage-entry processing. |
| `targets.read_position_delta(Int id, Float x, Float y, Float z)` | Bool; live fighter position delta, atomically published. Also available in article callbacks with `item.state`/Float locals. |
| `stage.read_floor(x0, y0, x1, y1, Float x, Float y, Int line)` | Bool; direct common floor probe with offset0 and no skipped/required line/joint or filter callback. Nonfinite inputs return false. |
| `stage.read_line_island_bounds(Int line, Float leftX, Float leftY, Float rightX, Float rightY)` | Bool; actual stage island endpoints, validated bounded line lookup. |
| `stage.same_line_island(Int a, Int b)` | Bool; both bounded live lines resolve to the same non-null island. |
| `stage.cpu_floor_excluded(Int line)` | Bool; the four existing common CPU stage-floor exclusion predicates. Invalid lines return false without querying stage consumers. |
| `stage.cpu_island_blocked(Int line)` | Bool; bounded live line resolves to the actual common item/stage hazard assessment. It owns no follower decisions and returns no retained pointer. |

`targets.damage`, `targets.facing`, `targets.cpu_body_half_width` read Float
values (absent0). `targets.sleeping` is Bool (absent false), and
`targets.floor_line` is Int (absent-1). These are ordinary live target reads,
not family-wide permission to access private authored state.

In the direct interpreter, `cpu.level` reads a live owned or stock Fighter and
writes a live owned Fighter only, including an owned receiver. Its Int
assignment has no level clamp. Other new CPU scalars and destination operations
are source-only in the language.

The direct interpreter also supports strict RW Bool
`cpu.combat_candidates_enabled` and `cpu.ranged_gate_enabled`, and
RO Bool `motion.allows_controller_sync`.

`cpu.output.left_x/left_y` are strict RW Int signed bytes
(-128..127); `cpu.output.left_trigger/right_trigger` are strict RW Int bytes
(0..255). Invalid runtime assignments preserve the previous output, without a
clamp. `cpu.output.buttons` is strict RW Int and preserves all32 bits.

Recording advances both existing cursors before writing, including wrap, and
preserves history bytes xA/xB that the source never writes. It accepts finite
normalized axes[-1,1]. Native decompiled negative-stick conversion through u8
is undefined for values whose truncated result is negative; authored recording
uses explicit signed truncation with factors127/128. This is a disclosed native
C boundary choice, not a claim about a retail console bug. Trigger bytes both
use triggers[0] directly, without multiplication by255. Finite trigger values
in(-1,256) have a representable truncated u8 result; other values fail without
mutating the collection. The initial history seed remains controlled by the
earlier `on cpu_prepare` contract. No per-frame allocation or second history
pool is introduced.

Test input copying and recovery with the
[CPU checks](authoring_checks.md#cpu-behavior). For a companion, repeat the test
while its leader is separated, incapacitated and removed.