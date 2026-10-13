# Interpreted CPU planner fields checkpoint

2026-10-10. Five existing generic CPU fields now have ordinary interpreter
catalog entries and execution paths:

| Property | Type | Access | Native field |
| --- | --- | --- | --- |
| `cpu.travel_frames` | Int | Read/write, owned source | `cpu.x90` |
| `cpu.selected_command` | Int | Read/write, owned source | `cpu.xA4` |
| `cpu.elapsed_frames` | Int | Read-only, owned source | `cpu.x7C` |
| `cpu.floor_in_bounds` | Bool | Read-only, owned source | `cpu.xFA_b5` |
| `cpu.body_half_width` | Float | Read-only, source or fighter receiver | `cpu.x564` |

The writes require exact native-width signed Int values. Reads preserve the
native field values; none of these operations probes terrain, changes another
planner field, increments a counter, allocates or draws randomness. Source-only
fields retain the owned actor/program/storage contract. Body width is an
observation and is readable through retained references to both stock and
other authored fighters, without exposing writable planner state.

## Verification

The connected CPU cohort passes **31,753 checks / 1,353 cases / 3,973 native
field/kernel pairs / 1,603 VM calls**, including 27 receiver VM calls. New cases
compare entire Fighter state for signed boundary reads/writes, compiled helper
execution and read-only rejection. Malformed tagged types and over-width Int
values reject without mutation. Body width covers the existing Float-bit
corpus through actual property/receiver routing and compiled article callbacks
on stock and foreign authored fighters. A stale recipient rejects without
publishing an output. Existing invalid-source/schema/storage cases include the
four new owned-source reads.

The command/selector cohort remains passing: **7,260 checks / 612 queue pairs /
80 selector pairs**. Five native targets build: client, content tool,
family/package runtime targets and capture enclosure target.

Receipt: `A:/FreakMelee-test-archives/script-cpu-planner-fields-oct10-final/receipt.json`.
SHA-256: `b5a16a8071cb71568e9f750c4722081a8c7fce0cb3c0c94aea868f80eaf47cea`.
All 1,727 pinned inputs were independently rehashed unchanged.

The retained-body nine-source cohort keeps Captain Falcon, Ganondorf, Marth,
Roy and Luigi preparation passing. Yoshi advances to `stage.floor_between`
on line 1613. The previous checkpoints called line 1574 a ranged-query gate;
the actual source at that line was `cpu.travel_frames`. Those descriptions
have been corrected. No template body was removed to make preparation pass.

Source receipt:
`A:/FreakMelee-test-archives/source-cpu-planner-fields-oct10-final/receipt.json`.
SHA-256: `9ab6d02d6d051c9c82ee4bf788aecfd8eee8473805f7b73861142b55e1c0ea6e`.
All 25 source-cohort pins were independently rechecked unchanged.

## Boundaries and remaining work

The fixtures use synthetic actor/package metadata and controlled planner
dependency leaves. Field comparisons establish exact values, isolated writes,
typed execution and receiver ownership, not full rendered CPU gameplay parity.
Yoshi's floor-only segment query, owned Article metadata/assets, remaining
roster integration and complete fighter acceptance remain open. The all-geometry
segment query is not a substitute for its floor-only threat check.

No game/Dolphin process launched, installed content changed or remote push
occurred. Compact whole-match snapshots and overall plan acceptance remain
unfinished; the goal remains active.
