# Young Link Milk: authored article source batch

Milk remains an unregistered source fixture. This batch implements a generic
selected-joint visibility property and translates the Milk article callbacks;
it does not activate an appeal override or claim complete Link/Young Link
behavior. The full fighter damage/death cleanup gate remains unchanged.

## Source decisions

`itclinkmilk.c` declares two states with animation IDs 0/1 and the same animation,
physics and collision callbacks. Pickup selects state 0 only when facing is
exactly 1; every other facing selects state 1. It then advances animation once.
Physics is empty, collision returns false, and interaction calls the existing
shared item interaction primitive. No fighter-specific Host delegate is added.

The constructor `it_802C8B28` uses the supplied fighter position, flattened onto
the gameplay plane, zero velocity and disabled initial collision. It clears
commands 3,2,1,0 and automatic bounds removal, saves its creator, then attaches
and runs pickup at the supplied joint. It does not register a normal held item.
The scratch attachment metadata is owner origin, plane on, initial collision
off and held off; the fighter supplies LeftThumb when eventually ported.
Young Link's owned article slot is 5, with fixed kind `It_Kind_CLink_Milk`; there
is no stock kind-attribute offset for this slot. No new model root or animation
bundle declaration is required by the article callbacks.

On every animation update, stock obtains the first child twice: the variables
named child and grandchild point to the same joint. It first changes that
joint's local hidden flag according to the **previous** item command 0 being
exactly 1. Creator checks and command updates happen afterward, preserving the
one-update visibility delay. A descendant or whole-subtree operation would
change the behavior.

The creator query `ftCl_Init_8014920C` expires Milk when creator/GObj data is
absent, creator motion is neither 342 nor 343, or the creator's Milk reference is
absent. On expiry, the fighter reference is cleared only when current owner
still equals creator. Creator and current owner are then cleared and animation
returns true for ordinary engine destruction. No immediate recursive destruction
is added to the animation callback.

Otherwise `ftCl_Init_801492F4` returns a bool conversion of creator command 1.
The script explicitly uses `creator.command1 != 0`, rather than copying arbitrary
command integers. The child scale receives `ftLib_800869D4`, whose actual getter
uses `instance_scale.y * co_attrs.model_scaling`. Creator command and scale are
read even if current owner differs. The existing equality-restricted `owner`
context therefore cannot substitute for `creator` on this branch.

The explicit removal helper `it_802C8C34` clears creator/current owner before
removing the article, and does not clear the fighter's Milk reference itself.
The scratch removal signal preserves that order. Fighter cleanup must clear its
ordinary Milk handle separately in stock order.

## Generic operations

`item.joint(model, "path").self_visible` uses joint-value channel 15 through the
existing Host read/write operations. Reads report the selected joint's local
JOBJ_HIDDEN flag, false when missing. Writes call single-joint
HSD_JObjSetFlags/ClearFlags. Descendant and sibling hidden flags are untouched;
stock matrix-dirty propagation inside those helpers is preserved. Existing
`.visible` keeps the SetFlagsAll/ClearFlagsAll subtree traversal, which stops at
instance joints and does not visit the selected joint's own siblings. Neither
read reports effective visibility through hidden ancestors.

Approved creator reads use the existing validated original-creator context:
`creator.command0..3` reads the corresponding integer command, or 0 when absent;
`creator.model_scale` calls the existing fighter model-scale Host operation,
or 1 when absent, matching `owner.model_scale`. Milk explicitly checks
`creator.exists` before these operations. Lifetime and exact immutable author
Definition validation remain the generic creator backend's responsibility.
There are no new Host fields or fighter-specific lookup operations in this batch.

## Fixtures and current acceptance

`tests/native/link_milk_fixture.fms` owns the ordinary fighter Milk handle, its
clear helper, two article states, constructor initialization, pickup, animation,
physics, collision, interaction and explicit removal signal. It remains outside
all fighter registry/import defaults.

`link_milk_fixture.py` extracts the whole unchanged stock Milk translation unit,
the three original creator queries, actual ftCommon/ftLib scale bodies, actual
HSD flag/dirty bodies and actual backend joint functions, alongside real compiler
output. `fighter_link_milk_test.c` compares both sides across both states,
creator motions around 342/343, absent/present references, absent/current/transferred
owners, absent creator data, arbitrary command values, scales and hidden/classical
scale flags. It compares final graph flags and scale, expiry, references, item
commands and external callback order. Constructor allocation/attachment and
interaction are shared primitive stubs; callback initialization and descriptor
semantics are compared, not claimed as a full live engine constructor test.

The actual compiler output passes **69,180 original-C/generated comparisons
with zero failures**: 69,120 animation configurations and 60 focused
pickup/physics/collision/interaction/removal/constructor comparisons. All three
required-Host rejection checks pass (joint setter, creator lookup and model scale).
The source hashes and exact scope are saved in
`.local/link-milk-source/acceptance.json`. No generated C is hand-edited to
simulate an unsupported property.

`article_joint_visibility_test.c` compares 32 selected-joint/subtree graph
configurations with the real HSD helpers, including hidden parents, selected
siblings, descendants, instance joints and classical-scale matrix flags. It also
covers model slots, dynamic joints, missing paths and non-finite writes. An
isolated executable using unchanged real HSD/backend bodies passes. The fully
linked `native_model_runtime_test` also passes with this graph test, and root's
coordinated generic7 build and 66 focused checks pass.

## Remaining fighter gate

`ftclinkappeals.c` creates Milk only for command 1 == 1 with no existing Milk,
registers the full Link cleanup in damage/death2 after successful creation, and
uses the existing item cleanup path for command 1 == 2 and animation completion.
Its input/physics/collision common policy and appeal entry command reset still
need an authored fighter batch. The damage cleanup order remains Boomerang,
Hookshot, Arrow, Bow, then Milk; constructors and resource release for every
referenced article must close before activating that handler. Bow recipient
exports and generic callback lifetime hardening remain a separate proposed batch.
This bounded article comparison cannot remove those activation requirements.
