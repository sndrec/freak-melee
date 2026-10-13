# Prepared Rook Item lifetime audit

This follows the current direct source path after owned blueprint/PNG resources
were prepared. It does not modify production/package/runtime/FMS or claim game
activation. The source contains nine Workshop recipes with eight icons and one
owned-model recipe, sharing the named blueprint `items/base.fib`.

`nativeScriptItemSpawn_PullHoldable` resolves the exact source program/owner and
data-only recipe, rejects an occupied hand, and reads the actual imported hand
joint index. This is consistent with the actual `ftLib_80086630` raw joint access;
the `Fighter_Part` type/comment alone is not evidence for another remap. It calls
the public authored constructor with author+owner, then Created, native pickup
and the Fighter inventory callback, checking caller/Item lifetime between stages.
No handle is published after retirement/fault. Existing connected factory tests
cover this ordering and caller retirement; real pull in the prepared Rook scene
is a separate parent live gate.

The actual constructor bypasses `Item_80267978` stock bank lookup for a nonzero
source owner. `nativeItemBehavior_Select` selects its exact spec/assets and
attaches owned state before model construction. Its empty shared capability
table cannot install authored C callbacks. Missing program/spec fails construction;
common userdata cleanup detaches the owner. Spawned runs only after the native
constructor has assembled model/scheduler state. Ordinary native Common Item
data is still loaded by `it_8027870C` even when random stage items are disabled;
Food is a shared engine operation, independently guarded by actual asset presence.

The direct shared holdable protocol selects four native states. Held state0
does not age or activate tick/collision hooks. Resting state1 ages/ticks but has
no collision callback. Throw/drop reset age and choose state2/3. Throw constructs
the actual native hitcaps before the typed WorkshopThrown callback. A custom tick
owns its physics choice; generic falling physics runs only without that hook.
Custom collision/hit Bool decisions supersede the shared no-hook response.
The Item scheduler preserves true-removal meaning, including independent Food
creation, and logical retirement stops source continuation before final detach.
The real actor and callback/native pool identities remain authoritative; missing
source Spec/status cannot enter an authored native table.

`nativeHoldable_Draw` now resolves the exact Item Spec. Missing Spec returns
without drawing. An icon recipe requires exact visual bank/identity/owner/PNG/
length/size and valid decoded pixels; it has no stock visual lookup fallback.
A no-icon recipe draws only its exact owned Article/model through the shared
native renderer. Immutable pixel preparation, real blueprint imports and actual
draw evidence belong to the parent/visual work; the console corpus does not
provide them.

The generic holdable comparison matrix previously established shared physics,
hitcaps, all contact masks and reentrant fault/retirement behavior against actual
native bodies. Its synthetic recipes were insufficient evidence for the exact
Rook corpus. The new test-only
`A:/FreakMelee-test-archives/script-rook-items-final/receipt.json` closes that
source-hook/protocol gap with 997 checks / 65 cases, nine actual recipe declarations,
all throw/tick/age/hit/collision behavior, native removal paths, real Food factory/
constructor and missing-spec rejection. Imported models/common attributes/geometry/
Food asset policy remain controlled; no factory/render/game acceptance is inferred.

No concrete production defect was found in the console corpus. The separate
actual-client replay now supplies real pull→held→throw→hit evidence:
`A:/FreakMelee-test-archives/source-rook-jump-snack-live-root/snacks/receipt.json`
completes 1900 input samples using the private prepared source Rook package.
It records source Item kind 237 in held/thrown/resting states, and Mario's damage
changes from 0 to 19.97 at sample 1248 as a thrown snack enters its hit response.
Rook completes all requested interpreter ticks without a fault, and traced
Fighter fields remain finite. This is gameplay evidence for the exercised
sequence, not exhaustive live acceptance of all nine recipes or Food behavior.

Framebuffer screenshots have been captured, but the small held item is not
visually unambiguous; decoded billboard drawing remains an open acceptance gate.
Full scene ground/expiry/creator ownership variants also remain separate from
the controlled corpus. The audit deliberately does not migrate unreachable
legacy helpers or invent source policy based on a physical kind alone.
