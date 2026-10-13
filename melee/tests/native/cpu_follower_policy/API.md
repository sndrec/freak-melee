# Proposed authored controller-history policy

Read-only SDK71 proposal. No interface here is implemented or accepted yet.
The accepted early CPU hooks remain unchanged. The source receipt and exact
body hashes are in `source_inventory.json`; extraction reproduction is in
`README.md`. Address shorthand B7AAC is incorrect: the destination body is
**A7AAC**. The ranged gate consumer is **B89CC**, not B885C (B885C implements
the ordinary low-level CPU activity duty cycle).

## Replacement placement and defaults

Add two optional immutable Definition callbacks:

```c
bool (*on_cpu_mode_update)(Fighter_GObj* source);
bool (*on_cpu_output)(Fighter_GObj* source);
```

DSL: `on cpu_mode_update:` and `on cpu_output:`. Both return strict Bool;
fighter context only, available in both generation routes and typed helpers.
`cpu_mode_update` executes at the beginning of B2AFC, before its mode switch.
True consumes that mode update; false preserves the original switch. The
follower script returns false only for modes it does not own. It returns true
for mode6 even when its explicitly selected target is absent: it then performs
the common no-target planning path. Missing target is not permission to run
B101C or A589C.

`cpu_output` executes at B0AF4 entry, after B3E04 command interpretation. True
consumes the final controller overlay; false preserves its original body. An
owned follower returns true even when copying is disabled or its target is
absent. No script call to B0AF4, B101C, CpuUpdateNanaFollow, A7AAC, inlineJ0,
inlineI3 or `ftPp_SpecialLw_Enter` is exposed.

B3900 retains this order: B33B0 destination/stage maintenance, B2AFC mode
update, B2790 command planning, B3E04 command interpretation, B0AF4 final
overlay, then x7C increment. At each actual continuing boundary which can
invoke authored callbacks, validate the original GObj generation, userdata,
kind, ft_data and Definition before another cached access. At minimum the
new mode and output calls and the already callback-bearing B2790 path need
these checks in B3900, not just inside callees. Audit B33B0 and B3E04 nested
callback paths as part of the same batch; neither the counter increment nor
a later phase runs after retirement or same-address/new-generation reuse.
Use actual direct Snapshot/Alive checks, not a forwarding dispatcher wrapper.

Absent hooks preserve all stock modes and raw stock partner lookup/output.
The proposal removes their reachability for an owned follower; it does not
rewrite healthy stock characters into a new policy by default.

## Explicit identity and authored state

Reuse strict `family.member_id(index)` only after publication. The early
`event.history_source` hint still seeds construction-time history through
the SDK71 resolver; it is not weakened into a global construction identity.
After publication the script chooses a full-width Int creation ID and stores
it in `state.history_target`. It can select a different live actor if desired;
history primitives do not infer a partner or restrict family policy in C.
Zero is invalid, high-bit IDs remain valid Int bit patterns, and no ID passes
through Float. Queries resolve the live registry on every operation.

Use ordinary authored Bool `state.copy_controls` and `state.recovery_lock`,
replacing xFA_b7/xFB_b0 decisions. Reset both in early prepare. The source's
final grounded reset of recovery_lock remains after both mode6 branches.
Keep the selected target independent of the combat enemy in cpu.x44.

The original A589C returns the first non-self same-player live-list actor,
and returns NULL if that actor has x221F_b3; it does not validate family or
immutable member identity. New scripts explicitly check `targets.sleeping`
for their chosen ID. This is intentional safe ownership replacement, not a
claim that arbitrary malformed multi-actor scenes select the same actor.

## Bounded history operations

Use the existing Fighter CPU 30-record inline array and two cursors. No new
heap, pool, family-shared history or rollback route. Initial write/read indices
remain5/0. Advance both before recording; wrap exactly at30. The cursor
offset is five recording updates, not a new configurable delay or necessarily
five game frames. Target changes do not silently reset previous records.

Proposed actual operations (fighter-only, substantive implementations):

| DSL | Result | Exact work |
| --- | --- | --- |
| `cpu.record_input(Int id)` | Bool | Resolve explicit live source; advance both cursors; write physical input[0], current position and facing into the new write record. Refactor B0918 itself into this operation. |
| `cpu.read_history_pose(outX,outY,outZ,outFacing)` | Bool | Read delayed record pose into four distinct Float locals/scalar fighter state outputs; publish atomically. |
| `cpu.apply_recorded_input()` | Bool | Copy delayed lstick/cstick, held buttons and both triggers to CPU outputs; no movement, eligibility, family lookup or special entry. |

All reject invalid caller/cursors before mutation. Validate cursor address and
alignment without subtracting or advancing an unrelated C pointer. Recording
rejects missing/stale targets and nonfinite/out-of-range normalized
sticks or triggers; position/facing must be finite. False leaves cursors,
records and outputs unchanged. Query failure leaves all output destinations
unchanged. Playback needs no surviving historical target: records are owned
values, not retained fighter pointers. The script separately checks its current
selected target before using them, matching healthy source policy.

Record writes preserve xA/xB and other records. Four stick axes use signed
truncate-to-int quantization, positive127*x and negative128*x, then store s8.
The native decompiled inlineM0 returns u8 from negative Float before s8
assignment: this is undefined C conversion, so signed quantization is the
explicit behavioral choice for negative inputs, not a retail-bug claim.
Defined nonnegative source branches compare strictly. Neither input clamping
nor a guessed retail bit pattern is required.

Both record trigger bytes receive the SAME Float `input.triggers[0]`, converted
directly to u8. Normalized0..1 therefore truncates to0 or1. Do not scale255,
substitute triggers[1], or silently repair this defined source behavior.
Physical held buttons preserve all32 bits. Input/runtime source tests must
include nonzero cstick and xA/xB sentinels.

## Minimal scalar and target reads

Existing SDK71 `cpu.mode`, follow_distance, combat_candidates_enabled,
behavior, previous_behavior, approach_spacing, body_half_width, destination, floor.line,
grab_attempts, level (RO), queue/run_script, target/held/motion/position queries
and ordinary math are reused. Only the following additions are needed:

| Name | Type/access | Meaning |
| --- | --- | --- |
| `cpu.level` | RW Int | Full native s32; script explicitly performs percent/20 truncation and upper bound9. No new lower clamp. |
| `cpu.ranged_gate_enabled` | RW Bool | xF9_b4. B89CC checks this after its Kongo position restriction and target-on-ledge fast return; it does not disable every ranged decision. |
| `cpu.output.left_x/left_y` | RW Int | CPU s8 axes; reject outside-128..127, no silent clamp. |
| `cpu.output.buttons` | RW Int | CPU held-button32 bits, preserving high-bit values. |
| `cpu.output.left_trigger/right_trigger` | RW Int | Native u8; reject outside0..255. |
| `motion.allows_controller_sync` | RO Bool | x2225_b3 common motion flag, reset on motion change and loaded from motion row x9_b0. |
| `targets.damage(id)` | RO Float | Actual live percent, absent0. |
| `targets.facing(id)` | RO Float | Live facing, absent0. |
| `targets.sleeping(id)` | RO Bool | Actual x221F_b3, absentfalse; presence guard required. |
| `targets.floor_line(id)` | RO Int | Actual collision floor.index, absent-1. |
| `targets.cpu_body_half_width(id)` | RO Float | Actual cpu.x564, absent0. |
| `targets.read_position_delta(id,outX,outY,outZ)` | Bool | Actual pos_delta; same atomic output and typed rules as read_position. |

No separate cstick setter is needed for this first translation: playback
copies it, and stopping copy intentionally retains it. No raw xFA/xFB flags,
union byte queries, target pointer getters or generic snapshot object.
Target reads are ordinary live actor queries, not same-Definition mutation.
Absence defaults never substitute for an explicit `targets.is_fighter` guard.
No new incapacitated query: A3200 is just CliffCatch/CliffWait, already readable
from targets.motion. Teeter and remaining excluded common motion ranges are
also authored comparisons.

## Common planner operations without a follower solver

B0760 mixes reusable planner preparation with a partner-dependent destination
choice. Do not expose the whole function as `cpu.follow` or `cpu.plan`.
Split its actual substantive code and adjust both stock and authored callsites:

1. `cpu.prepare_combat_plan()` owns the common flags, A4BEC cached-enemy/RNG
   selection and both item-target updates. It sets xF8_b0=true, xF9_b2/b4=true,
   xF9_b3/b5/b6/b7/b1=false, then enemy/common-item/special-item updates in order.
   Rename/extract the actual preparation body; stock B101C calls this common
   body followed by its original destination and ADE48 logic directly. Remove
   dead B0760 after all callsites are adjusted; no old-handler forwarding shim.
2. `cpu.prepare_destination_update() -> Bool` is the actual common
   CpuDataShouldAct/CpuShouldAct latch logic: behavior mismatch resets x60;
   behavior4 returnsfalse; other behavior clears xFA_b2 and returnstrue.
   It is a mutating operation, not a pure eligibility property. Consolidate
   the identical bodies and update their original callsites directly.
3. `cpu.update_recovery_destination()` directly exposes actual A8DE4 common
   ledge planning: xFA_b2 latch, x5C=10000, facing-ordered A4038/A3908 attempts.
   No partner lookup or fighter-specific recovery move is inside this body.
4. `cpu.plan_attack()` calls the actual common ADE48 command planner. Existing
   authored CPU hooks still own fighter-specific attack choices.
5. `cpu.plan_without_target()` is the actual AEFB8 common fallback body,
   renamed with direct callsite changes: A229C recovery test/AE7AC, otherwise
   A5CE0 special target and CpuTargetAct/ADE48, otherwise CpuInitNoTarget(true),
   common/special item updates and CpuActOnNoTarget. It is not simply neutral
   output. No Nana/Popo handling or inferred partner lives inside it.

A4BEC must remain the actual common cached-target operation: filters and
A1AB4 metric differ from cpu.nearest_enemy_at; selecting a new enemy consumes
one HSD_Randf and sets x30/x48. Substituting nearest-query changes RNG order.
Common no-target/recovery/planner paths retain their existing authored hooks;
any remaining raw fighter branches there are separate callback-closure audit,
not permission to hide a follower solver in these methods.

## Stage and destination operations

Translate A7AAC in FMS. Keep all air/ground branches, ray endpoints, island
decisions,5-unit margins and ledge exclusions as readable authored decisions.
Minimal missing data primitives, with Int line IDs and atomic outputs:

| Proposed operation | Actual body/input |
| --- | --- |
| `stage.read_floor(x0,y0,x1,y1,outX,outY,outLine) -> Bool` | mpCheckFloor; direct full arguments with original0 offsets, NULL/negative exclusions. Two Float outputs and one Int output, all distinct. |
| `stage.read_line_island_bounds(line,outLX,outLY,outRX,outRY) -> Bool` | Existing article line-island source operation, expose it in fighter context. No position/depth re-inference. |
| `stage.same_line_island(lineA,lineB) -> Bool` | Compare actual non-NULL mpIsland8005AB54 results; invalid linesfalse, no exposed pointer. |
| `stage.cpu_floor_excluded(line) -> Bool` | Actual A1B38 four stage-function checks: BigBlue, Inishie1, Corneria, Venom. |
| `stage.cpu_island_blocked(line) -> Bool` | Actual A2718 common active-item and Story/Zebes/Onett island hazards; resolve live line first. Missing islandfalse like source; invalid linefalse without unsafe lookup. |
| `cpu.set_destination(x,y,spacing)` | Actual A1F3C destination latch and A1CC4 stage adjustment, finite Float arguments. Not a raw x54 assignment. |

Validate stage line bounds before lookup, preserve endpoint order, and leave
output destinations unchanged on failure. The stage.read_floor query only
returns Bool and discards the required point/line. Existing positional island
query cannot replace a target's actual retained floor-line island on stacked
or moving floors. Shared A1CC4 stage behavior remains common; the script owns
which destination and spacing to request.

## Complete authored mode6 ordering

1. Set both candidate and ranged gates true. Resolve explicit target and check
   sleeping. If absent, call common plan_without_target and consume mode6.
2. Record target input, then compute Int level=target.percent/20, uppercap9.
   Nonfinite/out-of-int-range original narrowing is separately undefined.
3. If copy_controls: run authored A7AAC destination decisions BEFORE stop-copy
   check. Owned UpB motion range361..366 sets recovery_lock and stops copy.
   Otherwise distance>25 stops; otherwise eligibility false stops. Restore
   behavior=previous_behavior; zero left axes/buttons/both triggers only.
4. Otherwise prepare common combat plan; if prepare_destination_update,
   choose common recovery destination when recovery_lock, authored A7AAC
   otherwise; plan_attack. THEN evaluate B0E98 eligibility; iftrue set
   behavior0/copy_controls=true and both candidate/ranged gatesfalse.
5. Grounded clears recovery_lock after either branch.
6. Common B2790/B3E04 still run. At output stage require explicit live target
   and copy_controls. Before playback, authored synchronizer excludes common
   FireFlowerShoot/Air; if target owned groundLw357 and caller common motion,
   set facing=-target.facing and enter the caller's own down-special helper.
   Otherwise apply delayed inputs. If motion.allows_controller_sync, smooth
   ONLY cur_pos.x/y with original double0.95/0.05 math; outside caller owned
   Lw357/358 also copy recorded facing. z and unrelated output bytes retained.

B0CA8 eligibility excludes target CliffCatch/Wait, captureWaitHi/Lw,
ShoulderedWait..Turn, teeter, CaptureKirby..CaptureWaitKirby, Rebirth..Wait,
WarpStarJump..Fall, and ItemParasolFall..DamageFall; caller held heavy four
items; grounded target above a descending airborne caller. Then it accepts
motion flag or the exact squat/landing common motion set. B0E98 additionally
requires both grounded, no recovery_lock, caller not owned UpB361..366,
prepare_destination_update true unless callerWait, relative position-delta
squared<=mid_walk_point squared and distance squared<25 squared. These ranges,
25/5 margins and motion IDs remain named owned parameters, not engine policy.

A7AAC air target ray uses y+10..y-1000; rejected/blocked floor makes no update.
Ground target first rejects blocked actual floor island, probes y+2..y-2,
then requests saved target position on success or current target position on
failure. It does NOT use the hit point as the grounded destination. Same-island
requires both grounded and caller island non-NULL; different-island adjustment
is suppressed for caller common CliffCatch252/CliffWait253. Destination latch
can retain an earlier position, so comparisons read actual cpu.destination
after every set_destination call.

## CPU-coupled Popo capture decision

Reuse existing on_cpu_capture_decision: when attempts==0 and level>4, consume
one RNG only then, and if random<0.05*level run shared script59 and returntrue.
Otherwise returnfalse. Presence already suppresses the raw Popo kind branch;
false then continues common pummel/throw behavior. Separately returnfalse
from on_cpu_pummel_special to preserve ordinary A pummel rather than B.

Common fallback remains exact: grabbing RNG0.5/(1+attempts), selected A/B
pummel and attempts increment; otherwise edge-directed throws or quarter
random up/down/forward/back command sequence; non-grabbing completes command.
Do not reorder RNG or bypass held victim priority. Guard both authored callback
continuations in actual B683C for source identity; when continuation caches a
victim, validate its generation and unchanged link too. Existing hook code
does not provide all those guarantees. Script59's common command table is an
owned common asset dependency to verify, not a native Popo-special delegate.

## Coordinated implementation and validation batch

Ownership changes needed: runtime.h Definition hooks and module fingerprint/
Host slots; ftCo_0A01.c actual source refactors, mode/output hooks and B3900
guards; target.c/h safe scalar/delta getters; CPU compiler/tables/type/lifetime
classification; terrain/vector-output compiler; ftcpuattack.c pummel callback
continuations. No Player/MemberId weakening, constructor context, new history
storage, package schema, motion-private-union API or production follower
template until owned pair metadata is ready.

One coherent console batch should execute both generated routes inside actual
B3900/mode/output bodies, with all unaffected modes and hook-absent defaults.
Compare healthy history wrapping30+ updates, retained bytes/cstick, trigger
0/1 conversion, recording target switch, high-bit IDs and geometry branch
matrix against actual extracted originals. Controlled stage/item planner
boundaries must report ordered calls and RNG draws, not merely matching x54.
Compare generated owned destination helper against A7AAC using real island
chains where possible; primitive source tests separately cover real line
lookup/floor/endpoint bodies and invalid-output stability. Test stale IDs,
NULL registry, sleeping, cursor corruption, nonfinite writes and no partial
publication. Lifetime cases retire/recycle/change kind/data/Definition in
both true/false hooks and nested planner callbacks, including PAGE_NOACCESS
retired caller; no later output or x7C increment. Popo capture cases cover
all gate/RNG branches and linked-victim retirement. Negative-stick undefined
C cases have deterministic authored checks separately from strict original
pairs. No gameplay, rendered follower or complete pair independence claim
can be made from these controlled console tests.
