# Minimal generic persistent-form contract (pending approval/implementation)

## Original mechanism and exact callers

`Player_80031AD0` creates `player_entity[0]` from mapping.internal_id, then
`player_entity[1]` when extra_internal_id is valid; has_transformation controls
dormant startup. `Fighter_Create` calls common `ftCo_800BFD04` for that dormant
entity. This sets Sleep, visibility/collision/input/camera flags. Native
`Player_RegisterNativeCharacter` currently always writes `{fighter,-1,false}`.
`StageCached` only stages selected package IDs; it cannot load an unselected
partner definition. These are real blockers even when every move compiles.

Sheik `fn_8011412C` clears accessory4 and calls `ftCommon_8007EFC8` with stock
Zelda `ftZd_SpecialLw_8013B4D8`. Zelda `ftZd_SpecialLw_8013AEAC` symmetrically
passes stock Sheik `ftSk_SpecialLw_80114758`. There is **no raw-kind branch inside
EFC8 itself**; the raw coupling is the player mapping and caller-supplied finish
pointer. Both private finish pointers must be replaced by recipient-authored
decisions, not forwarded from a new wrapper.

## Proposed smallest engine/Definition surface

1. Immutable owned companion metadata, validated atomically before launch.
   Resolve two definitions/assets and install `{primary,partner,true}` with both
   native slots registered. No fallback to a stock partner if one is absent.
   A self-contained two-form package is the closest retail representation; an
   explicit immutable package dependency is another implementation choice for
   root. Either must account for unique definition slots, costume correspondence,
   shared player life/stock/score and initial form. Runtime capacity is currently
   a count of definitions, not players; do not assume four partner slots fit.
2. Directly rename/generalize the **actual** common transfer implementation to
   a substantive `bool ftCommon_ActivatePartner(Fighter_GObj*)` operation. Resolve
   the existing player partner, validate both live full-width identities and
   their expected definitions, transfer common state in original order, then
   invoke destination Definition `HSD_GObjEvent on_form_activated` directly.
   Host: `bool (*activate_partner)(Fighter_GObj*)`; DSL
   `forms.activate_partner() -> bool`. No wrapper retaining EFC8 or stock finish.
   A `forms.partner_id -> Int` query is unnecessary for the original move if the
   transfer dispatches the destination hook internally; add it only for a proven
   author use. Missing partner/hook returns false before mutating player roles.
3. Generic CPU partner-switch decision at the **two existing** post-gate sites
   `ftCo_0A01.c`5393 (`ftCo_800ABA34`) and5873 (`ftCo_800ACD5C`). An exclusive owned hook can choose the shared
   command38 while common code owns pending-flag consumption. Example
   `on cpu_partner_switch_request` with no fighter union exposure. Absent hooks
   retain exact stock behavior. A paired-definition capability can replace the
   raw-kind eligibility check; it must not consume commands for unpaired custom
   fighters. Preserve the first site's continuing `else if` behavior and second
   site's immediate return. Common pending generation remains unchanged: mode14
   checks every120frames, other modes every400, with existing RNG>0.5 order in
   actual `ftCo_800ADE48`.

These are proposals, not existing DSL. `on_form_activated` is specifically a
destination-owned policy boundary; it does not invoke stock Sheik/Zelda logic.

## Preserve actual common transfer order

Current EFC8 first swaps player transformed indexes and each entity's x221F_b4;
then `Fighter_UnkInitReset_80067C98` and `_func_8007E2FC_inline` reset destination
common state. It copies current/previous position and delta, facing, percent/HP,
damage state, self/ground/knockback/nudge velocities, air/ground status, input and
all shared input timers; then damage attribution/flags, counters, jumps/walljump,
shield, bury and shared flags. Invincibility/star presentation, shared scale,
metal/cloak, worn-item transfer and camera follow in their original order. It
then transfers held-item presentation and attachment, refreshes the shared view,
sleeps source through `ftCo_800BFD04`, and calls the destination finish.

Destination reset advances its spawn generation, but does not clear fighter
private state or native_module_state and does not call OnDeath/OnLoad. The generic
transfer must preserve ordinary authored per-form variables, especially dormant
Sheik needle charge. Revalidate/reset the expected destination identity at this
intentional reset boundary rather than treating the known new generation as a
removed source. Snapshot/revalidate both sides after every callback-bearing item,
presentation or state operation. No stale source pointer may be read after sleep
or destination finish; failed validation must stop later cached access.

The actual source includes `(src->x198C = 2)` inside its invulnerability test.
Keep that assignment in the reference and explicitly decide its preservation
or repair during the source comparison; do not silently replace it with equality.
Held items transfer to the destination's actual binding part and are not respawned
or picked up twice. Article author identity remains the original creator, even
after held/current owner transfer; paired forms must not weaken same-author state
access rules globally. A different recipient can expose a named public export.

## Entire Transform policy remains script-owned

Sheik ground361/air363 entry changes motion, starts animation, clears command0,
divides x/ground velocity by attribute0x60 and y by0x64, obtains raw TopN world
position and calls actual position-wind120/.4/.003/pi/3, then installs start
accessory. Start accessory conditionally spawns1276 on actual raw43 (semantic
R1stNa under the owned map), latches effect state, sets hitlag pause/resume and
clears accessory. Animation completion installs the transfer accessory; it clears
itself before the generic partner operation. Gravity/terminal0x68/0x6C and common
air deceleration remain authored inputs.

Ground/air conversion retains actual source branch tests, current frame and
`ftCommon_GroundAirColl_MF|KeepGfx|KeepColAnimHitStatus|SkipHit`. Finish362/364
uses ordinary Wait/Fall and the same ground/air physics. Conversions install
finish accessory; that conditionally spawns1277 on raw HipN4 and clears itself.
Recipient Sheik finish chooses362or364 from transferred air status and starts at
attribute0x70 before installing finish accessory. Recipient Zelda finish similarly
chooses356or358 and attribute0x80. Timing/particles/velocity/phase choices are
script policy, not generic form-switch flags or stock callbacks.

## Required source proof before activation

Compare actual unchanged EFC8 plus actual reset/sleep callers against generalized
code in both directions, ground/air, charge state, HP attribution, shared input,
worn/held items, star/metal/cloak, transformed role bits and source/destination
generation changes. Add missing partner, removed/poisoned/recycled partner and
removal from item/destination hooks. Compare both exact CPU sites with ordered
gates/RNG/command38/pending consumption. Then compare authored Transform entries,
all16 phases and both destination finish hooks with original stock bodies.

Normal project cloning/import, complete independent callback census and retained
article ownership must then run with real registered pair metadata. This proposal
does not accept any package/renderer/gameplay path merely because source compiles.
