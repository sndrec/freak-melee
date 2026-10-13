The grouped hidden console fixture compares interpreted Fighter hitbox state
and damage operations against the actual native setter, scale math, smash
charge multiplier and stale-move calculation. Native player stale-table lookup
is a controlled read of fixed fixture records. Source frontend, preparation,
publication, actor/VM, constants and GObj pools are actual implementations.
All four capsule slots and native states 0 through 4 execute actual source
queries; `Hitbox.disabled` and the enabled-only GameWatch source condition are
compiled through the current constant catalog. Damage comparisons cover native
scale branches, smash charge, stale reduction and DebugRom bypass. Integral
reference inputs use independent native C casts, including defined high-bit
words after scale reduction. Invalid indices, malformed arithmetic/common data,
physical identity changes and a faulted actor preserve outputs/fields.

Live damage observations run through a compiled Float-returning helper for
all four slots and native states 0 through 4. They preserve native Float bits,
including negative zero, subnormals, infinities and a NaN payload, without
changing Fighter fields. Wrong indices, malformed calls, retired identities
and non-Fighter contexts leave results unpublished. These reads do not run
the setter, recompute staleness or claim complete charged-move gameplay.

Native signed-to-unsigned narrowing and Float damage conversion are tested
separately from the native setter's defined signed-count range. Values whose
post-scale/post-smash Float cannot convert to Int32 are rejected before writes;
they are not compared against undefined native casts. No geometry, hit
detection, asset import, rendering or game acceptance is claimed.
Dynamic bounded indices extend the old reference emitter's literal-only
admission; no invalid-index native execution is manufactured. Packed authored
state restoration explicitly restores its native Fighter peer, while the
controlled stale-table input remains unchanged across the roundtrip.

Repeat-hit group queries and setters run through source compilation, the
prepared actor, VM and actual catalog/backend. Four slots, all five capsule
states and full-width tags compare the entire Fighter against native field
assignment and the stock non-disabled alternating-group policy. High unsigned
and negative signed assignments preserve native UInt32 narrowing. Invalid
slots, floating or malformed Bool tags and retired generation/epoch preserve
state and output. No collision-contact scheduler or rendered egg roll runs.

```powershell
python tests/native/script_fighter_hitboxes/run.py --compiler .local/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-fighter-hitboxes-final
```
