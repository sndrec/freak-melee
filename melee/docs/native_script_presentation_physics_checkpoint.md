# Interpreted presentation and physics observations

This batch adds three general operations used by the remaining fighter ports:

| Script API | Native behavior |
| --- | --- |
| `bones.root.rotation_y` | Read raw root part 0's Euler Y angle, or its Euler copy when the rendered joint uses a quaternion. |
| `physics.animation_vertical_velocity()` | Apply only TransN's Y translation delta to self velocity. No equipment multiplier or horizontal change. |
| `reflector.hit_direction` | Read the engine's last reflector-contact direction without changing Fighter data. |

There is no fighter-specific intrinsic, new actor storage, runtime allocation
or fallback to a stock special handler. The operations use the existing owned
rig, movement and guard bindings. Contact direction is read-only; root rotation
reads reject unavailable joints and invalid quaternion-copy layouts.

## Naming and source corrections

The original `ftPartGetRotZ` reads **Y**, using `HSD_JObjGetRotationY` in both
branches. It is now `ftPartGetRotY`; native callers, the reference module field
and authored Fox/Ness scripts use the correct name directly. The function body
and native turn calculations are unchanged. No misleading Z alias remains.

The earlier Ness Magnet comparison stub incorrectly returned `.rotate.z` for
that getter. It now returns `.rotate.y`, and its setup and observation label
match. This corrects an oracle assumption; older generated-C Magnet comparisons
are not proof of the newly interpreted port's gameplay acceptance.

Fox/Falco's grounded side-dash script now calls the existing
`physics.ground_animation_velocity()` binding, which runs the same
`ft_80085088` path as stock. The reference exporter and documentation use that
same script name. Fire rebound rotation and wall-impact angle locals initialize
as Floats rather than Ints, allowing their subsequent native Float calculations.

## Grouped verification

| Cohort | Current result | Evidence |
| --- | --- | --- |
| Root rotation callbacks/observations | 180 checks, 20 native comparisons | `A:/FreakMelee-test-archives/script-presentation-physics-root-oct10-b/receipt.json` |
| Air physics | 4,295 checks, 86 cases, 368 native pairs, 183 source invocations | `A:/FreakMelee-test-archives/script-presentation-physics-air-oct10-b/receipt.json` |
| Guard/contact observations | 17,692 checks, 2,994 cases, 2,767 native pairs, 2,872 VM calls | `A:/FreakMelee-test-archives/script-presentation-physics-guard-oct10-a/receipt.json` |
| Owned Fox/Falco body audit | Metadata passes; 256/254 bodies visited; 27 failures per form | `A:/FreakMelee-test-archives/fox-falco-presentation-physics-oct10-a/receipt.json` |

Root-Y comparisons use the original getter body at `1b8671834`, changing only
its function name. Both storage branches deliberately have different Y and Z
values and a semantic mapping different from raw part 0. Reads retain signed
zero, infinities and NaN bits; stale/wrong-class sources, missing joints and
quaternion-only copies reject without replacing output. Turns preserve unrelated
axes and joints. Top-level property routing is controlled in this fixture and
enters the actual presentation reader; catalog routing also receives build and
owned-source preparation checks.

Fourteen new plain-velocity comparisons use `ft_800851C0` pinned at `7b077ac85`.
They cover both Item-pointer states with Bunny Hood data absent, preserve all
velocity components and Float bits, and reject an extra argument. The existing
adjusted-velocity and landing cohorts remain passing. Unselected bounded-drift,
capture and ECB operations are explicit aborting fixture leaves.

Reflector direction reads execute through a compiled owned helper and the
actual guard observation binding. Float bits, read-only behavior, unchanged
Fighter data and four invalid-context cases are checked. Projectile collision
and full special-move decisions remain outside this fixture.

The first air/root runs failed to link stale fixture dependencies. The final
fixtures include current source-motion/animation metadata units and mark
unselected operations as unavailable. These initial failures are retained in
their `-a` folders and are not counted as passing evidence. Root fixture rig
ownership is controlled; unrelated class and owned-layout paths abort if entered.

Seven Windows targets build, and `native_fighter_script` /
`native_fighter_parameters` CTests pass. Build and CTest logs, executable hashes
and independent rehashes of 1,711 / 1,724 / 1,753 fixture pins plus 19 audit pins
are recorded in
`A:/FreakMelee-test-archives/presentation-physics-independent-oct10/receipt.json`
(SHA-256 `2edb697fa99b3b9acf494aece37f337da3e9636b6383ec575aa7dd451fdf6001`).

Neither Fox nor Falco is admitted yet. Each body still reports only its first
error; resolving one can expose another in that body. Their remaining groups
include article factories/signals, effect construction, radio-taunt stage calls
and item/receiver operations. This is native-operation and preparation evidence,
not a complete match, rendering or fighter-parity claim. No game windows, normal
installed packages or user drafts were touched. Full fighter and compact rollback
acceptance remain open; the goal remains active.
