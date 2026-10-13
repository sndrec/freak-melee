# Synchronous callback engine continuations

Batch 8 audits synchronous authored callback continuations. The engine
changes below pass the full native Release build and the 76-check focused
suite, including the direct engine-body fixture. No new gameplay comparison
is claimed by this ledger.

An engine callback may remove an item synchronously. Cached data cannot be used
after that callback without checking registry membership and the full creation
number. `nativeCallbackSource_Alive` scans the appropriate live object list before
reading user data. Pointer equality alone does not accept a recycled object.
These checks allocate no gameplay memory and preserve callback order and results.

## Implemented boundaries

| Production source | Retained continuations covered |
| --- | --- |
| `it/item.c` | Animated/collision/damage/reflection/shield predicates; accessory, pickup/drop/throw/air callbacks; constructor notification; nested enter/exit hitlag and outer damage processing; grab callbacks; removal cleanup |
| `ft/fighter.c` | Motion reset, load/death/tick/animation/map/physics/accessory callbacks; pre/post hitlag; damage/reflect/absorb/shield callbacks; grab callbacks; worn/held item drop/removal continuations |
| `ft/fighter_physics.c` | Physics and hitlag callbacks before movement/wind continuation |
| `ft/ftcommon.c` | Damage callback reset, extended item drop and item visibility continuations |
| `ft/ft_0D31.c` | Death callbacks, throw cleanup and held/worn item removal during death |
| `ftCommon/ftCo_Throw.c` | Throw animation, victim setup, common update and finish callbacks |
| `ftCommon/ftCo_CapturePulled.c` | Both retained fighters across damage reset/motion setup, collision callbacks and capture motion changes |
| `ftCommon/ftCo_Damage.c` | Retained source/victim during damage callbacks; item drop, motion changes and knockback enter/exit continuations |
| `it/it_2725.c` | Attachment release checks both retained item and owner after owner DropExt notification, before pose/collision updates |

Item removal initializes `native_removal_in_progress` to false at construction and
sets it before the first removal callback or cleanup. A recursive removal during
`destroyed` returns immediately. The outer removal retains the original cleanup
order; it checks identity before continuing if a native callback has independently
removed or recycled the object. Removal is immediate, not deferred.

Grab callbacks retain the original victim identity. After the first callback,
an unchanged victim address must still have its original creation number. A
changed address is checked with `nativeCallbackSource_Snapshot` before dereference,
so a legitimate redirect to a different live fighter proceeds. NULL, removed and
same-address recycled recipients stop the sequence.

Terminal callbacks without subsequent entity access do not need a redundant
check in their own dispatch body. Examples include terminal input/camera/accessory
callbacks and stage callback helpers `it_8026E248`/`it_8026E414`. Their generated
callers still check identity if they continue after the Host operation.

## Compiler binding audit

Confirmed callback-capable Host names include `reset_damage_callbacks`
(`ftCommon_8007DB58`), `change_motion_state`, `update_throw`, `item_remove`,
`item_release_attachment`, `fighter_item_pickup`, `fighter_item_drop`,
`notify_item_pickup`, `fighter_knockback_enter`, `fighter_knockback_exit`,
`begin_capture`, `apply_throw_damage`, `launch_thrown_victim`,
`item_land_on_stage` and `item_bounce_to_ground`. The compiler must check its
source after these operations, including operations nested inside expressions or
authored helpers. `item_set_airborne` binds direct `it_802762BC` flag assignment;
classifying it conservatively does not establish a synchronous callback there.
Constructors/signals, named dispatch depth and recipient exports have separate
backend fixtures; this engine audit does not substitute for those checks.

## Direct fixture

`tests/native/callback_engine_fixture.py` extracts actual production dispatch
bodies into the build header; it does not reproduce their guards in test code.
`callback_engine_lifetime_test.c` links the actual registry predicate backend and
uses engine stubs to observe continuation. It covers true/false predicate results
with removal and generation recycling, pickup/accessory/air transitions,
physics/hitlag inner and outer continuations, recursive destruction exactly once,
unchanged/redirected/stale/removed grab victims, and attachment release with
removed/recycled items or owners plus an ordinary live continuation. Joint
translation uses the actual inline joint operation.

The production-body fixture and scoped whitespace checks pass. Acceptance is
recorded in `.local/native-script-generic8-acceptance.json`; build4 and focused
checks logs preserve the coordinated result. No game, desktop input, renderer
or audio process is part of this fixture.

## Explicit audit limits

The scripting language exposes item removal, not arbitrary fighter destruction.
The checks above also defend assigned boundaries against native deletion, but
advanced native-C deletion from every stock helper is not a supported blanket
contract. In particular, external retained continuations after
`ftCommon_8007DB58` remain unaudited in `ft_0C8C.c`, `ft_0D27.c`,
`ftCo_800C78B0.c`, `ftCo_800C7590.c`, `ftCo_Barrel.c`, `ftCo_BarrelWait.c`,
`ftCo_Bury.c`, `ftCo_CaptureCaptain.c`, `ftCo_CaptureKirby.c`,
`ftCo_CaptureKoopa.c`, `ftCo_CaptureMewtwo.c`, `ftCo_CaptureYoshi.c`,
`ftCo_DamageBind.c`, `ftCo_DamageIce.c`, `ftCo_DamageScrew.c`,
`ftCo_DamageSong.c`, Kirby/Crazy Hand capture, Samus Bomb setup and Master Hand
tag cancellation. These files were deliberately not expanded into this batch.
The corresponding module Host continuation has separate backend checks.

Normal callback ordering and changed-victim behavior are preserved. Deliberate
differences apply only when continuing would access a removed/recycled entity or
recursively destroy the same item. No RNG adjustment or semantic digest masking
is used to accommodate undefined stock continuations.
