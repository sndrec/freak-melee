# Link and Young Link activation proposal

Read-only closure audit against checkout `4cd7dc5c8924bcf476dca52714e653419fec0fa0`,
with the frozen40 cleanup evidence supplied for independent root reproduction.
This proposal does not activate a fighter. No accepted fixture, template, SDK,
build output or existing document was changed for this audit.

## Current boundary

`examples/link_clink_wip.fms` owns Spin Attack356/357, Bomb Pull358/359 and the
two seven-state Bomb articles. It does not declare independent behavior or
the remaining fighter events. The copied bases in
`src/melee/ft/kinds/ftLink/ftlink_clone.c` and
`src/melee/ft/kinds/ftCLink/ftclink_clone.c` retain stock source kinds, special
entry pointers and motion tables. Some common paths dispatch through the donor
behavior kind; others compare the custom raw kind and skip Link behavior.
Neither outcome establishes complete stock behavior or independence.

The remaining work is mainly composition and common-entry integration. The
Hookshot resource architecture, nine-state authored solver, immutable-author
article references, recipient-owned exports and lifetime guards already exist.
Do not reopen those designs or add a native Hookshot solver.

## Exact component coverage

The original tables are `ftLk_Init_MotionStateTable` in `ftLink/ftlink.c` and
`ftCl_Init_MotionStateTable` in `ftCLink/ftclink.c`:21 rows,341..361.

| Rows or subsystem | Existing evidence | Activation work still required |
| --- | --- | --- |
| 341 second forward smash | Original common AttackS42 bodies; no accepted authored Link common-combat fixture | Own the command0+A continuation and row341 callbacks; avoid the raw-kind assertion in `ftattacks4combo.c` |
| 342/343 | Empty Link rows; Young Link Appeal callbacks in `ftclinkappeals.c` | Author Young Link taunt entry, command1 creation/removal and completion; Link retains ordinary common taunt |
| 344..349 Bow fighter phases | Readable unregistered Bow draft; article source evidence below | Compare all six fighter animation/input/physics/collision callbacks and exact paired constructors after full cleanup wiring |
| 350..355 Boomerang | Preserved `tests/native/link_boomerang_fixture.fms` and source harness:15,452 comparisons, constructors and six fighter phases | Promote through fresh owned import and the assembled package; replace its historical partial cleanup helper with the accepted composition |
| 356/357 Spin | Shipped authored template, direct source harness and16 owned runtime cases | Preserve tuning, effects and landing flags in the complete package; rerun package regression |
| 358/359 Bomb | Shipped authored template, complete seven-state articles and16 runtime cases | Preserve article/holding-joint import defaults and ordinary item lifecycle in the complete package |
| 360/361 AirCatch/Hit | `tests/native/link_aircatch/`:487,476 assertions /8,730 source phases | Wire accepted input hook and callbacks into the assembled definition; complete real Catch/capture gameplay checks |
| Hookshot constructor/state0/teardown | `tests/native/link_tether_held/` and standalone stock runner:1,710,332 assertions | Reuse accepted sources; constructor/HSD/ECB boundaries remain controlled in source evidence |
| Hookshot states4/5 | `tests/native/link_tether_deployed/`:1,851,554 assertions /2,496 phases | Reuse; no new solver primitive |
| Hookshot states1/2/3/6/7/8 and spool | `tests/native/link_tether_remaining/`:143,698,024 assertions in full reproduction | Reuse; preserve disclosed sqrt substitution, rounding and joint-dirty differences |
| Ground Catch schedules and removal order | `tests/native/link_tether_ground/`:608,690 assertions /12,980 cases | Implement the eight common hooks below and test the actual common continuations |
| Bow/Arrow articles | `tests/native/link_bow_arrow/` plus source runner:9,132 Bow and17,114 Arrow comparisons, including768 terminal recipient configurations | Full assembled fighter integration and runtime checks; article construction/impact/embed/trails must remain authored |
| Young Link Milk article | Preserved Milk source/graph fixtures:69,180 comparisons and actual HSD graph acceptance | Fighter Appeal construction/phase acceptance; do not add Milk to Link |
| Full immutable-author cleanup | Frozen `tests/native/link_article_cleanup/`, worker receipt `.local/link-article-cleanup40-final/acceptance.json`:15,802,352 assertions /18,432 strict source cases /66 safety cases | Root fresh reproduction and coherent package assembly; this is source-phase evidence, not game activation |

The cleanup namespace contains the actual full article callbacks. Its order is
Boomerang, Hookshot, Arrow trails/removal, Bow recipient cleanup/removal, Milk.
Death1 is Hookshot-only, death3 Boomerang-only, and damage/death2 run the full
chain. Global OnDeath then resets groups0/1/2 and named references/used state.
References are not interchangeable with the two engine held/auxiliary slots.

Bow terminal cleanup dispatches the recipient's exported function with the
recipient's Definition, parameter table and state layout. Its export clears
only Bow state, matching `ftLk_SpecialN_UnsetFv14`; it must not call full cleanup.
An absent foreign export skips that recipient mutation but still removes the
terminal article. Transfer/reflection does not replace immutable author context.

## First shared batch: eight common Catch hooks

The current `NativeFighterDefinition` has `on_catch_pull_frame`, capture-wait
hooks and `on_air_tether_input`, but not these eight hooks. The exact prior
contract is `tests/native/link_tether_ground/API.md`; preserve its distinction
between captor CatchWait and the caught fighter's CaptureWait.

| Proposed authored handler | Type | Original insertion/replacement boundary |
| --- | --- | --- |
| `catch_allowed` | Bool | `ftCo_Catch.c` input, after ordinary item pickup; replace the tether veto pair from `ftCo_0D8E.c`, retaining ordinary button checks |
| `catch_start` | Void | `ftCo_800D8C54`, after acceleration XYZ zeroing and before212/214 motion entry; reset ordinary tether counter |
| `catch_frame` | Bool | Exclusive replacement of `fn_800D8EC8` / `fn_800D9228` in standing/dash animation; true consumes, false continues to common animation completion |
| `catch_exit` | Void | `fn_800D8E30`, before common Fall; present chain cleanup, absent-chain defined no-op |
| `catch_wait_release` | Void | `ftCo_CatchWait_IASA`, after successful throw/cut; preserve throw short circuit and absent-chain binding cleanup |
| `catch_pull_anchor` | Bool | `fn_800D9CE8`, after start-frame selection and before throw flags/motion entry; true selects authored capture anchor, false common bone |
| `catch_pull_ready` | Bool | Exclusive CatchPull animation readiness decision, including absent chain and completion; common transition only after true |
| `catch_pull_follow` | Bool | `fn_800D9CE8`, after motion entry and before accessory1 assignment /x221B_b7 clear; false selects NULL, true the actual common follow function |

Relevant source files are `ftCommon/ftCo_Catch.c`, `ftCo_0D8E.c`,
`ftCo_CatchPull.c` and `ftCo_CatchWait.c`. Do not forward these hooks to native
Link/Samus handlers. Resource capture-anchor binding, completion reads, signals,
ordinary counters, callback identities and indexed geometry are sufficient
for authored decisions. `fn_800DA190` must preserve the Link/Young Link/Samus
no-follow choice rather than accidentally running its default for custom kinds.

Every callback boundary needs the existing generation/kind/Definition liveness
checks before cached fighter or victim continuation. A false Bool result can
still have removed its caller. Stock NULL-chain CatchPull entry accesses invalid
state; the authored false/common-anchor branch is a disclosed safety extension.
The existing `on_capture_wait_enter` can author the successful capture's tether
notification from `ftCo_CaptureWait.c:fn_800DB6C8`; no ninth hook is needed for
that existing boundary.

Validate the whole actual common Catch/CatchPull/CatchWait bodies, input item
pickup ordering, standing/dash timings, absent/completed chains, throw/cut
short circuit, exact anchor/accessory binding and caller/victim invalidation.
Reuse the accepted ground and full-resource fixtures instead of another solver
mock. This is the smallest next coherent shared batch.

## Common combat and presentation closure

### Down-air bounce

`ftCommon/ftCo_AttackAir.c:decideFighter` reaches
`ftLink/ftlinkattackair.c:ftLk_AttackAir_Enter` only for raw Link/CLink kinds.
After ordinary selected aerial entry, Link installs `lwOnHit`, `lwOnAnim` and
zeros an ordinary countdown for AttackAirLw. `lwOnHit` clears attacks, sets
vertical bounce, clears fastfall, starts the countdown and conditionally rewinds
the motion. `lwOnAnim` subtracts animation rate, restores three capsules/damage
within the original frame window, then runs ordinary aerial animation.

Existing state, damage callback binding, rate, clip length72, fastfall, clear
attacks, capsule victim clearing, enabled state and damage setters cover most
operations. A post-common selected-aerial-entry notification is still required
unless a source-equivalent existing entry boundary is demonstrated. The current
tracked `on state_change` callback is `Fighter.x21EC`: `fighter.c` invokes it
before final animation/callback assignment, then clears it; it is not a general
post-entry notification and cannot safely replace the original installation.

Correct hitbox semantics matter: `lbColl_80008434` only sets Enabled and
`lbColl_80008440` clears recorded victims. Full `ftColl_8007B064` additionally
sets `fighter.x2219_b3=true`. `fighter.c` tests that aggregate bit when clearing
attacks during later motion changes. Use `hitbox.clear_victims` and
`hitbox.set_enabled(index,true)` for their actual meanings; do not resurrect a
misnamed reset_detection operation. The remaining attack-presence bookkeeping
must be covered by an explicit generic setter/enable contract and an actual
later-motion-change test before claiming this restoration equivalent.

Import bounce tuning from0x4C/0x50, derive end frame from owned animation72,
and restore damages from0x58/0x5C/0x60. No donor union countdown is needed.

### Passive carried shield

`ft_08A1.c:ft_8008A348` and `ftCommon/ftCo_SquatWait.c` install the carried
shield after common entry setup. `ftlinkattackair.c:ftLk_AttackAir_800EB3BC`
and `ftclink.c:ftCl_Init_8014919C` test saved visibility group2 `.prev==0`.
The existing `visuals.current_group` reads the live model selector, not this
saved previous selector. Request a typed saved-group read and exact post-Wait /
post-SquatWait entry notifications; a tick poll changes timing and is unsuitable.

Generic guard creation, `guard.front_only`, `guard.hit_damage`,
`guard.hit_facing`, common pushback factors and owned recoil0xD8 already exist.
Two guard policies remain unexposed: x221B_b4 filters capsule x42_b4, and
x221B_b2 suppresses the contact-position/deflection calculation. These are
different from `guard.reactive` (b1). Name their public policies only after
tracing the actual capsule consumers in `ftcoll.c` (fighter and item paths);
never expose raw flag ordinal syntax. The two lightshield interpolation
coefficients x2E4/x2E8 also need generic named reads if still absent when this
batch starts. Recoil remains authored arithmetic from `ftCo_80092ED8`, followed
by velocity scale x294 and original direction; sounds160106/70106 are owned.

### Second forward smash, sword trail and Young Link taunt

`ftattacks4combo.c:ftCo_800CED30` asserts outside Link/CLink. Override the
ordinary first forward-smash interrupt at the existing per-motion callback
boundary, preserving the common IASA order around command0+A, and enter341
through normal state/animation primitives. Bind row341's ordinary animation,
ground physics/collision and interrupt policies directly. Do not call the
raw-kind combo helper. No new Link-specific Host method is justified.

`ftafterimage.c:read_trail` returns false for a registered definition without
`on_trail`, even while its donor is Link. The current template therefore loses
the sword ribbon. Existing `on trail` and packed-byte parameter import suffice:
copy SwordAttrs0x64..0x83, including its actual anchor, and reuse the renderer.
The existing source census found no private Link model-effect update policy
equivalent to Mario/Luigi Tornado; retain that bounded observation rather than
claiming all rendering exercised.

Young Link taunt entry is `ftclink.c:ftCl_Init_80149318` through
`ftCo_AppealS.c`; animation is `ftclinkappeals.c:ftCl_AppealS_Anim`.
Author342/343, command1 reset, owner-origin/LeftThumb Milk creation only when
absent, full cleanup binding only after success, command1==2 removal, and
completion removal before Wait. Existing taunt hook, constructors, semantic
bones, commands and common phase primitives suffice. Link needs no custom Milk
asset or invented taunt behavior.

## Definition, import and assembly requirements

1. **Article metadata:** frozen40 stock import records now cover Boomerang1/44,
   Hookshot2/188 and Young Link Milk5/-1. Normal40 composition imports/generates
   both packages without manual FMP. The copied-base article asset arrays still
   contain only Bomb0/72, Arrow3/12 and Bow4/16. Complete those immutable arrays
   before promotion; Link has five article kinds including Bomb, Young Link six.
   Graph6 is an immutable sword graph, not another article kind.
2. **Owned rig:** transfer the actual semantic and omission inventory from
   `.local/link-clink-closure-audit/rigs.json`, derived from
   `.local/mewtwo_common_stock.dat`. Preserve Link75 physical/76 runtime,
   omission68 under67; Young79/80, omission72 under71. Both have53 valid entries
   of54. ThrowN/TransN2 are distinct66/74 and70/78, unlike some other rigs.
   Existing Definition `.bones` and `.omissions` support these owned tables.
3. **Sword omission construction:** `ftLk_Init_OnLoad` / `ftCl_Init_OnLoad`
   call actual `ftParts_800753D4` with immutable item graph6. It loads TWO model graphs from the isolated descriptor and inserts
   current/alternate HSD joints,
   registers part/tree-depth/draw state and flags. Existing costume-public-root
   `attachments.load` does not do that. Add a generic immutable fighter graph
   declaration and omission-install operation using this actual body, validated
   against the declared omission/rig and with scene lifecycle ownership. No
   fake Item, raw script pointer or Link-specific setup delegate. Test both
   graphs, alternate pose, omission drawing, partial failure and teardown.
4. **Reserved tether endpoint:** retain article resource metadata part139 and
   generation/Definition ownership. It is separate from physical/semantic rig
   indices. Do not inflate joint_count to140 or overwrite the sword omission.
   Existing resource pool and borrower invalidation need no replacement API.
5. **Load/death/item events:** author walljump for Young Link, clip72 length,
   owned spin offset scaling and all item/visibility/knockback callbacks from
   `ftlink.c` / `ftclink.c`. Ordinary pickup/drop use1/1, visibility and knockback
   group1; extended pickup/drop conditionally switch heavy group1 and always
   group2. Existing methods/events suffice. Separate resource cleanup from
   OnDeath's handle/group reset; initialize every absent handle to-1.
6. **CPU/assets/results:** current named-kind census of `ftcpuattack.c` and
   `ftCommon/ftCo_0A01.c` finds no additional Link/CLink CPU decisions. Use the
   existing generic planner and owned seven-family profile import: both have
   counts14/6/3/2/7/4/0. Preserve each actual distance/profile table instead of
   inventing hooks. Demo constructors are common; copied bases already own
   result/intro/ending/wait names. Verify independent owned rig construction in
   those scenes separately; no actual results gameplay evidence is claimed.
7. **Readable assembly:** retain disjoint authored components and one source
   of each helper/field/article. A naive deduplicated merge of cleanup components
   and current Spin/Bomb has62 fighter fields, about208 aligned bytes in that
   declaration order, and58 global functions including full cleanup, before ground-hook assembly
   and remaining common policies.
   Current limits are64 fields,256 fighter bytes,64 functions,16 articles;
   each article has512 local bytes. Move temporary positions/results to typed
   locals where lifetime permits and consolidate ordinary scratch values before
   requesting any capacity change. No overflow padding or truncation. Keep
   authored component files below2000 lines and make the normal package build
   assemble direct definitions, not forwarding wrappers or duplicate behavior.
8. **Promotion:** complete every non-null stock table callback with owned
   generated pointers or genuine common engine primitives. Preserve Link's
   empty342/343 rows. Only then declare independent behavior/source_kindNone,
   retain owned animation/asset metadata and verify no stock Link decision
   address remains reachable. One-time donor import is asset provenance, not
   permission to retain runtime donor handlers.

The callback-table inventory is
`.local/link-clink-closure-audit/callback_tables.json`; its non-null lifecycle
families are load, special-attribute load, death, pickup/drop ordinary+extended,
visibility and knockback. Late/color update, absorb, auxiliary model events and
user-data removal are stock NULL; no fabricated policies are required there.
Resource and graph destruction still must be integrated with generic lifetime.

## Bounded completion sequence

1. Ship and source-test the eight common Catch hooks, then exercise accepted
   ground decisions and complete resource callbacks through actual common
   continuations. Keep defaults partial until those tests pass.
2. Assemble one immutable package per fighter from accepted components; close
   Bow six-phase fighter constructors and Young Link Appeal with fresh normal
   DAT/FMP import. Reuse cleanup40 presence/transfer/lifetime matrix against
   the actual assembled package, including Bomb's unchanged separate lifecycle.
3. Close Dair, passive shield, second smash, sword omission/trail and ordinary
   lifecycle/rig/CPU ownership in one independence source suite. Verify genuine
   needed generic entry/guard/model contracts before adding shared interfaces.
4. Generate/load independent definitions, audit callbacks and run coordinated
   hidden, muted, accelerated, drawn stock/script pairs covering four specials,
   tether standing/dash/air/reel/capture/release, Dair repeats, passive shield
   contact/filtering, second smash, item transfer, Young Link Milk, damage/death
   composition, respawn and CPU. Results entry requires a real scene receipt.
   Source fixtures' controlled allocation/ECB/creation/render boundaries are not
   substituted for those game checks.

Kirby copy, Classic bonuses and video demos remain outside this versus proposal.
Original stale-pointer/allocation UB and the documented sqrt reconstruction are
not targets for awkward native emulation. Preserve deterministic authored
behavior and disclose safety/rounding differences already classified by the
accepted source fixtures.
