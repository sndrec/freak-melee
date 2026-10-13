# Script-owned attached articles

Dr. Mario's taunt is the first stock cloneability case that needs a fighter
script and an item script to share ownership of one temporary article. The
stock taunt animation in `ftdrmarioappeals.c` chooses a vitamin color, spawns an
attached pill, saves its `Item_GObj*` in `Fighter.u.mr.x2240`, and installs
damage/death cleanup callbacks. The pill's state-1 animation callback in
`itdrmariopill.c` calls back into Dr. Mario C to check taunt motion, read
`cmd_vars[1]`, and clear the saved pointer on destruction. Merely scripting
the taunt's spawn would leave fighter-specific decisions in that item callback
and could leave an invalid saved pointer. The old partial clone kept neutral
special and inherited taunt vitamin histories in separate fields, causing a
visible color mismatch on the second pill; the authored taunt now shares the
module-owned history with neutral special.

Use a per-item number as the authored handle. `Item.x1C` receives a new number
at item initialization. Store that number in an integer field in the fighter's
rollback-owned `native_module_state`, not an `Item_GObj*`. Host operations that
inspect or remove the item must resolve the number against the live item GObj
list and verify the requesting fighter still owns it. A destroyed article then
fails lookup rather than leaving a dangling pointer. A reflected article also
ceases to be owned by the original fighter; cleanup must not delete someone
else's reflected item. The scan happens only when a script requests an article
operation, not on every simulation tick. A match-created item number need not
be a persistent package identifier.

The generic host surface should create a stock-DAT article with its authored
logic table, return its item number, query whether that number remains owned,
and remove it if it does. Attachment setup needs an explicit spawn joint and
follow target; the item script must own its state animation, command-driven
visibility, scaling, motion/owner checks, detachment and teardown. Item
scripts also need reusable operations for stock item gravity, collision,
bounce, hit effects, texture selection and life timers. No host operation
should call `ftDr_Init_*` or the stock Dr. Mario pill callbacks. Native
projectile and attached pills may share one item logic declaration because
they occupy different item states, but the generated module must supply all
seven state callbacks before it replaces the donor table.

The query and removal half of that contract is implemented as
`articles.owned(number)` and `articles.remove_owned(number)` in fighter scripts.
The host resolves `Item.x1C` through the live item list and checks `Item.owner`
before reporting or removing it. An isolated generated-module test exercises
both script operations and rejects hosts missing either function. The authored
pill-state behavior is now in the Dr. Mario template. Explicit item-destruction
and other interaction recordings remain open.

Authored article scratch values now occupy `Item.xDD4_itemVar.native_article`.
They no longer alias the holdable-item scratch layout. Its first fields reserve
the original attachment owner and facing; the eight script `item.stateN`
values follow them. The item engine zeroes these values before an authored
article's `on spawned` callback. A generated article test writes `item.state0`
and verifies the attachment fields survive.

`articles.spawn_attached(kind, bone, variant, appearance_state, initial_state,
active_state, lifetime)` is now a generic fighter-script expression returning
the item's creation number or -1 on failure. It requires an authored logic
table and an owned article asset. The host creates the item using the stock
item constructor, applies the requested appearance and states, and follows the
fighter's origin while ownership remains unchanged. The seven arguments are
explicit because an attached item may use a different source joint, material
frame and item-state pair from Dr. Mario's pill. The generated-module test
checks argument dispatch and missing-host rejection. A recorded Dr. Mario
taunt now spawns an authored attached pill and matches stock item traces.

`articles.spawn_bone_attached(kind, bone)` is the separate path for an item
whose model must stay attached to the specified fighter bone, such as Cape.
It returns the creation number for later ownership-checked lookup and removal.
The host resolves the named fighter part, uses the normal item constructor and
attachment operation, and records the original owner for authored item
callbacks. A generated-module test covers dispatch and the required host
operation. This primitive does not itself replace Cape's stock item callbacks;
those still need an authored article and a stock comparison.

Authored item callbacks can now read `item.attachment_active`, `owner.motion`
and `owner.command0` through `owner.command3`. The owner reads return -1 or 0
when the original attachment no longer owns the item, so item scripts can
decide whether to detach without looking into a fighter-specific struct.
They can also read/write `item.command0` through `item.command3`,
`item.collision_mode`, and `item.world_interaction`. These are direct item
fields; the latter controls the engine flag used for ordinary world-item
interaction and automatic lifetime ticking. A generated-module test exercises
the owner guard, command path, and field writes. Item scripts can also invoke
common stage collision, test the engine's stage-bounce result, and play an
item sound with explicit ID, pan and volume. The generated module checks that
the host supplies every item operation it uses. An authored article
can now read the guarded owner's model scale, set its own uniform scale, and
toggle its child model's visibility. The Dr. Mario template now authors all
seven pill states; only recorded taunt/neutral paths have been compared live.

The next projectile boundary must preserve the original constructor order.
`Item_80268B18` invokes the article's `spawned` callback while constructing
the item, before the stock Dr. Mario helper sets its appearance, initial
state, launch velocity and lifetime. A generic authored projectile spawn
therefore needs explicit post-construction setup; putting all launch work in
`on spawned` would execute it at the wrong point. The common first article
attribute floats describe speed, angle and lifetime for Mario's fireball and
Dr. Mario's pill, but a general authoring format should name and validate
these parameters rather than assume every article shares that layout.
The article script can now declare a named float at an aligned byte offset,
such as `attribute float launch_speed 0`, and read it as
`item.attr.launch_speed`. The compiler rejects duplicate offsets or names and
unknown reads. This establishes readable names for imported DAT attributes;
the declaration's offset still needs to be checked against the actual asset
extent during import. An isolated generated callback reads the declared values
from an article fixture. The neutral special now uses the authored path.
A declaration can now bind named speed, angle and lifetime parameters to an
authored projectile setup. `articles.spawn_authored_projectile(kind, bone,
variant)` calls the shared constructor, then applies the declared velocity,
lifetime, item state and appearance in the same order as the stock pill spawn.
The generated-module fixture checks the descriptor and call dispatch. The
Dr. Mario template uses this path, and its recorded taunt-then-neutral replay
matches stock with authored projectile construction. The importer does not
yet validate declared parameter offsets against the DAT allocation.
An article interaction callback can invoke the common `item.interact(other)`
operation. Article callbacks can also read `owner.root_scale_y`; this returns
the owner's root Y scale while the original attachment remains valid and 1
after detachment. Both paths have generated-module fixture coverage, including
rejection when an operation required by the script is missing from the host.

Dr. Mario's taunt animation and damage/death callbacks now use authored
fighter actions, and taunt and neutral special share two vitamin-history
fields. The seven-state pill table has authored pickup, reflection, collision,
damage, clank, shield and absorption callbacks. The recorded taunt path
matches stock, but interruption, command-2 removal, self-destruction and
projectile contact still need dedicated same-input recordings.

Acceptance requires the current first-neutral and taunt-then-neutral replay
comparisons to match stock, including RNG and item texture choice, plus new
same-input recordings for taunt completion, taunt interrupted by damage,
attached-pill self-destruction, and projectile reflection or collision. An
isolated module test must show that a removed item number cannot resolve to a
later item and that reflected ownership is rejected. A registration test or a
matching neutral-special replay alone is not evidence of full Dr. Mario pill
ownership.

### Stock pill callback parity ledger

The decompiled `itdrmariopill.c` is the reference for this translation. State
0 is the flying projectile: its animation callback clears world interaction,
decrements the lifetime, and removes the pill at zero; physics applies the
item's fall speed/max speed and stock spin update; collision invokes common
stage collision, then bounce, removes the pill below the article's speed
threshold, or plays sound `0x15FAE`. Preserve the callback order and the
threshold's numerical behavior in stock-versus-script comparisons.
The item expression language now has a general `sqrt` operation for authored
bounce-speed tests, with generated-module coverage. The stock pill uses its
own reciprocal-square-root approximation, so matching the exact removal
threshold still requires a same-input boundary comparison.

States 1 and 2 are the attached taunt pill. Their animation callback clears
world interaction, shows the first child only when item command 0 equals 1,
and reads owner command 1 while the owner remains in taunt motion 341/342.
It scales from the owner's fighter model scale. If the taunt has ended, its
tracked item has been cleared, or ownership changed, it clears the original
owner's taunt callbacks/tracking and removes the pill. This requires a generic
way for an authored article to read and clear its owning module's tracked
article state; a mere motion-ID test would leave stale tracking. Their physics
and collision callbacks also set collision mode 2 and clear world interaction.
The fighter compiler and article parser now share the same state declaration
layout. An article can read or assign `owner.state.<field>` by its compile-time
offset in the original owner's rollback-tracked module-state bytes. Reads
return zero and writes do nothing after attachment ownership is lost.
`item.creation_id` can be compared with a tracked article handle before
mutating it, and `owner.clear_damage_death()` clears the original owner's
damage/death callbacks while that owner is still valid. The generated-module
fixture checks int, bool and float reads/writes, stale attachment rejection,
and callback cleanup. The Dr. Mario article uses these operations; the
recorded taunt/neutral path matches stock, while cleanup branches remain open.

States 3 through 6 are the held/picked-up states (animation indices 1, 3, 4,
5). Their animation callback clears world interaction and copies the owner's
root Y scale only while the owner is in DeadUpFallHitCameraIce or Sleep;
physics and collision also clear world interaction. The current guarded
`owner.root_scale_y` read is useful for attached ownership, but it does not
model this held-item owner condition. Article scripts now also have
`holder.motion` and `holder.root_scale_y`, which inspect the current item
owner only when it is a fighter, independently of the original attachment.
`holder.incapacitated` invokes the common fighter predicate used by stock
held-pill code for DeadUpFallHitCameraIce and Sleep; it is false without a
fighter holder. This avoids baking Dr. Mario's motion IDs into the item
language.
The fixture covers an attached pill, a detached pill still held by that
fighter, and an ownerless pill. The picked-up callback resets life
from article attributes and enters state 6. State 0 and held states therefore
need distinct scale logic; applying the attached-ownership guard everywhere
would change stock behavior.

Damage, clank, shield-hit, shield-bounce, and absorption callbacks play
`0x15FAE` and return true; reflection calls the common reflector operation.
The interaction callback delegates to the common item interaction operation.
Taunt and neutral-special creation must preserve their different post-spawn
state, appearance, command, owner, and accessory order. The authored article
is compiled in the Dr. Mario template and the existing 520-sample taunt and
taunt-then-neutral recordings match stock: 1,038 fighter states, 114/154 item
states respectively, and 519 RNG states each. Eight seed variants of the
taunt-then-neutral recording also match, including texture selection. The
authored projectile constructor was then enabled and seeds 1 and 8 matched
again. This establishes those recorded paths, not the unrecorded callbacks
or exact behavior at the bounce-speed threshold.
An additional recorded path interrupts taunt with Zelda's jab. The clone
matches stock over 520 inputs, including 1,038 fighter states, 29 actions,
24 item states, four damage events and 519 RNG states. This exercises one
damage cleanup timing. The normal-taunt recording also reaches command 2:
its attached pill is present at sample 180 and absent at sample 181 while
Dr. Mario remains in motion 341, matching stock. A separate recording brings
Zelda close enough to take an 8% neutral-special pill hit at sample 284;
stock and clone match over 520 inputs, including 79 item states and one
damage event. The matching shield recording replaces that damage with Zelda's
shield-hit motion 181 at sample 284; all 520 inputs, 79 item states and 519
RNG states still match. A Mario Cape recording now reflects a pill: at sample
292 the pill changes owner from P1 to P2 and its horizontal velocity reverses.
The authored clone matches stock over 520 inputs, 152 item states and 519 RNG
states on that path. Another recording fires opposing Dr. Mario pills. Both
invoke the item clank callback at sample 289 and disappear without fighter
damage; stock and clone match across 520 inputs, 89 item states, both clank
events and 519 RNG states. Pickup, other contacts and the bounce threshold
still need targeted parity cases.
The two taunt motion states also now supply scripted interrupt, physics and
collision callbacks. Their shared engine operations preserve the original
common taunt input order, ground friction and ground-nudge collision behavior,
so the custom fighter no longer calls the Dr. Mario taunt wrappers. The four
recorded paths above still match after this replacement.

Dr. Mario's Cape/Sheet article is now script-owned too. The generic host can
spawn an article on a chosen fighter bone, track its creation ID, pause and
resume only that owner's live article during hitlag, activate the attachment,
and spawn an effect on one of its validated dynamic bones. The item script
clears its command variables on pickup, runs both Cape states and their two
effects, and removes the article when the owner leaves either side-special
motion. Fighter damage and death callbacks remove the tracked article. The
module fixture covers these host operations and rejects missing hosts.
Seven recorded stock-versus-clone runs now match: grounded and airborne Cape,
their respective contact paths, interrupted taunt, pill contact, and death
followed by respawn. Each run contains 520 controller samples. The airborne
Cape recording exposed a state discrepancy: stock initializes the attached
article in state 0 on that path, so the script does the same. These recordings
do not establish every Cape reflection, hitlag, or destruction branch.
Two more recorded paths cover separate branches. Zelda's jab interrupts Cape
at sample 196 after its attached article has existed for seven samples; stock
and clone both remove that article on the damage sample. A Dr. Mario pill
from P2 reaches P1's Cape and transfers ownership to P1 at sample 292; stock
and clone match the pill's item state and velocity afterward. The checked-in
inputs and generic removal/owner-transfer assertions make both paths
repeatable, while other projectiles and cleanup orders remain open.
