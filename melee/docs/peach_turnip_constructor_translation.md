# Peach vegetable constructor composition

This is an **unregistered source candidate**, not a Peach clone or gameplay
acceptance. The matching frozen30 compiler/SDK generates the callback module
from `tests/native/peach_turnip_composition.fms`. Its constructor, pickup and
vegetable-resource callbacks pass **5,295 original-source/generated pairs and
235,461 checks**. No production template, registry or shared runtime changed.

The source fixture compiles eleven unchanged bodies: `Item_InitSpawn`,
`it_802BD32C`, `it_802BD4AC`, Turnip pickup/destruction, and Peach's `pickVeg`,
`getVeg`, `setupVeg`, `spawnVeg`, unset and vegetable cleanup. The actual
`ftCheckThrowB0` inline header also executes. Generated callbacks come from the
real compiler; their item create/attach/scale/bind/clear/read calls use the real
`native_item_owner.c` and real generation registry.

Allocation, authored-article creation, `Item_8026AB54`, appearance/state entry,
geometry, pickup notification and effect construction are explicit controlled
engine dependencies. This fixture compares the **caller composition and
ordered arguments**, not their internal implementations. The separate accepted
97-case owner backend and actual attachment fixtures cover those ownership
boundaries; their API tests alone do not prove Peach caller parity.

## Ordered authored behavior

The consumed article cue obtains the world position of raw part109, draws the
rare gate, then draws either the rare item's weighted choice or the Turnip's
face choice. Ordinary Bomb/Saturn/Sword creation selects ordinary engine item
behavior. Turnip creation invokes owned initialization before held attachment:
lifetime, weighted face, face damage, initial pickup flag and sound mute flag.

Every branch uses the direct held attachment operation once. Its pickup
callback runs while the fighter still has its previous held/vegetable binding.
The temporary creation handle is an Int local; it is published only after
attachment and scale inheritance. Scale comes from the item's current owner,
including a live owner redirected by pickup. Sword skips inheritance and
remembering. The other three retail choices remember their inherited throw
scale. A failed allocation publishes no held item and a `-1` vegetable handle.

After publication, the script issues pickup notification and effect1234 at
the fighter's current position, then installs the owned resource callbacks.
The comparison checks those mutation points and call order, both facings,
current-owner scales, all128 rare-gate draws, all rare weighted draws,
supported rare counts1..3, every face draw with counts1..8, and four exact
handles including `0xFEDCBA98` and `0xFFFFFFFE`.

Repeated Turnip pickup preserves the first/repeat state0/state4 decision,
appearance frame, sound bit and animation advance. Destruction forgets the
original creator's vegetable handle. The vegetable-only interruption callback
removes and forgets a Turnip only when it is still the held item. It retains
the resource reference for ordinary rare items and non-held Turnips, matching
`ftPe_SpecialLw_8011CFA0` rather than broadening cleanup.

## Parameters and deliberate boundary

The 28 named values use the normal parameter importer/parser. The console
import entry calls the actual importer on retail `PlPe.dat` as Peach; it does
not fabricate a registered project. These declarations are owned literal
defaults, independently checked against the hashed retail census: rare gate
128, weights2/3/1, Bomb/Saturn/Sword kinds6/7/12, lifetime140, face weights
35/6/5/3/3/4/1/1 and damage2/2/2/2/2/6/12/30. Raw109 agrees with the source
position selector and retail held attachment descriptor, within114 parts.

Valid comparison configurations have positive weight totals and counts within
the stated bounds. Arbitrary replacement kinds, malformed counts/weights,
retired creator identity and invalid native callback behavior are not claimed
as original-source parity. The original selection loops perform an unused
next-weight read after finding the result, potentially past their declared
table. The script omits that read while preserving every defined weighted
selection and RNG call. No RNG fields or semantic digests are masked.

The article declares exact state animation indices1/-1/2/2/-1 but intentionally
has **no motion phase callbacks**. This candidate cannot replace the Turnip
article lifecycle. Throw/drop/collision/expiry, stored-scale restoration,
Float/Parasol/Toad and global cleanup remain separate work. In particular,
`ftPe_Init_OnDeath2` cleans Parasol, then Toad, then the vegetable; registering
the candidate's vegetable-only helper is a bounded comparison of resource
registration and must not be promoted as that complete lifecycle callback.

There is no registered Peach clone project in frozen30. A complete package
will need its owned rig/assets and the remaining authored callbacks before
activation. No gameplay process, UI operation or global build ran here.

## Reproduce and evidence

Evidence lives under `.local/peach-turnip-composition30`: `source.json`,
`result.json`, `run.log`, `generated.inc`, `owned.fms`,
`fighter_parameters.fmp`, `retail-peach.dat`, `compile_import.cmd` and
`compile.cmd`. Frozen29 preparation and the temporary cleanup-scope diagnostic
are retained separately; final counts use regenerated frozen30 inputs only.

Compile `peach_turnip_composition_import.c` with the matching SDK's parameter
importer, parameter parser, article bindings and `sysdolphin/baselib/dat.c`.
Pass retail Peach DAT, the candidate source, output owned source and output
FMP paths. Run the frozen content tool's `script-c` on that owned source with
the FMP beside it. Then generate the original bodies/parameter fixture:

```text
python tests/native/peach_turnip_composition_fixture.py --root .local/baseline30/sdk --output .local/peach-turnip-composition30 --parameters .local/peach-turnip-composition30/fighter_parameters.fmp --generated .local/peach-turnip-composition30/generated.inc
```

Compile `peach_turnip_composition_test.c` with actual
`native_item_owner.c` and `native_callback_lifetime.c`, the matching source
and Dolphin include directories, the fixture output include directory and
`MELEE_NATIVE=1`. The saved command files also specify the Windows DLL CRT and
math declarations. Only this console fixture executes.
