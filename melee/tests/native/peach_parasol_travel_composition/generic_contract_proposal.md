# Generic held-article and motion-entry contracts (proposal)

Source-first proposal only. No compiler/backend/default changes, generated
module acceptance or gameplay claim. Root chooses the contracts before the
new Parasol composition is activated. Frozen50 source inventory is retained;
`proposal_sources.json` pins the live sources inspected for this proposal.
Float/SideB acceptance a40650720 and all earlier namespaces are immutable.

## Recommended boundary

Retain the engine-owned common open/fall callbacks through explicit
`motion_policy` declarations for owned rows369/370. Those callbacks implement
ordinary shared gravity, drift, input and landing. Replace their three
pre-publication kind selectors with a generic Definition query. Scripts own
which authored motion to select, held status/request policy, cleanup, resume
selection and timer-expiry decisions. No stock Peach callback is invoked.

Full script open/fall entry is possible, but current/previous motion + blend
alone is insufficient: it also needs the common open flags, timer initialization,
open/close input thresholds and ordered input methods. Binding the existing
common callbacks and preserving their real entry setup is the smaller faithful
contract. This proposal does not expose the common union or duplicate physics.

## 1. Fixed authored held-item protocol

Add to `NativeFighterArticleLogic` (not an ItemLogicTable stock-kind switch):

```c
int32_t (*on_held_status)(Item_GObj* item);
bool (*on_held_request)(Item_GObj* item, int32_t action,
                        float frame_rate, float duration);
```

Article DSL (proposed new fixed handlers, not export-name lookups):

```text
on held_status:
    # Return script-defined numeric status, or -1 for unsupported.
    return attrs.closed_status

on held_request:
    # event.action: Int; event.frame_rate/event.duration: Float.
    # Compute rate then execute owned state/sound/accessory policy.
    return true
```

Both are actual generated item callbacks. Status default=-1; request default=false.
At `ftGetParasolStatus`, resolve the fighter's ACTUAL held item to its immutable
registered author and exact authored article logic first. If a declared handler
exists, its result is authoritative; do not fall through to a stock-kind branch
when it returns -1/false. Ordinary engine Parasol keeps its genuine engine table.
Raw stock Peach continues its stock behavior until explicitly authored.

At `ftCommon_8007E83C`, retain the preceding status query/assert order. For an
authored held item, pass `action`, `fp->frame_speed_mul`, and the original `div`
unchanged to on_held_request. The callback owns animation-length selection and
rate arithmetic, preserving `frame_rate * (length / duration)` rather than
reassociating it. Duration0 uses frame_rate directly. No new duration lookup is
needed: retail Peach lengths15/16 are named owned tuning (it_802BDA40's literal
animation-length table), not DAT fields or a fake generic animation query.

The script declares ordinary Int policy IDs: requested4 maps state1/length15,
requested6 maps state2/length16. Status state1=>4, state0/2=>6, other=>-1.
Request4 order is sound247/127/64, animation_rate write, state1/ITEM_ANIM_UPDATE,
then empty accessory binding. Request6 is animation_rate, state2/same flag,
then accessory. Unsupported IDs0..3/5 returnfalse: stock indexes-1/callsNULL,
so those cases are separate safety behavior, never healthy parity pairs.
No public Peach/Parasol-specific enum or handler is introduced.

All source caller query order must survive, including the two short-circuit
status queries in Fighter_ChangeMotionState and ft_80087988. Status is used by
DamageFall entry, ItemThrow, auto-open and command opcode ftAction_80072894 as
well. A request Boolean means handled/accepted, not a replacement numeric status.

Lifetime: snapshot fighter and actual item generations plus author/logic before
callback; validate afterwards before cached continuation. If the held slot changes,
stop that original request sequence instead of applying it to the replacement.
Every enclosing caller retaining Fighter/Item must stop if source identity dies;
returning a sentinel from a helper alone is insufficient to protect cached tails.
No hot-path allocation. Existing dispatch-depth bound applies to nested calls.

Optional fighter primitive if scripts need explicit requests:

```c
bool (*request_held_action)(Fighter_GObj*, int32_t action, float duration);
```

DSL `items.request_held_action(action, duration)` Bool expression. It dispatches
against actual held item and preserves the same rate rule/order as E83C. This is
optional for this translation: startup enters common open and its animation
commands already issue requests at the actual shared opcode site.

## 2. Current holder's actual held instance observations

Extend the existing target registry backend, as root suggested:

```c
int32_t (*target_held_id)(uint32_t target_id);
int32_t (*target_held_kind)(uint32_t target_id);
int32_t (*target_held_motion)(uint32_t target_id);
float   (*target_held_frame)(uint32_t target_id);
```

DSL query expressions `targets.held_id(target)`, `targets.held_kind(target)`,
`targets.held_motion(target)`, `targets.held_frame(target)`; item-context RO
shorthands `holder.held_id`, `.held_kind`, `.held_motion`, `.held_frame` use
`item.current_owner`. ID retains all32bits as Int, absenceID-1. Kind/motion absent
or stale=-1; frame absent/stale=0. Resolve fighter creation ID then held pointer
through live item registry BEFORE dereference; no creator restriction, attachment
assumption, saved handle substitution or pointer escape. Frame reads actual
x5CC, state actual msid, not authored local variables or animation rate.

This closes `notUsingParasol`: at LandingFallSpecial, test actual current
holder's held motion2 and frame>=16. The inspected item can differ from the
callback article and from the recipient's saved Parasol handle. A null/stale
held item is an explicit safe missing query; original it_802BDC38(NULL) is not a
healthy source pair. All other healthy range tests use existing holder.motion.
Current-owner typed restore_held dispatch and its Boolean result remain unchanged.

## 3. Generic motion selection before publication

Add to `NativeFighterDefinition`:

```c
int32_t (*on_held_motion)(Fighter_GObj*, int32_t common_motion,
                          int32_t previous_motion);
```

DSL `on held_motion:` with `event.motion` and `event.previous_motion` Int.
Default generated return is event.motion. Script maps common open/fall-special
IDs to owned369/370 only when its ordinary `state.parasol_handle` is not -1;
otherwise returns event.motion. Original predicate tests NONNULL saved pointer,
not held kind. The normal handle's not--1 test reproduces that healthy source
policy; do not silently strengthen it to creator identity or current held slot.
Stale handle rejection can be a separately documented safety case.

Exact sites: replace the selection in ftCo_ItemParasolGetFallMotionId used by
ft_800CEF08, ftCo_800CEFE0 and ftCo_800CF3C8. For open, previous_motion is the
value saved BEFORE selection into common prev_msid; for fall, pass current
motion_id. Call BEFORE Fighter_ChangeMotionState and preserve flags/anim/rate/
blend arguments. Validate returned ID against current package/common table;
invalid return safely retains the supplied common motion. After callback,
source generation/kind/Definition must still match before transition.

Declare owned rows369/370 with existing motion_policy to the respective
engine-owned common open/fall-special rows. Preserve their package animation,
flags and camera. The shared previous-motion union remains internally owned by
these engine callbacks; scripts need no public alias for it. The query carries
previous_motion explicitly, so authored state can observe it if useful.

## 4. Startup publication and real common open entry

Current `motion.id` is RO. Add Int assignment to the existing property,
validating a real common/package motion ID. This is RAW identity publication:
no callback swap, animation reset, metadata transition or union initialization.
Document use at deliberate entry boundaries only. Startup's original assignment
FallSpecial immediately precedes common open entry; an extra state transition
would change behavior.

Consolidate the two real duplicated open entry bodies, rather than add a wrapper:
change actual ft_800CEF08 to `(Fighter_GObj*, float blend)` and update auto-open
caller to0 and startup callers to10; remove CEFE0 and its ignored Int argument.
Bind that real engine function as:

```c
void (*enter_held_open)(Fighter_GObj*, float blend);
```

DSL `state.held_open(blend)` invokes shared setup, not a fighter-specific handler.
It saves current motion, calls generic held_motion selection, changes motion with
flags0/start0/rate1/blend, starts animation, clears automatic-open flag, sets
active timer flag, and if reset requested writes common.x59C then clears reset.
The ignored PeachDA.x90 must NOT acquire behavior. Source test compares original
CEFE0(blend10) to this consolidated actual body.

No new blend form of state.change is REQUIRED by the recommended path. If root
chooses full authored open instead, extend existing state.change with optional
fifth Float blend directly to change_motion_state (existing4args default0),
plus expose the common entry flags/timer mechanics; that is a larger alternative.

## 5. Neutral pre-entry notification and landing context

Definition notification:

```c
void (*on_neutral_enter)(Fighter_GObj*, float blend);
```

DSL `on neutral_enter:`; event.blend Float. At ft_8008A348 AFTER down/hammer early
returns and air-to-ground, BEFORE Wait ChangeMotionState at the former Peach
cleanup branch. Script uses actual held_kind; when its declared article matches,
it sends `articles.signal(items.held_id, "remove_cleanup", 0)` to the actual held item. That signal restores
saved item through typed current-owner helper. Do not substitute saved article
handle when a different article occupies the held slot. Guard source generation,
kind and Definition before Wait/camera/timer continuation. Existing wait_enter
keeps its later notification and is not moved or duplicated.

Add Bool RO `landing.interrupt_allowed` as stored common landing.allow_interrupt.
Valid only in on_landing_action or active common Landing/landing-fall-special
entry/phase context; other contexts reject or returnfalse under runtime state
validation. It must read the stored value after ChangeMotionState, not incoming
stack argument or an authored copy. Existing on_landing_action clears owned
Toad-use then cleans Parasol iff true, preserving that source order. No union
field name is exported. Hook remains at its existing publication site.

## 6. Mushroom air-resume policy

Definition query:

```c
int32_t (*on_air_resume_motion)(Fighter_GObj*, int32_t previous_motion);
```

DSL `on air_resume_motion:`; event.previous_motion Int; default return unchanged.
At fn_800D2890 BEFORE shared canonical switch, replace only Peach370 kind branch.
Script maps own370 to common FallSpecial, otherwise returns prior motion.
Shared switch still binds actual FallAerial/FallSpecial/Fall engine entry
pointers. This is a policy mapping, not a public function-pointer API or an
extra state change. Stop on invalidated source; reject invalid mapping by
using original input. No generic resume mechanics are copied into script.

## 7. Held timer expiry without stock article-kind decisions

Definition decision:

```c
bool (*on_held_timer_expired)(Fighter_GObj*);
```

DSL `on held_timer_expired:`; defaultfalse means continue normal engine fallback.
At Fighter_8006A360, AFTER actual decrement reaches0 and active flag clears,
BEFORE stock held-kind dispatch. Independent script checks actual held_kind.
For its declared article: set generic Bool `items.reopen_blocked = true` then
existing `state.freefall(1, 0, true, 1, movement.special_landing_lag)` and returntrue.
Those exact arguments reproduce real ftCo_800968C8, including current x2EC,
without a new convenience wrapper/entry Host. For other held kinds returnfalse
and retain ordinary ItemThrow path. Guard source after callback before fallback
or subsequent timer/camera work. Existing raw-stock branch can remain only when
there is no authored policy; authored false must not re-enable donor Peach logic.

`items.reopen_blocked` is generic Bool RW to the actual common x2221_b5, used
by auto-open's first gate. It is a shared held-action permission, not script
storage or Peach union. Other timer/open flags remain private common mechanics.

## Acceptance matrix and unresolved choice

Proposed healthy comparisons extract the actual common status/request, three
entries, Wait/Landing/resume/timer caller bodies, original article callbacks and
high-special bodies; compile the real assembled script under matching next SDK.
Cover div0/nonzero arithmetic, status call order, requests4/6, transition
flags/blends/prev motion, timer reset/expiry, Landing stored allow flag, held
ordinary item, actual article different from saved handle, changed/current/null
owners and article state/frame16 edges. Shared physics/IASA/collision callbacks
remain actual common C, with direct policy bindings proved rather than simulated.

Separate safety matrices cover missing/recycled source/held item, changed author,
invalid returned motion/request, absent exported recipient and invalid held rig.
No digest masking; preserve accepted constructor/debug/cleanup boundaries.
Root must choose the callback names/entry consolidation. Existing named signal
works for SAME Definition aliases, including changed current holders, but
native_fighter_module.c:signal_article explicitly rejects a held article authored
by a DIFFERENT Definition. Do not claim arbitrary foreign-package cleanup from
that path. If that case is within next acceptance, add the fixed item notification
below; it dispatches the actual held item's own immutable policy and does not
weaken the caller-scoped signal contract:

```c
// NativeFighterArticleLogic: current holder is about to enter neutral.
void (*on_holder_neutral)(Item_GObj*);
// Host: dispatch actual live held item, returns dispatch/lifetime success.
bool (*notify_held_neutral)(Fighter_GObj*);
```

Article `on holder_neutral:` executes its own cleanup/remove policy. Fighter
`items.notify_held_neutral()` is a Bool expression used inside on_neutral_enter,
after the script's owned held-kind check. The article's typed current-recipient
restore helper determines whether owner clears, exactly as existing cleanup.
Resolve actual item author+logic without requiring caller Definition equality;
validate item/source before and after dispatch. Callback may remove the item;
Boolean false on removal is a continuation safety result, not a reason to redo
cleanup. This is one generic held notification, not a stock handler/foreign named
export lookup or a creator restriction. Raw engine item fallback, if needed,
remains its genuine engine lifecycle behavior, never an inferred scripted body.
Root has now selected/started section2 with final names `target_held_motion` /
`holder.held_motion` and missing ID/kind/motion=-1, frame=0; this proposal matches
that contract. No implementation, tests or independence acceptance is claimed
by this namespace.
