# Direct interpreter: Item responses and attachment identity

This batch provides three generic Article operations used by Peach and other
fighter packages. Authored scripts retain their own state transitions and
removal decisions. The operations call shared native Item kernels directly;
they do not delegate to stock fighter handlers.

| Operation | Native behavior |
| --- | --- |
| `item.bounce()` | `itColl_BounceOffVictim`: common horizontal multiplier and vertical multiplier/addition. Void result. |
| `item.respond_stage_mask(mask)` | Whole `it_80276FC4` composition: velocity bounce, contact/ECB correction, conditional sound, effect and outgoing-damage attenuation. Integral 32-bit mask, Void result. |
| `item.attachment_owner_is(other)` | Physical identity comparison against the live revocable attachment relation. Object input, Bool result. |

See the [Item reference](scripting/item.md#itembounce) for author-facing
signatures and examples. The stage response preserves false-branch position and
collision-box writes as well as the successful presentation/damage tail. Its
preflight examines only the tail decision; it does not run the mutating native
contact helper twice or reconstruct the native sequence from separate calls.

## Identity and continuation

Item operations retain their physical pool identity, generation, native userdata,
Article/common attributes, model and owned prepared program/storage layout.
Stage presentation admits the actual native JObj required by the selected path.
Enabled damage capsules retain the current damage owner and validate otherwise
undefined Float-to-U32 damage conversions before calling the whole kernel. Native
damage setters still perform scaling and bookkeeping. Inactive/suppressed paths
do not require resources they never access.

The comparison name replaces the historical `item.creator_is` token everywhere
in tracked canonical/reference scripts. The reference emitter actually compared
`attachment_owner`, so calling this immutable creator provenance was incorrect.
Creator fields and queries retain their separate meaning. There is no old-name
alias or format compatibility layer.

Separately captured Object records referring to the same physical Fighter still
match. Valid null, Item/Effect or unrelated Fighter targets return false;
malformed/stale supplied tokens fail. Stock or foreign Fighters need no private
script program for this identity query. A captured target or attachment relation
is checked again before publishing; a recycled physical slot is not reacquired.

A live `Destroyed` hook can use these operations while its storage and captured
native identity remain valid. Setting the removal flag alone does not retire
that view. Storage retirement after the hook, faults or substituted live identity
prevent publication and subsequent script writes.

## Grouped evidence

Native client/content-tool builds pass. All 40 selected integration CTests and
six reference/compiler CTests pass. The older Item primitive fixture explicitly
aborts if it reaches the newly added response leaves; it is not evidence for
their behavior. The dedicated connected fixture supplies that evidence.

The fixture compares actual victim and masked-stage response composition against
the pre-batch native source. It executes the full source frontend, native catalog,
Item/Fighter actors, receiver export, Execution, VM, storage and physical GObj
pools together.

`A:/FreakMelee-test-archives/script-item-responses-final/receipt.json` passes
4,421 checks in 653 cases with 27 actual VM calls. Its 603 native comparison
pairs cover 25 victim-velocity combinations, 576 environment/previous-contact/
mask combinations and two foreign/non-Fighter damage-owner cases. Comparisons
check velocity, position, ECB state, hitbox damage and side-effect order. The
reference response bodies are pinned to pre-batch commit `bd6388eda4713850a66ecb2da9e46e1d20b7b5a1`.

The lifetime cohort exercises physical retirement/reuse, actor faults, changed
userdata/class/generation, model/parameter/common-data substitution, owner
invalidation and withheld output/tail writes. It includes distinct receiver
records for one physical target and a healthy removal-window comparison followed
by rejection after actual `Destroyed` storage retirement. All five hidden runner
stages exit successfully; root independently verified the recorded input hashes.

Package/Article/model graphs, selected runtime package and physical userdata are
controlled. Stage geometry speed/ECB setup, sound/effect creation and stale-move
factor are recorded leaves. Native response/vector bodies, the hitbox setter and
owner-scale arithmetic are actual source. Explicit post-kernel/getter observers
exercise continuation guards; they do not imply these pure native kernels
normally dispatch authored callbacks. DAT import, production constructors,
native callback scheduling, rendering and whole-match snapshots remain outside
this fixture.

## Preparation and remaining scope

`A:/FreakMelee-test-archives/source-item-responses-census-oct09-final2/receipt.json`
pins the rebuilt content tool and ten exact full-template inputs. Peach now
passes the Item-response group and reaches `guard.create` at line726. Mario,
Dr. Mario, Game & Watch and Zelda still prepare. Link/Young Link, Sheik and
Popo/Nana retain the secondary-resource metadata boundaries.

The [guard plan](native_source_guard_api_plan.md) groups the constructor, owned
ShieldHit helper, fields and callback continuation. The
[remaining Peach audit](native_source_peach_remaining_api_audit.md) lists later
input, collision, animation, observation, contact and effect groups. Secondary
graph ownership remains covered by the existing resource plan.

Preparation and controlled connected fixtures do not establish complete fighter
gameplay, rendering or whole-match rollback acceptance. No game was launched for
this batch. The project goal remains active; commits stay local.
