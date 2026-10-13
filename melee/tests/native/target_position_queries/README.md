# Retained article translation and nearest eligible fighter

Two generic engine contracts close the remaining observations needed by a
model-driven projectile CPU policy. Neither reads Zelda private state or calls
a stock special handler. Host/compiler integration belongs to the parent batch.

```c
bool nativeArticleRef_PrimaryModelTranslation(HSD_GObj* caller,
    uint32_t handle, ItemKind kind, Vec3* output);
bool ftCo_CpuNearestEligibleTarget(Fighter_GObj* source,
    const Vec3* origin, uint32_t* target);
```

The first query reads the primary HSD model root's **local translation** using
actual `HSD_JObjGetTranslation`. This is exactly the observation in original
`ftCo_800B9504`, and is different from `Item.pos` or `lb_8000B1CC` world position.
It resolves the full article generation in the caller's immutable author
Definition, independent of its current damage owner. It verifies the recipient
kind and active logic, caller live identity/author/logic, recipient live
identity, model root and removal state before committing the Vec3. Fighter
and article callers are accepted; a mismatched item caller logic is rejected.
Failure leaves all output bytes untouched. No speculative world-query variant
was added; the existing model world-position operation remains available.

The second contract directly renames/reworks actual `ftCo_800A4E8C` in
`ftCo_0A01.c/.h`; there is no old entry, wrapper or duplicated eligibility
algorithm. Its sole retail caller in `ftcpuattack.c` resolves the returned ID
and revalidates its source before continuing. Script integration can bind the
same actual function. Native guards preserve generation, userdata, kind,
Definition and fighter-data identity of the source and selected target.
Missing source, no candidate or invalidation leaves the output ID untouched.
There is no retained Fighter pointer outside the synchronous query.

## Preserved nearest semantics

The original shared helpers are unchanged and verified byte-for-byte against
the frozen reference:

1. Traverse the live fighter list and skip the caller.
2. `inlineD0`: reject positions outside blast bounds inset by the source CPU's
   half-width/half-height; equality is eligible.
3. `ftCo_IsAlly_dontinline`/`ftCo_IsAlly`: same-player and exact game/team rules.
4. `ftCo_800A1C44`: reject the original invulnerability/state flags and counters.
5. `inlineD1`: original state flags, `ftCo_800A0F00` and dead-motion predicate.
6. Compute the original ordered binary32 distance expression and replace only
   for strictly smaller distance, preserving first-in-list equal-distance ties.

Empty registry nodes are safely skipped. No new finite-coordinate clamp,
normalization, CPU-target mutation, RNG draw or tie sorting is added. These
eligibility dependencies are pure shared engine observations; they do not
normally invoke authored callbacks. The fixture deliberately injects mutation
at controlled observations to verify retained identity rejection, not to claim
that the shared getters are callback dispatchers.

## Reproduction and boundaries

```powershell
python tests/native/target_position_queries/run.py `
  --snapshot .local/baseline68 `
  --output A:/FreakMelee-test-archives/target-position-queries68-root
```

Use a fresh output directory on A:. The runner verifies all immutable snapshot
inputs, hashes the current assigned sources, overlays only the new refs header,
and compiles the actual complete refs and callback-lifetime units. The nearest
body, its identity predicate and unchanged eligibility helpers are extracted
from actual source; the old nearest body is independently extracted from
frozen68. Both bodies execute against the same controlled game-mode, stage,
special-state and Definition observations. Header overlay/current source is
explicitly recorded; this is not falsely labelled an unchanged frozen68 SDK.

The final matrix accepts 2,018 original/new nearest pairs, 21 article query
cases and 38 injected invalidation cases. Full-width IDs include values above
2^24 and with bit31 set. It covers blast boundaries, team modes, state filters,
list ties, reflected ownership, wrong author/kind/logic, missing models,
UINT32_MAX failure sentinel and unchanged failed outputs. Six retirement
cases protect old Fighter/Item payloads with PAGE_NOACCESS. Kind, data,
Definition, item-logic, removal state and model-root changes are rejected at
the tested query continuation boundaries.

`receipt.json` records the exact assertion count, hashes, extracted bodies and
compile/run commands. The fixture does not execute the entire CPU scheduler,
renderer, stage implementation, generated query expressions or Zelda Din
decision. Those remain separate compiler/module and full paired composition
acceptance boundaries. No games, default activation, package installation or
global build is performed.

Owned production paths are `src/melee/it/native_article_refs.c/.h`, the exact
nearest section of `src/melee/ft/kinds/ftCommon/ftCo_0A01.c/.h`, and the sole
caller section of `src/melee/ft/ftcpuattack.c`. Root owns Host, compiler and CMake.
