# Fox/Falco connected interpreter gameplay — 2026-10-10

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The complete Fox and Falco source packages now pass twenty connected gameplay
comparisons against their stock native counterparts: ten recordings per
fighter. These runs use the ordinary interpreter and independently installed
fighter packages, rather than the earlier generated-C backend. Source
preparation/publication evidence remains in the
[input/status checkpoint](native_script_input_status_checkpoint.md).

## Problems found and corrected

- **Article declaration identity:** a name such as `It_Kind_Fox_Laser` is both a
  stock ItemKind constant and a declared article identifier. The compiler already
  resolves the exact declaration into the call's semantic owner. The runtime
  incorrectly required the numeric argument to equal that source-tree index.
  Projectile and held-center factories now use the prepared declaration owner
  and validate the literal's native integer width. Stock ItemKind comparisons
  retain their meaning; declaration names do not globally shadow constants.
- **Secondary trail graph inspection:** replay hashing treated interpreted
  Illusion/Phantasm item union data as the stock private trail-model pointer.
  It now chooses the stock private graph only for stock-owned items and the
  actual `native_models` graph for script-owned items. This repairs both parity
  traces and replay state digests without hiding geometry differences.
- **Named effects attached to bones:** `effects.spawn_on_part(FireTravel,
  bones.index(Hip))` reached the numeric engine-effect dispatcher and faulted,
  aborting the remainder of the fire-travel callback before its hitbox update.
  The existing operation now accepts declared joint-model effects with an
  interpreted update program. Numeric engine recipes keep their existing ABI.

The effect attachment retains the creator root and selected joint identity
separately. It validates the owned rig, follows the bone, and retires if that
mapping is replaced. A missing optional bone is a no-op on a valid rig. Packed
save/load retains and validates the attachment without running callbacks.
All construction uses existing fixed effect pools; no new gameplay allocation
or fighter-specific scripting primitive was added. The actual factory and all
callsites were renamed directly, without retaining a compatibility entry point.
See [model effects](scripting/model_effects.md).

## Gameplay comparison coverage

Both fighters passed these recordings on the final build:

| Family | Cases per fighter | Controller samples per case |
| --- | --- | --- |
| Blaster | Single shot, facing right and left | 520 |
| Side special | Ground startup/travel, right and left | 520 |
| Up special | Ground upward launch; aerial left launch | 520 |
| Reflector | Ground turnaround; aerial hold and landing | 1,200 |
| Radio taunt | Right and left variants on Corneria | 1,200 |

Runs compare controller samples, actions, fighter geometry/hitbox/hurtbox
state, item state/relations/pose/visual hashes, damage events and RNG. Expected
motion IDs are checked so an input recording that fails to enter its intended
move cannot silently pass. Tests are hidden and muted; radio runs retain real
clock timing, while the other cases run accelerated with drawing enabled.

These are native-stock versus interpreted-clone comparisons, not new console
or Dolphin comparisons. They do not establish every branch of every move,
all multi-fighter interactions, pixel-complete effect visuals, or compact
rollback performance. Other fighters' interpretation gaps remain separate work.

## Grouped regression evidence

- Connected named-effect constructor/VM/scheduler/packed-save-load fixture:
  **30 cases, 5,238 checks, 1,548 native model frees**. It uses a controlled rig
  and source package while executing the actual selected engine paths. Absent
  parts, non-root attachment, replacement retirement and malformed attachment
  metadata are covered. Unselected construction leaves abort if reached.
- Projectile declaration naming: **67 cases, 482 checks**; held-center naming:
  **70 cases, 2,882 checks**. Both use a declared stock-named identifier with
  deliberately different physical item assets, keeping declaration identity
  separate from native ItemKind values.
- Existing numeric part-effect ABI: **51 cases, 247 checks**. Existing numeric
  part-pair ABI: **76 cases, 66,433 checks**, including 24 native comparisons.
- All seven selected Windows targets build. Script and parameter CTests pass
  **2/2**. Input pins were independently rehashed for every final receipt.

A new `--trace-geometry-frame <sample>` launch/runner option dumps the actual
fighter joints/materials/matrices, attack/grab/throw capsules and primary item
model geometry at one requested sample. It reads live fields without simulation
or drawing calls. It was used to distinguish the trail inspection fault from
real geometry changes and the up-special callback fault from pose errors.

## Receipts

Evidence is retained under `A:/FreakMelee-test-archives/`. Earlier failed runs
remain available; they are not counted as passing evidence.

| Receipt | SHA256 |
| --- | --- |
| `fox-falco-interpreter-gameplay-oct10-e/receipt.json` | `2dfe785305f4dd93f45aaeded6861a6d921ae6636777524bc73babc30486c845` |
| `fox-falco-interpreter-gameplay-oct10-f/receipt.json` | `39352485fa497bf1cb9867154077074f57e571be1cdc80218024a6caf22d15ce` |
| `named-effect-part-oct10-c/receipt.json` | `bbe199c0655ed2a7fd928cffbb6b56bb11cf2659b447affde4a0b68cc8b1bef6` |
| `fox-falco-spawn-identifier-oct10-c/receipt.json` | `cbbfa516facfcabead71db85df4f773a9b19b8a2265eb0e25b835c1db65f7d0c` |
| `held-center-spawn-identifier-oct10-d/receipt.json` | `0fa5ea1800f283c2edfb9769478bcec9b2cf6984b615011befe2b2ebd1d6356a` |
| `engine-effect-part-regression-oct10-a/receipt.json` | `94b893ea491fa39f8095dc72ded7f889a3f0e94bda61bb421fa413433d21c9d4` |
| `engine-effect-pair-regression-oct10-a/receipt.json` | `db7b609d6860ac854aa28a87cd55f384347c62d18d1a6c965f8177245e800c91` |
| `fox-falco-gameplay-independent-oct10-a/receipt.json` | `4b03e9da956233d3ce28e9b844b93e9cc63cb211f7848c8a7b363f879f900c56` |

The independent closure receipt records pin counts 63, 1,719, 1,704, 1,702,
1,616, 1,614 and 57, plus archived build/CTest logs. Normal installed content
and Workshop drafts were not changed by these isolated gameplay runs.
Goal remains active.
