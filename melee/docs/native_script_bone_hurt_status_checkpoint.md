# Interpreted per-bone hurt status checkpoint

2026-10-10. `hurtboxes.set_bone(bone, status)` now runs in the ordinary
interpreter. Both arguments are native-width Int values; status is normal,
invincible or intangible. The raw joint index belongs to the owned source rig.
Semantic roles remain explicit through `bones.index(role)`.

The binding admits the physical actor, retained program/storage and imported
joint table before scanning bounded capsule storage. Missing/out-of-range bones
and empty or unmatched capsule lists are healthy no-ops. A matching bone calls
the original generic `ftColl_8007B128` setter. Only the first matching capsule
changes. Position-update flags survive, and enabling one capsule does not clear
the sticky aggregate hurt-status flag. Malformed types/widths/statuses or invalid
source/rig state reject before mutation and preserve the caller's result.
No fighter-specific API, allocator or placeholder joint was added.

## Consumer migration

Bowser's Fortress effect uses a Float slope accumulator rather than inferring
Int from zero. Attached article visibility uses the existing Bool property;
Bomb callbacks register named helpers and clear their own registration. Hitlag
callbacks use their existing explicit setters. Fortress completion uses
`state.wait()`: the original `ft_8008A2BC` takes exactly that neutral entry for
Bowser, with its boss-only branches inapplicable.

Donkey Kong's Hand Slap queues the existing generic deferred world-position
effect, preserving native `EF_SPAWN_POS` sampling rather than changing it to an
attached effect. Accessory behavior moved intact into named registered helpers.
Ground and air Headbutt helpers retain their entry-selected effect IDs and now
set the native effect-active latch. Floor-following and island output locals use
Float types. No action body was removed to make source preparation pass.

## Verification

The connected capture/source cohort passes **8,257 checks / 851 cases**.
Thirty-eight new cases include 18 whole-Fighter comparisons with the actual
native per-bone setter, duplicates, all three statuses, both initial aggregate
flags, no-op cases, compiled mask/clear helpers and invalid source/argument/rig
rejections. Owned rig assets remain controlled; no rendered shell/collision or
whole-fighter gameplay acceptance is claimed.

Receipt: `A:/FreakMelee-test-archives/script-bone-status-oct10-final/receipt.json`.
SHA-256: `9912b667fa83daa94e519dfc3e55e1c8e84d4db80b1381874e5a71a1919c527d`.
All 1,747 input pins were independently rehashed unchanged.

The broader combat fixture passes **37,048 checks / 12,240 RNG pairs / 30 native
body pairs**. Its stale duplicate attachment predicate was removed in favor of
the existing shared fixture provider. Native hurtbox initializer/setter bodies
are extracted; assertion/report leaves remain controlled. This fixture has no
owned source actor; positive ownership is covered by the connected cohort.

Receipt: `A:/FreakMelee-test-archives/script-combat-bone-status-oct10c/receipt.json`.
SHA-256: `b9c4e05d0e12553b70ffd991ad0e9f9f00f4c288383b20b1ad2cd71ae0b977a5`.
All 1,678 input pins were independently rehashed unchanged.

The client, content tool, family/package runtime and capture enclosure targets
build. The ten-source preparation cohort keeps Captain Falcon, Ganondorf,
Marth, Roy, Luigi and Yoshi passing. Bowser advances from its slope conversion
to `items.block_ground_pickup` at line 230; Donkey Kong advances from its effect
spawn to `stage.follow_floor` at line 193. Fox/Falco metadata/asset gates remain.

Source receipt: `A:/FreakMelee-test-archives/source-bone-status-oct10c/receipt.json`.
SHA-256: `449adc883f835a80b3b869799665e270cd7109016b6b9c58be66695031474370`.
All 27 source-cohort pins were independently rehashed unchanged.

No game/Dolphin launched, installed content changed or remote push occurred.
Remaining roster integration, full-fighter gameplay and compact whole-match
rollback remain unfinished; the overall goal remains active.
