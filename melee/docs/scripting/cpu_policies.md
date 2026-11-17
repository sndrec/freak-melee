# CPU behavior policies

[Script callbacks](language.md#predicate-callbacks) | [Owned attack profiles](cpu_profiles.md) | [Stage queries](stage.md#terrain-queries)

The shared CPU planner chooses targets, considers attack candidates and runs
queued controller commands. A fighter can replace its motion classifications
and decisions at the callbacks below. Use ordinary fighter variables for
charges and other move state. These callbacks never call a donor fighter's
special handlers.

All 27 CPU events are connected to the direct interpreter's native scheduling
sites. Declarations are checked for exact argument and result types before a
package is published. An absent event uses its native default. In particular, the mode-6 follower
and final Blizzard tails remain reachable when `cpu_mode_update` or
`cpu_output` is missing or returns false; an authored follower must consume
the phases it owns. Other declared policy hooks replace their corresponding
stock-kind branches. Initial spacing is 3.5; periodic spacing is 4.0 times the existing
random factor. Missing stick overrides preserve CPU-level defaults, and missing
recovery uses the common diagonal recovery command.

Callback failure stops native continuation. This includes a VM fault while the
fighter object is still alive, as well as removal or reuse. The scheduler checks
the retained object, fighter data, generation, program and state storage before
continuing. These checks require no per-tick allocation.

For complete mode/output ownership and delayed controller input, see
[Follower CPU and delayed inputs](cpu_history.md). These hooks can implement
companions without exposing a fighter-specific follower handler.

## Callback summary

| Callback | Purpose |
| --- | --- |
| `on cpu_mode_update` / `on cpu_output` | Return true to consume the current mode or final output phase; false continues shared CPU policy. See [CPU history](cpu_history.md). |
| `on cpu_pending_action` | Handle the shared planner's pending-action branch. True completes that branch; false resumes common planning. A declared callback replaces stock fighter-kind policy. |
| [`on cpu_prepare`](#on-cpu_prepare) | Choose early mode/follow policy and return a full fighter ID for initial input-history seeding, before common mode initialization. |
| [`on cpu_drop_command`](#on-cpu_drop_command) | Queue a platform/drop response after common neutral input and squat handling. True owns the remaining sequence; false keeps the common response. |
| [`on cpu_stick_step` / `on cpu_stick_limit`](#on-cpu_stick_step-and-on-cpu_stick_limit) | Choose the shared travel planner's stick change step and magnitude limit. |
| `on cpu_initialize` | Set fighter-owned approach spacing after common CPU initialization. A declared callback replaces the stock spacing choice. |
| `on cpu_spacing_update` | Update approach spacing on the shared planner's 600-tick interval, after its normal random draw. `event.value` supplies the resulting `1 - r*r*r` factor. A declared callback replaces the stock spacing multiplier. |
| `on cpu_attack_motion` | Return whether the current motion counts as an attack. Common normal attacks and item swings/shots already return true before this query. |
| `on cpu_grab_motion` | Return whether the current motion is a custom holding/grabbing state. Common CatchWait already returns true before this query. |
| `on cpu_attack_decision` | Customize decisions while attacking. Return true to finish this planner step or false to continue the shared attack logic. |
| `on cpu_travel_decision` | Customize the attack follow-up step after the shared NULL-target exit. True finishes the step without an implicit counter increment. The callback owns `cpu.travel_frames` changes for handled cases; false continues ordinary common selection. This preserves behaviors which deliberately return without advancing the travel counter. |
| `on cpu_capture_decision` | Customize decisions while holding a victim. Return true to finish this planner step or false to continue shared throw/pummel logic. Not called without a victim. |
| [`on cpu_projectile_defense`](#on-cpu_projectile_defense) | Own the response after common projectile-defense gates. True finishes the step; false continues common defense without a stock fighter-kind response. |
| `on cpu_pummel_special` | Return true to select the shared B-button pummel sequence instead of A. The common grab planner still owns its random draw and attempt counter. |
| `on cpu_ranged_candidates` | Add fighter-specific ranged candidates after the common candidate is considered. A declared hook replaces the stock candidate switch. |
| `on cpu_ranged_allowed` | Additional ranged eligibility, after the shared target/behavior checks and before the level gate. Return false to stop this path. |
| [`on cpu_projectile_hit`](#on-cpu_projectile_hit) | Classify the first successful predicted projectile contact using its sampled horizon and unblockable flag. |
| [`on cpu_special_candidate`](#on-cpu_special_candidate) | Consume a special-choice decision using target angle and distance; set `cpu.selected_command` explicitly when needed. |
| `on cpu_threat` | Called on a potential attacker with an observing defender. Return whether the move's additional geometry threatens that observer, replacing the stock threat switch. |
| `on cpu_charge_decision` | Customize charging after common target, facing and level checks. Return true to finish this step or false to allow its normal neutral-input finish. |
| `on cpu_charge_cancel` | Queue cancellation input at the shared planner's charge-cancellation point. The common behavior guard runs first; a declared callback replaces the stock cancellation choice. |
| [`on cpu_recovery`](#on-cpu_recovery) | Assemble recovery input at the planner's recovery point, replacing the stock recovery switch. |
| `on cpu_falling_recovery_allowed` | Filter the size-dependent gate inside the common falling-recovery query. False rejects recovery; true continues its destination-angle, wall and floor checks. Its forced-fall, airborne, rising and thrown-state gates run first. |
| `on cpu_falling_destination_reached` | Replace the destination-height decision in the falling planner after its floor, airborne, rising and wall checks. Return whether this falling state should trigger recovery input. This is separate from the size/angle eligibility query above. |
| `on cpu_available_jump` | Queue an alternative available-jump sequence. True means the sequence is complete; false continues the common destination-directed jump. The shared forced-forward-jump branch runs first. |

A declared policy replaces that fighter's stock-kind branch, even when it
returns false. False permits the remaining **common** logic; it does not
request delegation to the donor. Classification callbacks are queries: do
not enter motions or enqueue input from them. Ordinary event callbacks are
void; classification/decision predicates return booleans. `cpu_special_candidate` returns a boolean and writes its chosen command through
`cpu.selected_command`; `cpu_projectile_hit` returns an integer contact category.

`event.value` is available only directly inside `cpu_spacing_update`.
Pass it as a typed helper argument when needed elsewhere. Its random draw
has already happened: drawing again would alter the gameplay RNG sequence.

```text
parameter float initial_spacing = 8.5
parameter float spacing_range = 9

action CpuPolicy:
    on cpu_initialize:
        cpu.set_approach_spacing(attrs.initial_spacing)
    on cpu_spacing_update:
        cpu.set_approach_spacing(attrs.spacing_range * event.value)
```

See [fighter parameters](language.md#fighter-attributes) for editing these
values independently of behavior compilation. Candidate tables remain in
[`cpu_profile.fcp`](cpu_profiles.md); callbacks do not replace those tables.

## Properties

| Expression | Meaning |
| --- | --- |
| `cpu.level` | Current planner level. Ordinary versus CPUs use levels 1â€“9. |
| `cpu.behavior` | Read/write current shared planner behavior index, rather than a fighter motion ID. |
| `cpu.previous_behavior` | Read-only saved planner behavior to resume after a temporary decision. |
| `cpu.travel_frames` | Read/write signed attack follow-up counter. Increment it explicitly in handled callbacks when the behavior requires that; the planner does not add a second increment. |
| `cpu.selected_command` | Read/write signed shared attack-command selector, independent of the fighter's motion ID. |
| `cpu.pending_action` | Read/write pending-action flag sampled by the shared planner. This is planner state, not a persistent move charge. |
| `cpu.elapsed_frames` | Read-only signed shared CPU frame counter. |
| [`cpu.target_id`](#selected-target) | Read-only UInt creation ID of the planner's selected fighter; zero when no target exists. Use generic target queries for its position and air state. |
| `cpu.grab_attempts` | Current capture planner's attempt counter. |
| `cpu.approach_spacing` | Fighter contribution to the shared approach distance. |
| `cpu.floor_in_bounds` | Whether the latest shared probe found usable floor below the fighter inside the blast bounds. This is a cached planner result, not a new terrain query. |
| `cpu.body_half_width` | Read-only half of the combined left/right body bounds used by the shared CPU threat test. Also readable on another fighter, including stock fighters. |

These five planner fields run in the ordinary interpreter:
`travel_frames`, `selected_command`, `elapsed_frames`, `floor_in_bounds`, and
`body_half_width`. The first two accept exact signed Int writes; the rest are
read-only. They access existing native planner fields without allocating,
probing terrain, incrementing counters, or drawing randomness. The script owns
any counter increment or attack selection. `floor_in_bounds` remains the
planner's cached result, rather than recomputing a floor query.

Mutable planner fields require the current owned fighter source. Body width is
an observation and can be read through a retained fighter receiver such as
`other.cpu.body_half_width` or `attachment_owner.cpu.body_half_width`.

### `cpu.set_approach_spacing(distance)`

Set the current fighter's spacing contribution. It is engine planner state;
use named project parameters for defaults, rather than treating it as an
attribute offset or as a custom move's persistent variable.

### `cpu.clear_target()`

Clear only the planner's fighter target. Restore `cpu.behavior` from
`cpu.previous_behavior` and queue any completion commands explicitly when the
policy requires those additional effects.

### `cpu.run_script(index)`

Append an existing common CPU command script to the planner's fixed controller
command buffer. The integer expression is evaluated once. Indices `1..61` are
valid; an invalid index does nothing. This runs the same engine operation used
by the stock planner, without another random draw, changing targets or updating
the grab-attempt counter.

Use it when a decision callback chooses a complete command sequence. It differs
from `cpu.add_candidate`, which queues a possible choice for later selection,
and `cpu.queue`, which appends individual controller commands. Do not enqueue
commands from pure classification or eligibility callbacks.

```text
on cpu_capture_decision:
    if capture.victim_damage < 100:
        cpu.run_script(57)
    else:
        cpu.run_script(58)
    return true
```

The script owns any probability or level gate around that choice. Returning
false instead allows the common throw/pummel planner to continue.

### Selected target

`cpu.target_id` reads the current planner target; it does not find a new opponent
or change target selection. It is available directly in the interpreter for
Fighter sources. The UInt preserves all 32 creation-ID bits. Zero means the
planner has no target. The binding admits the selected physical Fighter before
reading its identity; a stale pointer, foreign scene or retired caller fails
without publishing an absent substitute. Stock and independently authored
fighters are valid targets. Reading allocates nothing and draws no RNG.

```text
let int target = int(cpu.target_id)
let float x = 0
let float y = 0
let float z = 0
if targets.read_position(target, x, y, z):
    let float separation = x - position.x
```

Use [generic target observations](targets.md#retained-fighter-references) for
positions and air state. To test shared ground support, check that both fighters
are grounded and use `stage.same_line_island(floor.line, targets.floor_line(target))`.
That compares actual non-null island identity, not equal endpoints. For
world-position probes, use [`stage.compare_floor_islands`](stage.md#stagecompare_floor_islands).

## Projectile defense

### `on cpu_projectile_defense:`

The callback receives the integer `event.category` directly in its body.
The shared planner uses category 1 for detected fighter threats and category 2
for detected item/projectile threats. Its special item predictor can produce
category 3 for a hitbox flagged for the alternate evasive response. Category zero, current roll/air-dodge/spot-dodge motions and
current shield motion are handled before this callback. It runs for airborne
categories 1–3. On the ground it runs only for category 2, after the shared
Flipper, Barrel Cannon and mushroom-item responses. Those item priorities
remain common engine behavior.

Return true after queuing a complete response. Return false to use the common
category response; false does not execute the stock fighter's special branch.
An absent callback preserves the stock branch and its RNG order. The common
caller verifies the source's live generation after the callback, before any
cached fighter access. `event.category` is unavailable in other callbacks;
pass it as an integer helper argument when necessary.

The Fox/Falco template queues down-B for category 2 in the air, and retaps R
for airborne categories 1 and 3. On the ground its script owns the probability
of choosing down-B; false allows the common shield response. Probability,
level-dependent delay and command values are ordinary editable parameters.

## Ranged candidates and threats

### `on cpu_special_candidate:`

Select an immediate attack-command index from the planner's target geometry.
`event.angle` is the signed elevation in radians, calculated from target Y
minus fighter Y and the absolute horizontal separation. Targets below the
fighter have negative angles; targets directly left and right have the same
elevation. `event.distance` is their two-dimensional world-space distance.
These expressions are available only directly in this callback. Pass them
as typed arguments when calling a helper.

Return true to consume this decision, or false to continue common horizontal
attack selection. A declared hook replaces the stock fighter's special-choice
branch. Assign `cpu.selected_command` to choose a shared attack script; the
property accepts integers `0..61` and ignores out-of-range assignments. Zero
clears the command. Returning true without assigning preserves the current
command, including zero: consumption and command choice are separate policies.

Target/behavior, cooldown, terrain, held-item and random gates run first.
`event.angle` and `event.distance` are already calculated; do not repeat those
planner operations. `cpu.add_candidate` instead appends to a list for later
selection.

```text
on cpu_special_candidate:
    if not grounded and event.angle < -1 and event.distance > 40:
        cpu.selected_command = 38
        return true
    return false
```

The [Samus template](../../examples/samus_full.fms) uses this general query for
her airborne bomb choice. Command indices refer to the shared controller
command scripts, not fighter motion IDs or article IDs. The same callback
can choose other special attacks without adding fighter-specific API names.

### `cpu.ranged_unsafe(distance)`

Run the existing ranged geometry probe against the planner's current target.
It returns true if terrain blocks the target segment, or if no floor is found
at the supplied distance ahead of the fighter, from five units above to 1000
below its Y position. It performs no random draw and changes no planner queue.
The name describes the geometry result; the script decides whether to add an
attack, avoid it or choose an alternative. Stock Bowser adds command 38 when
this probe is true. Preserve that choice when translating stock behavior.

### `cpu.add_candidate(command)`

Append a shared input-command script index, `1..61`, to the planner's existing
eight-entry attack-candidate list. Invalid indices or a full list are ignored,
matching the bounded stock insertion. This is distinct from `cpu.queue`, which
appends controller bytecode. Use this operation from `cpu_ranged_candidates`;
it does not select an attack or immediately press a button. Candidate ordering
and duplicates affect the shared planner's later selection.

### `on cpu_threat:`

This query runs on the potential **attacker**, not on the observer. Its ordinary
fighter properties describe the attacker. `other.position.x`,
`other.position.y` and `other.cpu.body_half_width` describe the observing
defender, and are available only in this callback. Pass their values as helper
arguments when necessary; a helper does not inherit the observer context.

```text
on cpu_threat:
    let float probe_x = 0
    let float probe_y = 0
    let int probe_line = -1
    let bool blocked = stage.read_floor(position.x, position.y, other.position.x, other.position.y + 5, probe_x, probe_y, probe_line)
    blocked = blocked and not stage.cpu_floor_excluded(probe_line)
    if blocked: return false
    return position.x + cpu.body_half_width + other.cpu.body_half_width > other.position.x
```

That partial example illustrates a terrain/geometry query, not a complete move
policy. See [floor segment queries](stage.md#floor-blocking-in-cpu-decisions)
and the [Bowser template](../../examples/koopa_wip.fms). Keep threat queries
free of state writes, transitions and RNG; ordinary hitbox prediction remains
part of the shared defensive planner.

## `on cpu_recovery:`

Queue recovery input using the same fixed command buffer as other CPU
decisions. The shared planner has already chosen a destination and reached
its recovery step. The callback returns no value and replaces that fighter's
stock recovery sequence. Finish the sequence explicitly with `Cpu.done`.

`cpu.destination.x` and `cpu.destination.y` read the planner's world-space
destination. It is a two-dimensional point; there is no Z property. It need
not be the position of the opponent. Use ordinary owned parameters for
distance thresholds, stick magnitudes and waiting durations.

```text
on cpu_recovery:
    cpu.queue(Cpu.stick_y, 127)
    cpu.queue(Cpu.stick_x, 0)
    cpu.queue(Cpu.press_b_for, 1)
    cpu.queue(Cpu.release_b)
    cpu.queue(Cpu.wait, 15)
    cpu.queue(Cpu.x_toward_destination, 127)
    cpu.queue(Cpu.stick_y, 0)
    cpu.queue(Cpu.wait, 15)
    cpu.queue(Cpu.done)
```

This illustrates a two-stage recovery sequence, not a general solution for
every move. A script can branch on destination geometry or draw from the
shared RNG; it owns the order and number of those draws. The
[Pikachu/Pichu template](../../examples/pikachu_wip.fms) reproduces its
far/near recovery decisions this way.

## Queued input

### `cpu.queue(command[, argument[, second_argument]])`

Append one command to the existing fixed 256-byte CPU command buffer. The
command must be a recognized constant literal with exactly the listed argument
count. Prefer the named `Cpu` constants below. Computed commands and unknown
opcodes are load errors; an explicitly recognized numeric literal is accepted.
Arguments are ordinary expressions converted to integers, then converted to
the engine's byte encoding. Converting through an integer preserves negative
stick values held in floating-point script locals. Stick axes use signed values from -127 to 127; triggers use 0â€“255.
Durations and other indices fit in one byte. `wait_if_motion` consequently
cannot represent motion IDs above 255.

The interpreter checks the complete command footprint before calling the native
one-, two- or three-byte append routine. Invalid/full queue pointers stop the
script without a partial append. Native per-byte asserts could leave a partial
command before failure; that invalid-input behavior is deliberately not preserved.
Floating operands truncate toward zero, then use defined byte modulo conversion.
Nonfinite or out-of-I32 floating conversion is diagnosed before mutation.

| Commands | Arguments |
| --- | --- |
| `Cpu.press_a`, `Cpu.release_a` (also `b`, `x`, `y`, `r`, `l`, `z`, `up`, `down`, `right`, `left`, `start`) | None. Press/release a controller button. |
| `Cpu.release_all` | None. Release every held button. |
| `Cpu.done` | None. Terminate this command sequence. |
| `Cpu.stick_x`, `Cpu.stick_y`, `Cpu.cstick_x`, `Cpu.cstick_y` | Axis value. |
| `Cpu.trigger_r`, `Cpu.trigger_l` | Trigger value. |
| `Cpu.press_a_for`, `Cpu.release_a_for` (also `b`, `x`, `y`) | Duration in frames. |
| `Cpu.wait` | Number of frames to wait. |
| `Cpu.toward_destination`, `Cpu.x_toward_destination` | Stick magnitude toward the planner's destination; the second affects only X. |
| `Cpu.toward_target`, `Cpu.x_toward_target` | Stick magnitude toward the planner's fighter target; the second affects only X. |
| `Cpu.forward` | Stick magnitude along current facing. |
| `Cpu.wait_if_motion` | Motion ID: wait one frame if it is still current. |
| `Cpu.set_behavior` | Shared planner behavior index. |
| `Cpu.toward_destination_clamped`, `Cpu.x_toward_destination_clamped`, `Cpu.forward_clamped` | Increment and magnitude clamp for the corresponding direction calculation. These add to the current stick value; they are not lower/upper bounds. |

Queue commands from decision/cancellation callbacks, where the planner is
assembling its next input sequence. They do not immediately change the
fighter's current input or reset the queue. Keep sequences bounded: overflowing
the fixed buffer hits the engine assertion. `Cpu.done` is explicit because
some decisions append input and then allow common logic to finish the queue.
Do not use this API to inject player input from ordinary move callbacks.

```text
func cancel_charge():
    cpu.queue(Cpu.press_r)
    cpu.queue(Cpu.wait, 1)
    cpu.queue(Cpu.release_r)
```

This helper does not finish the queue. Its caller decides whether to append
more commands and whether to return true. The [DK example](../../examples/donkey_wip.fms)
uses the same helper at two planner decision points and uses
[`stage.island_bounds`](stage.md#stageisland_boundsx-y-z-probe_y-found-left_x-left_y-right_x-right_y)
to choose a reachable floor edge while carrying a victim.

These operations preserve the shared planner; they are not a complete
replacement AI framework. Stock comparison coverage is recorded in the
[translation ledger](../donkey_script_translation.md).

## Nearest opponent

### `cpu.nearest_enemy_at(x, y, z)`

Return the nearest eligible opponent's full creation ID, or integer zero when
none is available. The position is supplied explicitly; it need not be the
fighter's position or the planner's current target. This is a fighter-only query
and does not change the current target, enqueue input or draw from the RNG.

```text
let int enemy = cpu.nearest_enemy_at(position.x + 20, position.y, position.z)
if enemy != 0:
    let bool target_in_air = targets.airborne(enemy)
```

The shared Melee opponent filters and fighter-list order apply. Distance uses all
three axes. Equal distances keep the first eligible fighter in that list, as the
original query does. The returned ID preserves all 32 bits in an integer; later
[target queries](targets.md#retained-fighter-references) resolve it again rather
than exposing a fighter pointer. The interpreter validates its owned caller and
physical target before publishing the U32 ID. The native selector's missing or
rejected-candidate result is zero; caller retirement stops continuation. No
finite position filter or replacement tie-breaker is added.

## Early preparation and travel controls

### `on cpu_prepare:`

Return an Int fighter creation ID to seed initial controller history. Zero
requests neutral origin `(0,0,0)` and facing `1`. The engine resolves the returned
ID when it copies history; a missing or recycled target uses the same neutral
fallback. See [retained fighter references](targets.md#retained-fighter-references).

The hook runs before the CPU mode switch, floor probe, random timer draws and
history setup. Supplied mode, level and planner input are already initialized;
follow distance starts at40 and the combat-candidate gate starts false.
`event.history_source` is a read-only Int hint for a live, published counterpart
of this owned fighter family, or0. A constructing second actor can receive its
primary's hint before it appears in a Player slot. This does not loosen
[`family.member_id`](paired_fighters.md#member-identity)'s published-source contract.
The script can use the hint, return another live fighter ID, or return0.

```text
move CpuPreparation:
    on cpu_prepare:
        cpu.mode = int(6)
        cpu.follow_distance = 15
        cpu.combat_candidates_enabled = true
        return event.history_source
```

The subsequent common reset still clears `cpu.combat_candidates_enabled`.
This write ordering preserves original startup behavior; the flag is not a
persistent setting applied after reset. Common history setup writes only the
fields stock initialization writes, leaving other retained record bytes alone.
`on cpu_initialize` remains the later approach-spacing hook, after history and
common random initialization. Initial seeding alone does not implement ongoing
follower input recording or fighter-specific behavior within a planner mode.

| Property | Type | Meaning |
| --- | --- | --- |
| `cpu.mode` | Int | Shared planner mode. No invented numeric bound; the common switch has a default branch. |
| `cpu.follow_distance` | Float | Distance/tolerance used by shared follow travel. Assignment requires a finite, nonnegative value; an invalid value leaves the current value unchanged. Int values can be assigned as Float. |
| `cpu.combat_candidates_enabled` | Bool | Common attack-candidate generation and the near-target A/R retap decision gate (`xF9_b2`). It does not enable or disable every combat behavior. |

### `on cpu_drop_command:`

Common neutral stick input is queued first. The common squat/SquatWait/SquatRv
fast path returns before this hook. For other motions, return true after queuing
all remaining commands, or false to continue the common level-dependent response.
This policy is used by platform travel and no-enemy rebirth, rather than being
limited to respawn behavior.

```text
move DropPolicy:
    on cpu_drop_command:
        cpu.queue(Cpu.wait, int(1))
        cpu.queue(Cpu.stick_y, int(-127))
        cpu.queue(Cpu.wait, int(5))
        cpu.queue(Cpu.stick_y, int(0))
        cpu.queue(Cpu.wait, int(1))
        cpu.queue(Cpu.done)
        return true
```

### `on cpu_stick_step:` and `on cpu_stick_limit:`

Return an Int in `0..127`. Step controls the travel planner's stick change;
limit controls its magnitude cap. Step runs before limit. Each callback may
read ordinary attributes and fighter state. A missing or invalid result retains
that component's common value rather than clamping or narrowing it. For an
unknown stock level, an absent/invalid component preserves the caller's output,
as the original switch does. Normal CPU levels use the existing stock table.

```text
move StickPolicy:
    on cpu_stick_step:
        return int(64)
    on cpu_stick_limit:
        return int(127)
```

Both outputs publish only after both callback continuations are valid. Removing,
recycling, changing the owned data/kind or replacing the script Definition stops
the current operation before either output or a later travel caller is used.
Fighter life reset similarly reports failure to its direct callers when early
policy retires the actor. Its normal new spawn identity is a successful reset,
not a stale-source failure.

## Source lifetime during decisions

The native attack/travel planner checks the fighter's complete lifetime
generation after an authored attack decision, travel decision, attack-motion
classification or special-candidate callback. If that callback removes the
fighter or reuses its object for a new fighter, the current continuation stops
before changing the old fighter's command or planner counters. A live travel
decision returning true owns its travel-counter mutation. Increment
`cpu.travel_frames` explicitly when that decision should advance it; returning
true does not add an implicit increment.

The shared callers of the special-candidate query also check the source before
using the returned command. This is a lifetime boundary around those specific
callbacks, not a replacement for ordinary planner rules or permission to
mutate state from classification queries. See [synchronous callbacks](synchronous_callbacks.md)
for source identity and callback lifetime in other engine paths.

The actual native source fixture covers 86 cases / 1,128 assertions, including
removed sources, protected old object pages, address reuse with a new generation,
invalid command indices and nested candidate continuations. Its outer selection
coverage is limited to the exact candidate block, rather than the whole CPU planner.

### `on cpu_projectile_hit:`

Classify the first projectile hitbox that passes the common predicted contact
geometry. `event.prediction_frames` is the already-sampled prediction horizon;
`event.unblockable` is that hitbox's unblockable flag. Return an integer `0..3`:
0 ignores the contact, 1/2/3 select the common planner's contact categories.
Out-of-range results become zero. The common owner/ally/item filtering, random
horizon draw, ordered hitbox checks and swept contact prediction run first.
The hook runs once for the first successful contact and does not ask the engine
to keep scanning when zero is returned. It is distinct from
[`cpu_projectile_defense`](#on-cpu_projectile_defense), which decides the later
defensive response. This callback does not receive an item pointer or change
projectile collision behavior.

## Destination and scalar controls

These direct interpreter operations use the current live owned Fighter and
retain its physical object, generation, kind, fighter data, pinned program
and state storage before native continuation and result publication. They
allocate no per-tick storage and install no follower policy.

| Operation | Direct interpreter behavior |
| --- | --- |
| `cpu.prepare_destination_update()` | Bool; clear the destination latch when the current behavior differs from both saved behaviors. Behavior 4 returns false; other behaviors clear the recovery-destination flag and return true. This mutates planner state. |
| `cpu.set_destination(x, y, spacing)` | Void; numeric arguments convert to Float. A nonfinite argument is a successful no-op. When the native destination latch is zero, update the coordinates and spacing, then run common stage-entry adjustment. A nonzero latch preserves the destination. |
| `cpu.set_approach_spacing(spacing)` | Void; assign the Float approach contribution directly, including negative and nonfinite values, without clamping. |
| `cpu.approach_spacing` | RO Float; current approach contribution. |
| `cpu.mode` | RW Int; native CPU mode, with no range clamp. |
| `cpu.follow_distance` | RW Float; finite nonnegative assignments update the value. Negative or nonfinite assignments are successful no-ops. Negative zero is accepted. |
| `cpu.combat_candidates_enabled` | RW Bool; common combat-candidate gate. |
| `cpu.ranged_gate_enabled` | RW Bool; common ranged gate. |
| `cpu.level` | RW Int; read a live owned or stock Fighter, write a live owned Fighter only. No level clamp. |

All operations in this table except `cpu.level` are source-only in the language
and cannot be projected through related receivers. `cpu.level` remains
available through related Fighter receivers: an owned target accepts writes;
a stock target supports reads only. Mode and level assignments use the
interpreter's signed 32-bit Int; Float assignments are not accepted for them.

```text
func ChooseDestination():
    if cpu.prepare_destination_update():
        cpu.set_destination(position.x + 20, position.y, 5)
    cpu.set_approach_spacing(4)
    cpu.follow_distance = 15
    cpu.combat_candidates_enabled = true
```

Use [CPU history](cpu_history.md) when decisions depend on previous inputs or
actions. Follow the [CPU checks](authoring_checks.md#cpu-behavior) to compare
ordinary decisions, recovery and unavailable targets.