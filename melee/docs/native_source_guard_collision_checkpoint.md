# Interpreter guard, collision history and companion fields

Four generic operations are connected: `controls.try_guard()`,
`collision.copy_from(id)`, read-only `motion.allows_controller_sync`, and
read/write `spawn.horizontal_offset`. Fighter-specific choices remain scripts;
native common input/collision kernels are unchanged.

## Behavior and boundaries

- [Guard input](scripting/ground_controls.md) preserves reflect-start priority,
  the native analog-input window and held-input/shield-health decision. The
  fixture controls common motion setup. The final binding guard does not prove
  safe continuation inside unchanged native guard setup after an authored
  motion callback retires its owner; that engine boundary remains separate.
- [Collision history](scripting/collision_copy.md) directly copies the target's
  collision data into the caller with native mode 2. It preserves the caller's
  collision owner and non-copied fields. It does not move the Fighter, change
  velocity or set ground/air state. Missing IDs return false; full signed Int
  bits, stock targets, independent source targets and self-copy are supported.
  The existing live registry requires valid Fighter userdata, as documented.
- [Fighter fields](scripting/fighter_observations.md) read semantic `x2225_b3`
  and preserve Float bits in `x40`. Actual receiver routing supports stock and
  independently owned source actors; writes target the retained receiver.

## Verification

The client, content tool, source-assets and selected integration targets build.
All 16 selected CTests pass. One missing isolated link dependency was fixed with
an explicit unavailable collision leaf; that leaf rejects calls and is excluded
from the collision fixture that executes the real backend. No game, GPU or
audio device was opened.

| Grouped console fixture | Checks | Native comparisons | VM coverage |
| --- | ---: | ---: | --- |
| Ground controls, specials and guard | 87,721 | 4,453, including 3,000 guard pairs | 4,493 source invocations |
| Collision history | 19,073 | 524 | 12 actor calls |
| Fighter observations, spawn and synchronization flag | 8,190 | 1,545 | 742 calls, including 480 receivers |

Collision fixtures execute unchanged `mpCopyCollData` and legacy direction/
registry reference bodies, real compilation, program publication, actors,
storage and fixed GObj pools. Imported data, motion metadata and userdata are
controlled. Observation fixtures execute production Read and Write routing;
they do not establish spawn positioning or controller behavior in a match.
Guard fixtures test the native predicate and entry selectors with controlled
common-entry leaves, not complete shield behavior or arbitrary inner reentry.

Root independently rehashed these receipts and their connected inputs:

- `A:/FreakMelee-test-archives/script-ground-controls-guard-oct10-final4/receipt.json`
  (1,723 inputs), SHA256
  `c93e3b27aaf44c5e6ee65613ce66f37e947f59c722980955612538eb644a1b1e`.
- `A:/FreakMelee-test-archives/script-collision-copy-oct10-04/receipt.json`
  (1,819 inputs), SHA256
  `6f705fcb46da1c76efcf451ce4b28cff45fd801ac9579485123788604476f270`.
- `A:/FreakMelee-test-archives/script-fighter-observations-spawn-sync-oct10-final3/receipt.json`
  (1,691 inputs), SHA256
  `9a4ab624898992664cada10bccfe45107c8d254fe731e6ca1f0c3a7a9f726e28`.

## Next work

Six retained full sources still prepare: Mario, Dr. Mario, Peach, Game & Watch,
Zelda and Sheik. The next first diagnostics moved to Link 722 / Young Link 726
at `controls.try_air_jump`, Popo 1395 at `item.spawn_async_joint_effect`, and
Nana 1372 at `cpu.prepare_destination_update`. All bodies remain present.

Preparation census receipt
`A:/FreakMelee-test-archives/source-guard-collision-census-oct10-final/receipt.json`,
SHA256 `3f8afec8dccdc8813ab788fb6cc4dc240a27ca460a1e99c4766a8ec3f8ccb8c4`;
root verified the rebuilt tool and all ten sources. This is preparation evidence,
not complete fighter acceptance.

Continue the remaining shared input, Article and CPU operations. Actual paired
roles/member mapping and simultaneous activation remain unfinished; see the
[current companion audit](native_source_companion_contract_audit.md). Public
resource-animation application and complete fighter behavior remain open.
Compact whole-match save/load is also unfinished: Regions/Pages still owns
broad snapshots. The full goal remains active.
