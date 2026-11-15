# Immutable fighter hitbox creation checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Fighter scripts can declare root `hitbox_definition Name:` blocks and call
[`hitbox.create(slot, Name)`](scripting/fighter_hitboxes.md#creating-a-hitbox).
The definition owns semantic fields rather than packed GameCube words. The
compiler validates required fields, exact integers, finite geometry, names
and a closed definition selector. Its fixed 64-entry bank requires no tick
allocation. Existing pose selectors share the same bounded selector rules.

The backend initializes complete native capsules using the common damage,
angle and group/victim-history primitives. Active capsules refreshed in the
same group preserve native state and history. Owned rig joints and declared
live article reservations are supported. Reservation lookup retains the
actual resource, attachment, graph and pool identities; stale or replaced
endpoints are not published. Existing resource detachment clears and disables
both ordinary and thrown capsules borrowing that graph.

Samus's authoring components now own modified-ground attack creation and the
mode-1-to-2 transition in extending, falling and slack updates. Their root
fighter helper clears capsules before testing the attack press, as native
behavior does, and creates the semantic endpoint definition at reserved
part139. Flight applies this decision before checking homing. No script calls
a native Samus special handler or the native grapple initializer.

## Evidence and limits

Connected receipt:
`A:/FreakMelee-test-archives/hitbox-create-oct11-g/receipt.json`.
The complete target cohort passes 19,897 checks, 1,451 cases, 1,640 native
comparisons and 1,085 VM calls. The new slice contributes 128 creation
comparisons against the corrected native descriptor initializer, with all
capsule bytes, cleanup marker and joint compared across four slots and eight
activation/group configurations, directly and through Fighter VM helpers.
Compiler negatives and real pool/resource identity rejection and detachment
are covered. New metadata and creation units pass strict warning checks.

Damage/angle helpers are controlled observers here. Graph construction and
payload removal are fixture leaves; resource binding, capture, lookup and
borrower detachment are actual source bodies. These comparisons do not prove
retail graph construction, release staleness behavior, collision damage or
rendered gameplay.

Samus composition receipt:
`A:/FreakMelee-test-archives/samus-hitbox-source-oct11-a/receipt.json`.
Its source audit prepares all 280 executable bodies and reports only the
deliberately absent independent registration declaration. Existing geometry
and lifecycle/sequence/hang comparisons pass. This audit checks the new
helper/call wiring; it does not execute that entire modified-ground flow with
a retail grapple graph.

Seven native targets build, and the focused `native_fighter_script` and
`native_fighter_parameters` CTests pass. No game was launched or installed
content changed during this checkpoint.

Samus remains an unregistered authoring candidate. Throw-accessory recipes,
the correctly placed post-common throw presentation event, complete
constructor/solver/lifecycle closure and gameplay acceptance remain open.
This checkpoint does not claim full Samus or project completion.
