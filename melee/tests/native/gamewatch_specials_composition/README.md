# Game & Watch specials source components (unregistered)

This namespace is a source-only, partial Judge / Fire Rescue (up special) /
Oil Panic composition against immutable SDK53. `candidate.fms` is deliberately
NOT a default template, imported clone, independent fighter, or installed module.
The accepted Chef52 namespace is unchanged. Smash torch (`itgamewatchfire`) is a
separate normal-attack prop, not the Fire Rescue article covered here.

## Reproduce

```powershell
python tests/native/gamewatch_specials_composition/run.py --snapshot .local/baseline53 --output .local/gamewatch-specials53-root
```

The runner verifies every snapshot manifest SHA, uses its `native_content_tool`
for `script-c`, then compiles the emitted C using frozen Clang and SDK includes
(VC tools14.44, GNU11, O1, FP contraction off). It does not use `script-build`
with embedded live include paths, launch a game, perform an ISO import, or install
anything. Original stock inputs are pinned in `original_inputs.json`; unchanged
fighter files and mechanically extracted article/common pause bodies are used.
The source components must assemble exactly to `candidate.fms`.

Final local receipt `.local/gamewatch-specials53-final/receipt.json` reports
**1,067,939 checks, zero failures, 45,880 paired callbacks**:

- 33,280 phase pairs: all four slots for every Judge355–372, Rescue373–374,
  Panic375–380 motion, with64 command/input/collision flags and five frames.
  The matrix checks ordered common calls, motion/flags/frame, commands, all
  velocity components, acceleration, jumps, facing/launch angle, state history,
  release/turn counters, absorber flags and enabled-only hitbox damage writes.
- 10,368 weighted Judge rolls:9×9 history combinations, four edited weight
  distributions,32 RNG states; checks a single equal RNG bound/advance and
  exact chosen index/history. All nine numbers must actually be reached.
- 384 ground/air entry pairs: six entry routes×64 states, including full-bucket
  shoot decisions and both original command-variable damage truncations.
- 1,848 article pairs: present/absent healthy creators,50 motion IDs and both
  facings; Judge animation/pickup/interaction, Panic animation/destruction/
  interaction, Rescue animation/empty physics/collision; all three pause/resume
  signals with actual unchanged `it_8026B724/73C` bodies and all relevant bits.

`fighter_parameters.fmp` owns all tuning in this comparison. Values are synthetic
controlled matrix inputs, **not claimed retail DAT defaults**. The common stick
threshold is currently fed equally from a controlled parameter and original
`p_ftCommonData` until the semantic engine query below exists.

## Files and source boundaries

`fighter.fms` contains the actual phase/entry policy. `articles.fms` contains
accepted prop callback slices. `candidate.fms` assembles them. `pending.fms`
contains UNCOMPILED replacement fragments for the remaining setup, absorption,
constructor and pickup policies. Do not append it verbatim: those fragments
replace explicitly pending helpers/article clauses. `compare.c` tests fighter
policies; `article_compare.h` contains controlled article boundaries and matrices.
`run.py` performs extraction, generation, compilation and receipt construction.

The accepted candidate's three setup helpers are intentionally empty placeholders
for the **unexecuted** constructor/accessory paths. The original setup callbacks
are retained only as callback-presence comparisons and abort if mistakenly run.
No constructor, allocation-failure setup, bonus-food spawn or absorb callback is
claimed accepted. Generic signal dispatch is controlled here; its separate actual
native lifetime/author test suites provide backend coverage, not this fixture.

Shared physics, world matrices, animation evaluator/transitions, collision/ledge
queries, sound and material-group selection are controlled ordered boundaries.
The actual generated callback uses the real full32-bit registry-based
`nativeCallbackSource_Alive`; controlled identities include fighter0xFEDCBA98
and article0x80000001. Model joint rotation uses actual HSD inline setters;
matrix dirtiness propagation is controlled. No pixels, replay or rollback claim.

Judge/Panic `owner` policy is tested for their live original creator and NULL.
It does not cover foreign/reflected holders, poisoned owner pointers or retired
creator reuse. Rescue tracks creator independently of current damage owner in
script; the present/absent healthy routes are covered. Its original animation
removes the object then rereads userdata. The authored callback detaches and
removes terminally, returns false instead of the original true after removal,
and has a source-generation guard. The fixture preserves memory for the old
reread, checks equal immediate removal/owner detach and explicitly permits the
terminal return delta only when BOTH versions removed the Rescue. It does not
pretend to reproduce the original use-after-free.

The scoped cleanup chain is Judge→Panic→Rescue. Complete stock OnDamage includes
Greenhouse→Manhole→Torch→Parachute→Turtle→Sparky before these three, so this is
not complete lifecycle acceptance. No OnLoad/OnDeath overrides are installed.
Full lifecycle must initialize owned handles=-1 and history Judge1/0; death resets
Judge1/0, impulse flag0 and Panic damage0, but **must preserve Panic charge**.

## Exact remaining generic contracts

1. **Absorber hits RO Int**: actual `Fighter.AbsorbAttr.x1A48_hitsTaken` in
   `ftGw_SpecialLw_AbsorbThink_DecideAction`. Order: charge+=hits, signed damage
   accumulator=int(float(old)+absorber.damage), if charge>=full color5, choose
   ground/air Catch motion, change motion, update bucket visibility. Existing
   active/create/damage/skip_position_update operations otherwise suffice.
   `ftColl_8007AF10` only clears `skip_update_pos`, so the accepted script uses
   the existing direct `absorber.skip_position_update=false` after physics;
   a separate refresh operation is unnecessary.
2. **Common horizontal stick deadzone RO Float**: exact
   `p_ftCommonData->horizontal_stick_deadzone`, used by Oil Panic turn IASA.
   Suggested `controls.horizontal_stick_deadzone`. Do not invent a GW threshold
   constant or copy dynamic shared tuning into persistent scratch state.
3. **Live target air-state query RO Bool**: suggested
   `targets.airborne(Int handle)` or equivalent semantic target state. Panic
   PickedUp uses current `Item.owner` through `ftLib_800865CC` and writes item
   ground/air, chooses state0/1, then `Item_802694CC`. It cannot substitute the
   original creator, current motion range, or item's prior grounded state.
   Missing targets require an explicit default/exists policy; query after live
   full-generation ID resolution rather than raw userdata access.
4. **Ordinary engine item spawn descriptor / bookkeeping**: Judge number7
   calls common `it_8028FAF4`, which gates ordinary Foods availability through
   `it_8026D324`, takes supplied left-thumb position as prev_pos, current center
   from `it_8026BB68`, facing=-1, zero velocity/damage/group, NULL primary and
   secondary parents, collision=true. After `Item_80268B18` succeeds, increment
   the global ordinary food count via `it_80274ED8` (`it_804D6D0C++`). Current
   `items.create_engine` uses source parents and equal positions; authored
   `create_unowned` requires authored logic and is not an ordinary Foods path.
   Extend the substantive common constructor with explicit positions/parent
   roles, plus a reusable availability/count policy; no Judge-specific API or
   delegation to the stock special handler. This is separate from palette.

Judge weighted RNG needs no new engine primitive: script sums nine owned Int
weights excluding the last two outcomes, makes ONE `random.int(total)`, selects
in ascending order, then updates previous/latest. Nonnegative weights with a
positive eligible total are the compared defined domain. Stock leaves the result
uninitialized for unsupported tuning; this fixture does not claim that undefined
case or emulate its stack garbage.

`item.set_appearance` already binds actual `it_80273670`, `physics.air_motion`
binds actual85154, and model scale is exactly
`model.instance_scale_y * model.base_scale`. Rescue's2.5 offset multiplies that
binary32 intermediate. No new Fire velocity/angle/scale helper is needed.

## Constructor and presentation order still pending

Judge setup: clearcmd1→right-thumb world position→create Judge (both parents
owner, XYZ retained, collisiontrue)→if selected6 left-thumb offset(0,5,0) and
ordinary food spawn→bind damage/death only if Judge exists→bind hitlag→clear
accessory. Judge created: attach(rawpart32 from verified copied rig) including
pickup→appearance descriptor0/frame(selected+1)→numeral child-child Y rotation
π if current owner facing1 else0→debug owner→paper setup. History changes BEFORE
creation and does not roll back when allocation fails.

Panic setup: create only when absent at rawpart0 world position, both parents
owner, XYZ retained, collisiontrue; always conditionally bind damage/death and
bind hitlag/clearaccessory. Panic created: attach/raw0→debug→paper setup. Pickup
clearscmd1/cmd0→copy current-owner air state→state0/1 animation→advance animation.
Shoot damage writes command1=int(old_damage*multiplier), then separately
int(float(command1)+addition); clear charge/damage; bucket groups; cue binding.

Rescue setup: if absent, rawpart0 position→Y=-(2.5*common_model_scale-Y)→spawn
with Zflattened, collisionfalse, both parentsowner; creation_value=motion-373;
conditional damage/death binding→hitlag binding→clearaccessory. If already
present original leaves accessory unchanged. Constructor ignores its part and
2.5 arguments: clearcmd3/2/1/0→boundsflagfalse→capture fighter owner→paper setup→
state0/1→advance animation. No attachment operation. Cleanup conditionally
clears the original creator's fighter handle only when damage owner equals it,
then clears creator/current owner and removes terminally.

Actual PaperArticles resource import, layered renderer, palette/width/material
policy and real GameWatch base remain outside this namespace and acceptance.
Those are the source-backed generic proposal in the earlier Chef audit; do not
replace them with stock `it_8027CE64` or mutable private-union pointers.
