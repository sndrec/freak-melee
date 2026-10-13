# Ice Climbers activation census

Status: **read-only preparation**. No Popo/Nana template, clone registration,
engine behavior, compiler, or default has been changed. No compiler build,
gameplay launch, or source-parity fixture was run for this census. Kirby copy
and hat behavior is excluded.

`census.json` records the exact live source SHA-256 values, both complete custom
motion callback tables, Popo/Nana columns of explicitly initialized ftData kind
tables, retail rig maps, omissions, costume hashes, and article descriptor roots.
The source anchors below refer to these pinned inputs; later source edits can
move line numbers. `census.py` only reads source and the retail ISO.

## Current coverage and owned data

There is no Ice Climbers FMS template/translation ledger in examples or docs,
no Popo/Nana entry in `native_fighter_runtime.c:stock_clone_bases`, and no
Ice Climbers slot in `native_fighter_article_bindings.c:nativeStockArticleSlots`.
This is an unported pair, not a nearly complete four-special template.

| Data | Popo | Nana |
| --- | --- | --- |
| Internal kind | 10 | 11 |
| Fighter DAT/root | PlPp.dat / ftDataPopo | PlNn.dat / ftDataNana |
| Animation archive | PlPpAJ.dat | PlNnAJ.dat |
| Physical/runtime joints | 49 / 49 | 49 / 49 |
| Semantic role bindings | 40 | 40 |
| Omission records | 0 | 0 |
| Costumes verified against retail | Nr, Gr, Or, Re | Nr, Ye, Aq, Wh |
| Async effect bank | 14 | 14 |
| Demo motion count | 14 | 14 |
| Article mask / typed format | 0x007 / ClimberString | 0x007 / ClimberString |

All eight costume descriptor graphs were traversed using HSD preorder semantics
(instance children excluded). Both PlCo inverse maps reconstruct exactly from
the forward mappings. The complete forty role/joint pairs are in `census.json`;
no guessed aliases or physical-count substitutes are needed.

`ftPp_Init_OnLoad` (`ftpopo.c:364`) registers exact slots **0 Ice, 1 Blizzard,
2 GumStrings**. Nana OnLoad does not repeat that registration, but both DATs
contain all three descriptors and both stock import layouts admit them. A
self-contained Nana definition must bind the descriptors it actually imports,
rather than rely on Popo's process-global item registration. Belay's slot2
special attributes contain two graph roots at serialized offsets0x24/0x28;
the existing typed decoder preserves them. Retail link count is **40** for
both forms (the decoder accepts2..64). Keep slot2 typed pointer metadata;
ordinary float-word import cannot replace it.

The demo strings are explicit in `ftpopo.c:330`/`ftnana.c:334`.
Nana's VIWait root is deliberately `ftDemoViWaitMotionFilePopo`.
Versus closure does not establish results/intro/ending/VI scene parity.

## Complete callback ownership

Both custom tables have **26 states341..366**, the same callbacks and flags.
States363/366 deliberately have four NULL behavior phases; the other24 states
have all four phases nonnull (96 phase bindings per table). All26 use the common
camera callback. Equality is of the parsed table content, not a claim that both
instances enter all states.

| States | Policy owner in the source |
| --- | --- |
| 341..342 | Ice Shot ground/air; shared Popo implementation |
| 343..346 | Primary Squall ground/air, paired/unpaired choices |
| 347..356 | Primary Belay startup/throw variants |
| 357..358 | Blizzard ground/air; shared Popo implementation |
| 359..360 | Secondary Squall follow/collision policy in ftnanaspecials.c |
| 361..366 | Secondary Belay policy in ftnanaspecialhi.c, including two NULL states |

The actual entry table is asymmetric: Popo has all eight special entries;
Nana only has ground/air Neutral and DownB. Nana's ground/air SideB and UpB
entries are NULL. Its coordinated states are entered by the primary/secondary
policy, not by copying Popo's four special entry hooks onto both definitions.

The nonnull ftData lifecycle slots are load/death, attribute load, pickup/drop
(ordinary and Ext), visibility, and knockback enter/exit. Both forms use Popo's
ordinary held-item/model group1 operations and common knockback calls. All
UnkMotionStates0..5, OnAbsorb, OnUserDataRemove, custom matrix/demo callback
slots and the two UnkIntBoolFunc0 columns are NULL. There is no additional
fighter-specific taunt/normal-attack implementation in those stock tables.

`ftpopo.c:364..451` sets load flags, the separate x40 scale scalar, group0/1
visibility, and persistent handles/counters. `ftnana.c:348..387` additionally
uses attrs.xC4 for x40 and attrs.xC8 for armor on death. Cleanup
`ftPp_Init_8011F060` is ordered **Ice -> Blizzard -> Belay -> Squall links**;
Nana damage/death2 cleanup `ftNn_Init_80122FAC` only resets Squall links.
Translate these into per-instance ordinary state and exact callback bindings.

## Existing paired engine path and its gaps

1. `player.c:38..95` contains a signed-byte primary/secondary internal-kind
   mapping plus a transformation flag. The stock IC CSS entry is `{Popo,Nana}`
   with false transformation; solo ChKind_Popo is a distinct entry.
   `Player_RegisterNativeCharacter` currently writes `{custom,-1,false}`.
   It cannot express two simultaneously active authored definitions.
2. `Player_80031AD0` (`player.c:227`) creates primary first with b0=false,
   publishes entity0, then secondary with b0=true and the mapping's transform
   flag, publishes entity1. Both share the player slot. Primary-first order is
   observable to Nana initialization. Coordinate allocation failure, teardown,
   rebirth, assets, package pinning, and slot UI; arbitrary script-time fighter
   spawning is unnecessary for this specific existing engine path.
3. `Player_8003248C` (`player.c:460`) treats the secondary of a nontransform pair
   as CPU even when the player slot is human. It already derives this from the
   mapping, so preserve it once generic pair registration is supported.
   `Player_80031EBC/31FB0/32070` operate on two published entities and maintain
   their shared player lifetime. Pair members are two independent live fighters
   and damage/capture recipients, not alternate forms of one active instance.
4. `ftData_80085FD4` (`ftdata.c:1965`) borrows a missing Nana animation entry from
   **the raw global Popo data**, outside demo mode. Native callers85CD8/85E50
   simply use the returned FigaTree. An explicit immutable per-definition bank
   fallback relationship must target the paired package's primary data, with
   validated clip bounds/lifetime. Do not make it a stock donor-kind lookup.
   `ftData_80086060` obtains the primary live fighter for the old archive path;
   its copy/relocate consumers are nonnative branches, not current native math.
5. `ftCo_800A589C` (`ftCo_0A01.c:2804`) scans live fighters for another same-slot
   fighter and rejects an inactive one. Many specials instead use exact
   `Player_GetEntityAtIndex(slot,0/1)`. A generic **same-player entity index ->
   validated full-width creation ID** query would cover these authored callers;
   resolve afresh and never persist a GObj pointer or truncate it to an Int.

Generic engine pair metadata should name two authored definitions, their
simultaneous roles/control mode and animation fallback, preserving the actual
existing spawn path. Package presentation (one paired CSS entry and optional
solo variant versus separate exposed members) remains a product decision;
this report does not commit a package schema.

## Follower controls and CPU decisions

The most substantial missing contract is follower control, not special input.

* `ftCo_800A101C` (`ftCo_0A01.c:659`) forces raw Nana CPU mode6, spacing15,
  initializes a fixed30-sample input/position/facing ring from its partner,
  read/write cursors0/5, then later calls the existing on_cpu_initialize hook.
  That current hook is too late to replace the earlier raw-kind mode/ring work.
* `ftCo_800B101C` (`:7644`) advances that ring via B0918, sets secondary AI level
  from primary damage/20 (integer truncation, cap9), chooses delayed mirroring
  versus autonomous targeting, handles distance/capture/UpB exclusions, and
  updates follower mode flags. Mode6 dispatch at `:8329` calls this stock body.
* `ftCo_800B0AF4` (`:7446`) is called after CPU decisions in B3900 (`:8687`),
  injects delayed buttons/sticks/triggers, optionally blends position0.95/0.05,
  suppresses facing copy during Blizzard, and can **directly enter stock DownB**
  when the primary's opposite-facing coordination rule fires. This body cannot
  remain active as an opaque engine follower primitive for independent scripts.
* Nana squat queue (`800A08F0:454`) has one-frame delay; aggressive travel stick
  (`800AA320:4842`) is0x40/0x7F. These raw-kind policies need generic ordered
  selectors/hooks; they are not covered by on_cpu_initialize/spacing_update.
* Existing CPU hooks cover later spacing initial1/update1*rand, and Popo's
  grab script59 choice at `ftcpuattack.c:1125` (attempts0, level>4, probability
  0.05*level). Keep short-circuit RNG order and the existing shared continuation.

Recommended split: keep fixed bounded input history, byte quantization and
controller publication as reusable engine mechanics; author copy-vs-autonomous,
distance/motion/capture tests, damage-derived level, movement/facing blending,
DownB dispatch, and command decisions in scripts. Minimal new contracts are
an ordered pre-CPU initialization policy, a follower-decision event at mode6,
a post-command control event at B0AF4, fixed-history record/read operations,
and typed control output publication. Expose named generic flags/counters only
where the source comparison proves they are needed; do not export CpuFighter
pointer/cursor layout or a `run_nana_ai` operation.

## Cooperation, capture, lifecycle and presentation

* Squall sets reciprocal `x1A5C` links (`ftpopospecials.c:134..187`,
  `ftnanaspecials.c:42..108`); these are **the shared hitlag recursion link**,
  also used by ordinary capture. Fighter_8006D044/D10C (`fighter.c:2714..2780`)
  traverse it. Ordinary exported state alone cannot reproduce this mechanism.
  Propose a generic validated sync-link set/clear operation, with generation
  retention and actual link teardown, distinct from capture/victim ownership.
  Preserve the original request-to-leave-hitlag ordering on both fighters.
* Belay changes partner motion, facing, position and animation rate, reads
  partner bone positions, records the thrown member's rope endpoint, and binds
  accessory callbacks (`ftnanaspecialhi.c:33..109`, `ftpopospecialhi.c:209+`).
  Existing typed recipient calls/exports can own decisions and expose own
  queried values once an entity ID is available; validate whether arguments are
  actually needed before adding generic typed request arguments. Bone values
  must use exact raw traversal versus semantic mapping at each original call.
* Primary Sleep dispatch currently removes its secondary via load flag b5
  (`ftcolanim.c:49`). Nana load flag b4 suppresses pick-up of the special item
  class checked in `ftpickupitem.c:51/111`. These are generic pair lifecycle and
  pickup eligibility policies, not safe substitutions with invisibility/armor.
  x2222_b2 is already the generic `damage.song_immune` property.
* Landing clears repeated aerial-Ice-Shot boost (`ftCo_Landing.c:82`); existing
  on_landing_action can reset the authored counter. Ground friction ignores
  stage material for both forms (`ftmovement.c:465`), with no current authored
  selector. A small immutable surface-friction policy can replace that rawkind
  test; there is no need for an IC-only physics primitive.
* Costume>=2 swaps voice ranges through `ftCommon_80080144` and
  `ft_80087D0C` case13 (`ft_0877.c:364`). The new seven-channel mute mask does
  **not** supply this remap. Prefer immutable generic bounded range/costume
  remap metadata at the actual sound-ID mapping boundary, retaining ordered
  range tests. Explicit special voice choices belong to owned script tuning.
* Squall's accessory manipulates a retained effect model's child visibility
  and root floor tilt (`ftpopospecials.c:54`). Named model-effect policy can own
  the decision, but original animation-command creation/timing and part layout
  still require a separate actual-source/retail command receipt before replacing
  its private pointer transport. Effect4ED is attached-child model construction;
  4EC Blizzard and4E9/4EA/4EB Ice visual families are generators/common effects.
  No IC-private EF updater found by this source census; do not replace particle
  dispatchers merely to claim model policy ownership.

## Articles and bounded next batch

Ice uses four states and the actual it_3F2F logic row (`:1266`): destroyed,
damage/clank/shield/reflect/absorb/bounce/events all require authored callbacks.
Its saved source fighter is distinct from current damage owner after reflection;
cleanup notification should use the retained creation ID and typed recipient
export, not current-owner inference. Launch, shrink, floor acceleration,
bounces/lifetime and transitions are ordinary script scalar policies.

Blizzard has one state plus damage/reflect/clank/shield/absorb/bounce/event
callbacks (`it_3F2F.c:1284`). Constructor draws RNG once for angle, initializes
velocity/lifetime before update, performs debug notification/manual lifetime/
initial physics in order (`itclimbersblizzard.c:53..130`). These are mostly
existing generic constructor/collision primitives; verify exact debug-call
contract before declaring the constructor closed.

Belay String has four states sharing animation-only policy (`itclimbersstring.c:23`),
picked_up/event logic (`it_3F2F.c:1392`), and three accessory solver schedules.
It allocates40 linked models in retail, attaches the item, tracks two moving
endpoints and releases models in order. Existing bounded article resources,
named model roots, attachment matrices, animation and typed target APIs cover
much of the mechanics. A direct solver-expression comparison must establish
remaining fields/endpoint control and constructor draw/order requirements;
do not call the stock String helpers from scripts.

First tractable source batch: **shared ground/air Ice Shot + Blizzard, all Ice
and Blizzard article phases/events, owned load/death/item callbacks and tuning**,
keeping paired activation absent. Follow with generic paired registration/bank
and fixed-history foundation, then complete authored follower/cooperative Squall
and Belay. No independent declaration until all stages have source acceptance.

The future fixture should extract actual old complete bodies, compile actual
generated modules, compare order/math/RNG/owned state, and test missing/recycled
primary, secondary, item and Definition boundaries. Healthy engine spawn/control
and staged source safety are separate checks. Game evidence later must include
secondary follow/desync/separation, primary/secondary capture, death/rebirth,
paired and solo Squall/Belay, reflection and cleanup; idle equal CPU traces are
insufficient. No such evidence is claimed by this report.

Concrete source issues to pin separately: SpecialS_80120FE0 reaches a missing
return when the rope handle is absent; the string allocator uses an uninitialized
previous link on first-allocation failure; some old code casts pointer identity
to u32. These are diagnostic candidates, not justification to reproduce native
undefined behavior or omit lifetime checks. Retail instructions should resolve
ambiguous contracts before choosing authored safety deviations.

Datatype boundaries: the mapping stores signed-byte kinds (current runtime
capacity128); CpuFighter stores thirty fixed0x1C samples, each with byte sticks
and triggers, full button bits, Vec3 and Float facing, plus native cursor
pointers. Climber fighter vars and motion vars contain native pointers; their
console offset comments are not native serialization offsets. The0x15C fighter
attribute record has Int fields at0x1C/0x68 and padded byte spans, while rope
attributes have two actual descriptor pointers. Owned script storage should use
full32-bit IDs, typed Int masks/counters and named scalar imports, never copy
private pointer-shaped unions or reinterpret descriptor words as floats.

## Reproduce the census

```powershell
python tests/native/ice_climbers_activation/census.py --root . --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --output tests/native/ice_climbers_activation/census.json
```

This reads the ISO without modifying/exporting its assets. The resulting file
pins the eight costume archives and PlCo/fighter DAT members, not the entire
1.4GB ISO. It does not validate animation coverage, renderer behavior, solver
math, package presentation or deterministic replay.
