# Ground input arbitration

These Fighter calls each perform one shared native input check and return its
Boolean decision:

```text
controls.try_side_smash()   controls.try_up_smash()   controls.try_down_smash()
controls.try_side_tilt()    controls.try_up_tilt()    controls.try_down_tilt()
controls.try_jab()         controls.try_ground_jump()
controls.try_dash()        controls.try_crouch()
controls.try_turn()        controls.try_walk()        controls.try_guard()
controls.try_catch()       controls.try_taunt()
controls.try_side_special()      controls.try_up_special()
controls.try_neutral_special()   controls.try_down_special()
controls.try_heavy_item_throw()
```

The script chooses the priority by calling them in order and returning after a
successful decision. Checks retain native held-item arbitration, c-stick profile
and tap-jump behavior. They can enter a motion or invoke an owned authored entry
event; they are source-only mutating calls, even when the decision is false.
They require the live selected source program, owned motion layout and common
input data. A fault or retirement terminates the caller without publishing a
Boolean result or continuing its authored statements.

`controls.try_heavy_item_throw()` attempts the common carrying-state throw
input. It chooses the forward/back/up/down ordinary or smash throw from the
current main-stick/button input or c-stick input, using the engine's thresholds
and cached input ages. With no held item or no selected motion change it returns
false. A successful entry clears command/throw cues, computes animation speed
from the item's damage multiplier, and installs the common throw bookkeeping.

The [`heavy_throw_enter`](fighter.md#on-heavy_throw_enter) event then runs from
the attached source's prepared program. It can replace the movement callbacks
with the fighter's own scripted helpers. A source actor never consults a stock
fighter Definition for this event. Without an override the common carrying
policy is retained. Retained Fighter/program checks after motion initialization,
animation initialization and the authored event stop stale entry continuation.
The initial accessory/release-cue check remains part of common item handling.

The four ground-special checks use cached input ages being zero: side `x688`,
up `x686`, neutral `x689` and down `x687`. They synchronously invoke the source's
own `special_s`, `special_hi`, `special_n` and `special_lw` move-entry hooks.
The native input-counter update derives those ages before arbitration; these
calls do not test a fresh raw B press. Side special also retains the native
facing threshold and ground momentum adjustment, including source friction
policy and floor-surface friction.

The current ground kernels report true for an attached source with a zero age
even when its corresponding hook is absent. That entry is a no-op, and side
special can still reverse facing and adjust momentum. Provide the intended
hooks when exposing these checks. This preserved ground behavior differs from
`controls.try_air_special()`, which tests hook presence before changing facing.
See [ground_special_controls.md](ground_special_controls.md) for kernel mapping
and the source-lifetime contract.

`controls.try_guard()` returns Bool from the native common guard input check
`ftCo_80091A4C`. A pressed L or R with `trigger_analog_timer` strictly less than
`powershield_input_window` enters the common reflect-start motion first,
including when shield health is zero. Otherwise the native derived held
`HSD_PAD_LR` input bit and nonzero shield health enter the common guard-start
motion. A missed input returns false; the script decides its priority among
other ground checks.

Ordinary source packages have no callback-bearing native Fighter definition,
so this operation follows the generic common guard setup. It does not dispatch
an authored `shield_start` or `shield_transition` entry or select a donor
character's shield handler. Common setup retains the native shield geometry,
flags, animation and effect behavior. The binding keeps the existing exact
source/program/storage/lifetime admission and validates again before returning.
Common motion changes can run `grounded_motion_setup` and a bound state-change
helper. The native guard setup bodies remain unchanged: this binding's final
check withholds its result after a retired entry, but does not add guards inside
that native setup or roll back its completed mutations.
The console fixture controls common entry setup and tests input selection; it
does not establish complete shield behavior with imported assets.

`controls.try_catch()` preserves `ftCo_Catch_CheckInput`: it attempts the
native held-item throw first, then catch eligibility, then a common Catch
entry when the derived held `HSD_PAD_LR` bit and pressed A are both set. A
handled item throw also returns true. Catch setup, item classification and
motion delivery retain their existing native contracts.

Ordinary source packages dispatch authored `catch_allowed`, `catch_start`,
`catch_frame` and later [catch policy hooks](catch_hooks.md) through their own
program. Stock Link/Samus eligibility helpers inspect stock-kind union fields;
a source actor is not assigned those donor kinds. The input check and policy
dispatch do not establish complete Link/Young Link tether Article behavior.

`controls.try_taunt()` preserves the D-pad Up check `ftCo_800DE9D8`. A matching
press synchronously runs the source's `on taunt` hook. If absent, the native
source branch enters common AppealSR/AppealSL motions rather than selecting a
donor or debug taunt handler. Healthy entry then delivers the native taunt bonus
notification. Fault or retirement withholds the result and skips that bonus;
completed mutations are not rolled back.

`state.crouch_wait()` enters the common stationary crouch through
`ftCo_800D638C`. It returns Void and preserves the common setup followed by the
owned `crouch_wait_enter` notification when declared.

`stats.landing_air_neutral_lag` and `stats.landing_air_back_lag` read the exact
F32 `co_attrs.landingairn_lag` and `landingairb_lag`. They do not clamp or clean
nonfinite values. Read-only foreign Fighter receivers may observe their own
data through the retained physical/source lifetime checks.

The common up/down-smash checks use their common motions and prepared phases
unless the fighter declares `up_smash_enter` or `down_smash_enter`. These
[complete entry hooks](common_policies.md#complete-smash-entry) replace entry
after ordinary input acceptance and held-item throw priority. They have no event
arguments and return Void; the script owns initialization and motion selection.
Without them, an attached script uses common entry rather than stock
fighter-specific dispatch. Both ordinary and NoD0 up-smash checks support the
replacement. See the [dispatch checkpoint](../native_script_ness_smash_hook_checkpoint.md)
for tested boundaries and the remaining Ness gameplay work.

The grouped console fixture lives in tests/native/script_ground_controls.
Its controlled engine leaves and lack of full gameplay acceptance are recorded
in its README and receipt.

## Heavy-item policies

`on heavy_pickup_finish:` returns Bool when a heavy pickup animation finishes
with an item selected. True means the script handled completion; false, or an
absent hook, enters common heavy-item waiting. This hook does not create or
attach the item; it runs after the ordinary pickup work. A light pickup follows
its existing separate path.

`on heavy_item_damage:` returns Void on the heavy-held-item branch of the
common damage reaction. It replaces the fighter-specific response on that
branch; use ordinary script state, motion and item operations to implement it.
It is not a general notification for every hit and does not replace the damage
or knockback calculation. An absent hook preserves the existing branch policy.

Both callbacks retain the executing actor/program/storage lifetime. A source
fault or retired actor stops the cached native continuation. The damage branch
also keeps its existing retained victim check. Completed mutations are not
undone. The grouped [catch/throw fixture](../../tests/native/script_catch_policy/README.md)
compares these decision boundaries, not the entire damage engine.
