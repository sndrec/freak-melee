# Shared absorption, input, target and ordinary-food contracts

This namespace is unregistered and leaves the accepted SDK53 specials component
unchanged. It tests actual generated callbacks using frozen SDK56; it does not
provide a playable GameWatch base, a paper renderer, or default activation.

```powershell
python tests/native/gamewatch_specials_contracts/run.py --snapshot .local/baseline56 --output .local/gamewatch-specials-contracts56-root
```

The runner verifies every snapshot hash, extracts unchanged original bodies,
checks their complete source-file receipts, generates with the frozen content
tool, and compiles with frozen Clang and SDK headers. It selects vcvars14.44
explicitly, uses GNU C statement expressions and disables contraction. It never
uses snapshot `script-build`, which embeds live source include paths. All outputs,
logs and binaries belong in the supplied output directory.

## Accepted scope

The source matrix passes **11,140 checks** and **702 original/generated pairs**:

- 250 Oil Panic absorb pairs: charge/hit combinations, ground and air selection,
  color-before-motion-before-bucket ordering, and exact integer damage addition.
  Damage cases include16777217 and16777219; accumulator7 does not overflow.
- 448 turn pairs: four dynamically queried common deadzones, both facing
  directions, positive/zero/negative turn counters, held/released B and threshold
  equality. Original turn-frame Float conversion to Int is preserved.
- Four healthy/NULL-current-owner Panic pickup pairs: command1/0 clear first,
  current holder air state, item state0/1 with flags2, then animation advance.
- 40 generated food-call cases: coordinates evaluate once in XYZ order;
  success/failure, source removal, address recycling and detached poisoned
  userdata. Removal stops both Bool assignment and subsequent script write.
- 16 generated fighter-helper/article-physics query cases use16777217,
  2147483649,UINT32_MAX and0xFEDCBA98 as full-bit identities. Healthy ground/air,
  retired sources and recycled creation generations retain exact query meaning.
- Two extra stale-owner pickup safety cases are authored-only. The original
  reads retained owner userdata directly; that undefined input is not compared.

The actual `native_fighter_target.c` and `native_callback_lifetime.c` units run
against their live GObj lists. Original `ftLib_8008741C/460` registry bodies are
mechanically extracted. Common motion, facing, color, model-group presentation,
item animation and food allocation are controlled boundaries. Unused camera,
shield and model-scale dependencies assert if called. No stock callback is used
as an authored gameplay delegate.

The separate root-owned ordinary-food backend fixture verifies actual
`it_8028FAF4` availability, spawn tuple and success count. This namespace binds
a controlled Host result to isolate generated ordered continuation; it does not
duplicate or claim that engine allocation simulation.

## Contract details

`absorber.hits` and `absorber.damage` are readonly Int observations of the actual
signed `AbsorbAttr.x1A48_hitsTaken/x1A44_damageTaken` members. Previous drafts
incorrectly described damage as a Float source. Scripts may convert Int to Float
for healing formulas, but an integer accumulation needs no rounding detour.

`controls.horizontal_stick_deadzone` reads current shared common tuning through
the named enum and Host query. It is readonly Float and is not imported into a
fighter-owned parameter snapshot. Editing common tuning takes effect immediately.

`targets.airborne(Int)` resolves live creation IDs, consumes no RNG or allocation,
and returns false if absent. It is available in fighter and article callbacks.
Zero is the missing fighter-reference value. UINT32_MAX is an ordinary valid
fighter generation here, unlike an article handle's -1 sentinel. Current holder
sampling intentionally follows the current live owner; retained IDs reject old
generations. `targets.is_fighter(item.current_owner)` distinguishes absent owner
from live grounded owner in the article pickup callback.

`items.spawn_food(x,y,z)` returns allocation success as Bool through direct Host
`spawn_food`, bound to common `it_8028FAF4`. XYZ becomes previous position; the
caller center supplies current position. No parent is invented. Source-generation
checks execute after the synchronous spawn returns, before the RHS store or
continuation. Existing article `item.spawn_food()` remains its distinct ordinary
Food-item dispersion operation. Fighter food spawning is unavailable to article
and model-effect callbacks.

`compiler.c` covers valid typed reads in both contexts, exact high-bit literals,
required Host checks, readonly writes, property-as-call misuse, Float handle
rejection, malformed arity and model-effect context rejection. Root registers
this with the normal compiler source list and parameter implementation.

## Composed draft and remaining boundary

`compose.py` reads and hashes the accepted SDK53 fighter/article/pending/FMP
components. It resolves the setup helpers, food call, live pickup query and
absorb callback into fresh output, removes the obsolete controlled deadzone
parameter, and preserves all other accepted source slices. The composed source
and generated C **compile** with SDK56. New prop setup/constructor callbacks and
presentation are only syntax-checked here; they are not executed by this source
matrix. `composition_inputs.json` and the final receipt pin their provenance.

Paper setup remains explicitly excluded: immutable paper resources, layered
renderer, palette/width/material policy, real GameWatch base/import, full
lifecycle composition and gameplay need separate acceptance. The three-prop
damage chain still excludes six earlier props. Do not publish the composed draft
as the default or claim complete fighter independence from this checkpoint.
