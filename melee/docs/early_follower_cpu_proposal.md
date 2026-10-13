# Early authored follower CPU policy

This records the historical SDK69 design and its source census. The bounded
early-policy interfaces are now implemented for SDK71; current console scope,
commands and receipts are in [cpu_early_policy](../tests/native/cpu_early_policy/README.md).
Mode6 history forwarding and partner-specific move decisions remain outside this batch.

## Confirmed source order and gaps

| Actual source | Decision and position in the common pipeline | Existing authored coverage |
| --- | --- | --- |
| `ftCommon/ftCo_0A01.c:660`, `ftCo_800A101C` | Before the mode switch, Nana sets `cpu.xF9_b2=true`, mode `xC=6`, distance `x3C=15`; ordinary actors use supplied mode and distance40. Level and `x14` then initialize. | `on_cpu_initialize` is only at830, after the mode switch, reset, history seeding, jump-height calculation and `ftCo_800B9704`. It cannot repair the early mode-dependent state consistently. |
| same function:767 | Common reset unconditionally sets `xF9_b2=false`, including Nana. | Preserve the earlier true write **then** this false write. Do not move an authored prepare callback after the reset and silently change this order. |
| same function:785-814 | Read/write history cursors start at records5/0. Every record's buttons/lstick/x4/x5 are cleared; Nana copies another same-player fighter's position/facing into every record, ordinary actors seed position0/facing1. Cstick and xA/xB are not written by this loop. | Late initialization cannot retroactively make the original initialization branch authored. |
| `ftCo_800A08F0`:456 | Clear/queue neutral stick first. Squat/SquatWait/SquatRv queue wait1/done and return. Otherwise Nana queues wait1, down-127, wait5, neutral, wait1, done; others use level-dependent first wait45/30/15/10/5/1. | No dedicated authored policy replaces this branch. It is used for rebirth-with-no-enemy **and** platform/drop travel (`ftCo_800AA704`, `ftCo_800AB224`, `ftCo_800AB58C` callers around5112/5184/5486). Calling it only a rebirth policy would omit reachable uses. |
| `ftCo_800AA320`:4906 | Nana returns stick step64/clamp127; ordinary level table gives step1/2/4/8 and clamps67..127. Five travel callsites consume both integer outputs. | No existing scalar CPU policy changes these two outputs. |
| `ftCo_800A0CB0`:532-563 | Nana spacing random factor1 versus ordinary4. | Existing `on_cpu_spacing_update` precedes the stock switch. |
| `ftCo_800A101C`:830-853 | Nana initial spacing1 versus ordinary3.5. | Existing `on_cpu_initialize` precedes this final switch. |
| `ftcpuattack.c:1125`, `ftCo_800B683C` | Popo pummel special candidate0x3B when grab attempts0, level>4 and RNG<0.05*level. | Existing capture/pummel policy hooks intercept this decision earlier; port the authored policy, preserving RNG order. This is Popo, not another raw Nana branch. |

The exact raw Nana kind inventory in these CPU units is six sites: A08F0,
A0CB0, A101C early mode, A101C history seed, A101C late spacing, AA320, plus
the Nana explanatory comment at4908 is not another decision. In executable
code that is **six** `Ft_Kind_Nana` references (466,556,682,787,843,4910).
The same units contain one actual `Ft_Kind_Popo` comparison (ftcpuattack1125).
Named helper/motion references below are additional dependencies; counting only
kind comparisons is insufficient for independent follower behavior.

`x3C` is consumed as distance/tolerance in common travel (5055/5080). The
similarly named `var_r29->x3C` angle threshold at1015 belongs to a different
DAT structure, not CpuFighter. Expose the CPU field as a distance with a finite
nonnegative Float contract, not a generic untyped offset.

## Creation and publication timing

`Player_80031AD0` (`pl/player.c:243-296`) assigns the result of
`Fighter_Create` into `player_entity[0]`, then creates/assigns member1. Inside
`Fighter_Create` (`ft/fighter.c:937-1033`) the GObj, user data and spawn identity
are established, owned data/parts/animations load, on_load runs, procs install,
then `Fighter_UnkProcessDeath_80068354` runs before the constructor returns.
That function calls A101C at634. Thus:

- At primary preparation neither newly creating actor has a published player
  role. A healthy primary must tolerate no secondary identity.
- At secondary preparation the primary is published and live; the constructing
  secondary itself is still absent from both player roles.
- Rebirth/death reset reuses A101C for an already published actor. Role ordering
  can have swapped for alternate forms and cannot identify immutable members.

`nativeFighterRuntime_MemberId` (`native_fighter_runtime.c:401-431`) deliberately
requires the caller itself to be one of the player's role objects, then checks
active family/kind/owned ft_data and resolves the requested immutable member by
matching either current role. Consequently `family.member_id(0)` returns0 in a
secondary's constructor, even though its primary exists. This is a real timing
constraint, not a reason to remove the existing role/lifetime checks.

## Minimal early policy batch

Proposed Definition callback is an Int-returning `on_cpu_prepare`, before the
mode switch and all history seeding. It replaces the entire raw early Nana
choice when present; callback absence retains stock behavior. Initialize the
ordinary supplied mode/distance first so script reads have defined defaults.
Typed script writes are `cpu.mode` (Int), `cpu.follow_distance` (finite
nonnegative Float), and an accurately documented generic combat gate Bool.
The gate corresponds to xF9_b2: it enables candidate generation in B8A9C and
the near-target A-retap/R-retap choice around5914. Suggested name
`cpu.combat_candidates_enabled` must document both consumers, rather than
claiming it controls all combat. The authored follower writes true in prepare;
the existing common reset still writes false later.

Return value is the full-width creation identity to seed the30 history records;
0 requests the ordinary neutral origin/facing1. At the later seeding site resolve
the retained returned ID again through the live lookup, reject absent/reused
identity and invalid userdata, and copy position/facing to local values before
the30-record loop. No float conversion, stored Fighter pointer, private union
offset or current-kind substitution. A stale/invalid result falls back to the
ordinary neutral seed. Invalidity must not restore a retired source. Every
continuing callback path checks caller generation, kind and Definition before
using cached CPU/Fighter pointers.

There are two mandatory outer continuation sites as well: after A101C returns,
Fighter_UnkProcessDeath immediately clears async effects and invokes additional
engine operations (fighter.c:636-639); after that reset returns during creation,
Fighter_Create reads cached fp and selects entry state (1016 onward). Guard both
before continuing. Guarding only inside A101C cannot protect these callers.

The return-ID interface needs an explicit solution for the creation timing
above. Preferred option is **an explicit validated suggested history ID passed
as an event argument** from the shared creation/runtime policy. For example,
`on_cpu_prepare(Fighter_GObj*, uint32_t suggested_source) -> int32_t`, with
`event.history_source` RO Int in script. Shared creation selects a published
live counterpart by immutable owned family member identity; a lone ordinary
fighter gets0. The script decides whether to use the suggestion, return0, or
return another already validated target. Rebirth can supply the same policy
without depending on mutable role index. This avoids a persistent global
construction context and leaves MemberId's existing contract unchanged.

The substantive implementation must explicitly carry this value through the
actual creation/reset path; it cannot fake it by calling strict MemberId early.
If adding an event argument changes the root's proposed zero-argument signature,
settle that before writing the Definition/compiler. Shared policy must validate
actual active family slot, caller kind/owned data, player selection and the
published counterpart's generation/kind/data. It must not infer same package
membership from a raw same-player pointer or expose arbitrary registry matches.

Alternative: allow construction-member resolution in a **narrow, explicit
construction lifetime window** after owned data binds and before returning from
Fighter_Create. This needs an owned actor/family identity, source-generation
validation and guaranteed closure on every early-return path; on_load's present
load window ends too soon. It adds mutable state and reentrancy/rollback review.
Never broadly relax MemberId's role requirement or accept a registry-only
lookalike. This option is more complex than the explicit event input and is not
recommended for the bounded first batch.

For A08F0, add `on_cpu_drop_command -> Bool` **after** common neutral setup and
the existing squat fast route, but before the Nana/level branch. True means the
script queued the remainder; false uses common default. Generated follower
queues the exact wait/down/neutral/wait/done sequence using existing cpu.queue.
Guard retirement for either result before a default continuation. The name
covers both platform and rebirth calls.

For AA320, a pair of scalar Int callbacks `on_cpu_stick_step` and
`on_cpu_stick_limit` avoids packed bit returns and pointer outputs in script.
Each reads ordinary attrs/state and returns64/127 for a follower; both may be
dynamic. Evaluate them in a documented order, validate/recheck the caller after
each, and publish neither output if source is retired. Validate step/limit0..127
without silent clamping; absent callbacks retain the existing native table.
These are policy hooks, not a native follower solver. A single typed two-output
callback is another valid contract if root prefers its event-output machinery;
do not encode two integers in a Float or invent private callback pointer fields.

## History forwarding and remaining mode6 ownership

Early prepare owns **initial seeding**, not future controller sampling. Mode6
at `ftCo_800B2AFC:8432` calls B101C. B101C:7748-7802 gets another same-player
fighter via A589C, appends live input history using B0918, derives level from
the other's damage, and chooses follow/combat modes. B0918:7510 increments
both30-record ring cursors, preserving their five-record relative offset,
then copies both sticks, both triggers, buttons, position and facing into the
write record. Do not change this to an invented30-frame delay or float IDs.

`ftCo_800B0AF4:7550` consumes the read record after B3E04 in B3900:8791.
It also runs `inlineJ0` (partner down-special, caller common motion, excluding
FireFlower), flips facing and directly enters `ftPp_SpecialLw_Enter`. When
x2225_b3 is set it smooths caller position toward recorded position at0.95/0.05
and sometimes copies facing. `isPopoUpB` and B101C_inline0 refer to the Popo
up-special motion range. These decisions require authored policies/motion IDs;
mode6 is **not** universally fighter-independent just because it has a number.

A later cohesive follower batch should replace mode-specific choices with an
authored mode callback and expose generic, bounded history operations:
`cpu.record_input(target_id)` validates the target and appends one exact native
record; typed delayed-history reads or a substantive `cpu.apply_recorded_input`
operation copies only CPU controls, with authored movement/facing decisions
separate. The chosen recording target is script-owned Int state and resolved
every use. Invalid target policy must be explicit (skip record/retain cursor,
or append neutral); it cannot blindly reuse a stale pointer. No getter may
return mutable CpuFighter/ring storage. Delay and capacity remain the actual
30-record/cursor setup for this first source port, not guessed/clamped custom
history. B7AAC's partner destination/recovery decisions likewise need an
explicit target input or authored generic stage queries. These are follow-up
contracts, not additions approved by this document.

Existing generic `cpu.nearest_enemy_at` can select enemies; it does not select
an immutable partner nor redirect controller history by itself. Existing
on_cpu_initialize/spacing/capture/pummel/recovery hooks should be reused for
their already correctly placed portions. Do not add a Nana handler delegate,
behavior_kind disguise or universally donor-backed mode6 path to a clone.

## Validation and mutation boundaries

First batch should extract full A101C/A08F0/AA320 bodies with common command,
floor/RNG and B9704 boundaries controlled and ordered. Compare healthy original
Nana/ordinary cases with actual generated callbacks for levels0..9, supplied
modes1/3/15/25/6, squat/non-squat, histories neutral/copied and stick outputs.
Assert true-then-false gate order, unchanged common RNG/floor ordering, all30
records (including exactly cleared and retained fields), cursor offsets and the **later** existing
initialize hook. Include 0/high-bit/stale/reused IDs, primary-before-secondary,
secondary-before-publication, rebirth after role swap, foreign family, removed
or replaced source/counterpart and PAGE_NOACCESS boundaries. Pure hooks still
need source continuation checks if a Host extension retires either endpoint.

Expected shared edits, once this design is accepted: runtime Definition/Host
headers and module validation; CPU type/property/handler tables and exact-site
lifetime classification; direct A101C/A08F0/AA320 callsites; actual creation/
reset identity transport if choosing event input; focused original/generated
fixtures and CMake. Player family publication remains all-or-nothing. Full
mode6 B101C/B0AF4/B7AAC ownership is a distinct source matrix. No game/render/
whole-follower behavioral acceptance is claimed by this proposal.

## Historical SDK69 source receipt

Read-only SHA256 inputs (2026-10-03; live SDK69 integration):

| File | SHA256 |
| --- | --- |
| `src/melee/ft/kinds/ftCommon/ftCo_0A01.c` | `78b58c4504a682128b84b8c6a2372648b1aea4a5215dcec2f7fbc95a4d431ea8` |
| `src/melee/ft/ftcpuattack.c` | `c270df90c23c49b8ad7d40f8f217e44d458d221b19fd2741fc62300fe6d64396` |
| `src/melee/ft/fighter.c` | `cb32a353f22696973a7bd91852cba56cfe40be2b694a7d5851607ca1532c628d` |
| `src/melee/pl/player.c` | `e244aadcc02a8fcf32abc11ea75a29204f8173f5addf38b9e09797e90af6254b` |
| `src/melee/ft/native_fighter_runtime.c` | `6bf3d2d2c97aa259f9eb8251220480e52388f5cc9695fddc5dbe67527988913b` |
| `tools/native_fighter_script_cpu.c` | `c33be14ebc4c6a85776f64efc468428eadee3075b3c0d519e40be2bbf2061f86` |

At that historical design checkpoint only this proposal was written. Subsequent
SDK71 changes and console checks are recorded in the linked fixture README;
immutable SDK70 inputs and previously accepted templates remain unchanged.
