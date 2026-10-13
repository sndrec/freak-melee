# Bounded early CPU policy implementation and evidence plan

This is the historical pre-SDK71 implementation plan. The explicit handoff
followed SDK70 freeze; current implemented source and generated-module evidence
is recorded in [README.md](README.md).
The source audit is `docs/early_follower_cpu_proposal.md`. Mode6 forwarding,
Popo special entry, family templates and gameplay are excluded from this batch.

## Settled interface shape

| Interface | Native signature | Script contract |
| --- | --- | --- |
| prepare | `int32_t on_cpu_prepare(Fighter_GObj*, uint32_t history_hint)` | `on cpu_prepare`; `event.history_source` RO Int preserves all32 bits. Set `cpu.mode`, finite nonnegative `cpu.follow_distance`, documented strict Bool `cpu.combat_candidates_enabled`, and return a creation ID or0. |
| drop commands | `bool on_cpu_drop_command(Fighter_GObj*)` | `on cpu_drop_command`; true means the remainder was queued by script. The common initial neutral and squat fast route have already run. False continues the original level policy. |
| stick step | `int32_t on_cpu_stick_step(Fighter_GObj*)` | `on cpu_stick_step`; return Int0..127. No Float/Bool result or implicit byte wrapping. |
| stick limit | `int32_t on_cpu_stick_limit(Fighter_GObj*)` | `on cpu_stick_limit`; same0..127 range. Evaluate step before limit. |

The two stick callbacks deliberately avoid a packed integer or Float, private
buffer pointer or script-accessible mutable engine storage. If one is absent,
use that component's original default. A returned out-of-range result leaves
that component's default unchanged; no clamping/narrowing. Source retirement
after either callback aborts without publishing any of the two output values.
Defaults for valid stock CPU levels0..9 remain exactly the old table. Unknown
levels must retain old no-output behavior rather than manufacture a level.

Mode is the actual CPU `xC`, distinct from existing `cpu.behavior` (`x18`). The
live full mode switch has an explicit default path, so an Int mode should not
be rejected or clamped to an invented0..29 domain. Distance rejects nonfinite
or negative values by leaving the defined default. Combat gate maps xF9_b2:
actual B8A9C uses it for candidate generation, and the near-target branch5914
also uses it for A/R retaps; documentation must state both. These properties
are ordinary CPU state, not raw field-offset syntax.

The script-owned follower can use mode6/distance15, initially set the combat
gate true, and return `event.history_source`. Common initialization still
writes that gate false at its original position later. This batch does not
make native mode6 independent: it only owns the early entry decisions.

## Runtime history-hint operation

Recommend a substantive runtime helper invoked directly by A101C, returning
`uint32_t` counterpart ID. No global construction window or extra retained
pointer is necessary. This helper is not a replacement for strict MemberId,
and need not be a separate public DSL query: its result is the explicit event
argument. Exact bounded flow:

1. Snapshot source with actual fighter live-registry/generation check; require
   non-NULL userdata. Read player, kind and ft_data only after this check.
2. Find the active slot by actual kind in the eight-actor active bank; require
   source ft_data equals `&slot->character.data`, active family, valid
   member_index, and selected player character equals that family's CSS kind.
   Also require the actual active Definition is the slot's Definition.
3. For member_count1 return0. For two members select immutable
   `family->members[slot->member_index ^ 1]`, independent of current role order.
   Validate its active slot, same family and Definition.
4. Enumerate only the two **published** `Player_GetEntityAtIndex` objects.
   Snapshot candidate before reading it; require same player, expected kind,
   exact owned ft_data/Definition and not the caller. Return candidate ID.
   Null, stale/unlinked, mismatched or retired counterpart returns0.
5. The source is allowed to be absent from player roles during construction.
   All owned active family/kind/data/selection checks above remain mandatory.
   Do not change MemberId, which continues to require source role membership.

This deliberately supports both an unpublished constructing secondary and an
already-published rebirth/role-swapped actor. It excludes a raw same-player
foreign actor. Alternate versus simultaneous mode does not impose a hidden
follower rule: both can receive a hint and authored prepare chooses whether to
use it. Primary creation normally sees no counterpart and gets0.

The callback result is a retained ID, not a borrowed pointer. At history-copy
time resolve it again using the actual registry. Copy target position/facing
to local scalar values, then write all30 records. An invalid/stale ID uses
ordinary origin/facing1. Do not assume result equals the hint: scripts may
choose another live target. The hint has the strong owned-family relation;
the returned history source has the explicit live-ID contract. If root wants
to restrict return IDs to the counterpart, decide/document that policy before
implementation instead of silently turning a valid alternative into0.

## Exact initialization phases

`ftCo_800A101C`: assign ordinary supplied mode/distance first; authored prepare
replaces the raw early Nana choice. Immediately guard caller generation, kind,
userdata and Definition. The chosen mode then drives the existing common
state switch. Preserve level/x14, floor probe, RNG and all subsequent resets.
Capture callback parameters/local history ID before any later code consumes
them; no callback or allocator inside the30-record copy loop.

At the original reset site xF9_b2 is false even if prepare just wrote true.
Read/write ring cursors remain `&xFC[5]` / `&xFC[0]`. Clear exactly x0 buttons,
lstick.x/y, x4/x5 and set position/facing. **Do not memset the ring:** cstick.x/y
and xA/xB are untouched by original A101C. Fresh construction's earlier whole
Fighter zero-fill makes those zero, but reset/rebirth may retain old values.
Poison these four bytes in tests and preserve them, including in neutral seed.

Keep gravity/jump-height calculation, body defaults, B9704, and existing late
on_cpu_initialize in their original order. A late hook must observe the new
mode-dependent state and final gatefalse/history seed, not simulate prepare.

`ftCo_800A08F0`: common neutral first, then squat fast route, then authored
drop-command hook. True returns; false runs existing level wait policy. Guard
both results and kind/Definition replacement. This path covers platforms and
rebirth, not just one motion. Authored sequence uses current cpu.queue APIs:
wait1, stick_y-127, wait5, neutral stick commands, wait1, done, preserving exact
queue order and termination. No native Nana branch or fallback delegate in
the authored callback.

`ftCo_800AA320`: compute old component defaults where defined, evaluate step
then limit hooks with an actual snapshot before either call, retain local
results, then publish output integers together only after all continuation
checks. Five actual caller sites consume both outputs (5035,5104,5150,5180,
6083); their continuation must also stop if AA320's callback retires caller.
A void helper cannot signal retirement by itself: caller must guard on return
before touching cached fp/CPU or consuming unwritten stick outputs.

## Outer ProcessDeath callsite audit

New early callback retirement makes all existing reset continuations relevant,
not merely A101C. Full actual source inventory:

| Caller | Actual continuation after reset | Required guard placement |
| --- | --- | --- |
| `fighter.c:634`, inside `Fighter_UnkProcessDeath_80068354` | efAsync clear and two further engine calls use fp/gobj. | Immediately after A101C, before636. Compare pre-call generation/kind/Definition, not a fresh post-call identity. |
| `fighter.c:1015`, `Fighter_Create` | Cached fp.kind and spawn/entry decisions, then final ftLib operation. | Immediately after ProcessDeath; preserve the constructor's original generation and own identity. |
| `ft_0D4D.c:108`, `ftCo_800D4FF4` | Immediate fp.smash_attrs write, player-derived spawn calculations, common state/animation/accessory setup. | Immediately after reset, before109; abort all rebirth continuation on retirement/replacement. |
| `ftlib.c:778`, `ftLib_80087140` | Calls ftCommon_8007D92C using the same object. | Snapshot before reset, validate after before entry. Earlier operations also invoke shared transitions and require their existing safety audit. |
| `ftdemo.c:126`, `ftDemo_CreateFighter` | Takes a **new** Snapshot, then on_create/on_demo_enter and final ftLib operation. | A post-reset Snapshot alone accepts same-address/new-generation reuse. Retain the pre-reset generation/kind/Definition and compare before the new creation continuation. |
| `gm/native_practice.c:45`, `reset_escaped_sandbag` | Calls Fall_Enter then reports successful reset. | Guard before Fall_Enter; reporting must not claim a completed reset after source retirement. |

The practice and demo routes are part of safety closure even though no games
or demos are launched in this console batch. Capture corresponding actual
continuation fragments if extracting whole scene constructors would introduce
large unrelated engine fake boundaries. Label fragments precisely; never claim
whole creation parity from a fragment. No synthetic always-true liveness mock.

## Coherent fixture layout and matrix

Suggested namespace files after the implementation ownership handoff:

- `source.py` / `source_test.c`: definition-prefix balanced extraction of full
  stock70 and modified71 A101C/A08F0/AA320, actual runtime hint helper, and
  explicitly named continuation fragments. Real production lifetime and
  ftLib lookup bodies; controlled floor/RNG/B9704/CPU command sinks.
- `compiler.c`: both normal action and motion routes; prepare Int event and
  history hint, drop Bool, stick Int returns; strict setters/context/arity/type
  negatives; require every used Host/lifetime field. Generate actual modules.
- `module_test.c`: invoke real generated callbacks through Definition metadata
  under current own parameter/state layouts, full32-bit IDs and nested helpers;
  no pointer coercion or native Nana policy in authored code.
- `run.py` / `README.md`: one batched console invocation with frozen70 old source
  and new71 inputs explicitly separated, full source/command/dependency hashes,
  ordered boundary census, results and exclusions. Output on A archives.

Healthy source matrix should cover levels0..9; modes0,1,3,6,15,25 and an explicit
default mode; ground/air/floor hit/miss; gravity zero/normal; supplied arg3;
no counterpart/primary/role-swapped counterpart; neutral, high-bit and alternate
returned IDs; squat/SquatWait/SquatRv versus ordinary/rebirth motion; all30
records with retained-byte poison; all stick table rows and64/127 authored
values. Record initialization floor/RNG/late-hook order, true-before-resetfalse
gate state, cursor offsets and exact written/untouched fields. Do not compare
an undefined stock Nana null-partner dereference as healthy behavior: classify
the authored neutral fallback as safety extension.

Safety matrix: absent registry/source/counterpart, missing userdata, unpublished
source with valid owned pair, wrong selected family, retired family/Definition,
source wrong kind/data/slot, counterpart foreign player/kind/data/generation,
same-address reuse, null counterparts and IDs above2^24/highbit/allbits. For
each hook test removal/PAGE_NOACCESS, same-address recycle, kind/Definition
switch, true/false or valid/invalid result; later callbacks/output/common writes
must not run. Tests of strict MemberId retain source-role requirement.

Invalid setters/results preserve defined defaults rather than clamp or narrow.
For drop false, stock fallback is intentional; do not silently treat false as
handled. Exact source changed on invalid/stale partner is separately documented.
No per-frame heap, solver wrapper, format migration or deferred dispatch shim.

## Source inputs for the added caller audit

SHA256 from live sources during SDK70 build (2026-10-03):

| File | SHA256 |
| --- | --- |
| `src/melee/ft/ftdemo.c` | `c85c12ef67b04a6edc39a5f949ebcbe9c52a9f48a738e94974fb13a4ab1e7b51` |
| `src/melee/ft/ftlib.c` | `299ecf23ef06bdb6b8bc674f7fea539cbef45cd4f3bc2858d6197d7e734407d7` |
| `src/melee/ft/ft_0D4D.c` | `c54d04daf3dd820c3874b3c1444c6c359d4c4b8d6594de7665ba545e795a75bc` |
| `src/melee/gm/native_practice.c` | `f32bf98e30867ce93d9a4e204e43e8eba5c4a581d2d49670032fa1881c1ec477` |

This checkpoint writes only the plan and the precise retained-history wording
correction in the existing proposal, as root requested. No executable source,
SDK, template, build or game change is included.
