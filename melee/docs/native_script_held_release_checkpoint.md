# Interpreted immediate held-item release checkpoint

Date: 2026-10-10. Ordinary fighter scripts execute through the typed
interpreter; generated C remains test-reference infrastructure only.

## Delivered

`items.release_held(vx, vy, vz, multiplier)` executes the actual common
`Item_8026ABD8` release routine. It does not substitute common throw-motion
entry for immediate release, or call a DK-specific handler. Arguments are
finite Float values, including signed zero and negative values. The shared
engine retains its throw-speed scaling, drop sound, authored/ordinary dropped
callback, damage ownership and collision continuation.

The binding captures the exact owned calling fighter and the selected held
item's physical pool identity, generation and published script peers. It rejects
missing native release assets before entering the routine. An absent held item
is a no-op. A dropped callback may destroy its item, including immediate reuse
of the same pool slot; that is a normal terminal outcome without dereferencing
the replacement. Surviving item faults or identity changes, and retired/faulted
callers, reject result publication. No persistent ownership record or allocation
was added. Existing native release continuation guards remain unchanged.

## Evidence

- Connected item-control compiler/Prepare/VM/actor fixture: **1,430 checks /
  154 cases**, including 52 native comparisons overall and **10 new release
  comparisons** against the exact current native body.
- Direct/native and interpreted operations cover ordinary heavy items and
  source articles, zero/negative damage factors and healthy tail order. Compiled
  helpers release both families. Real authored Dropped hooks execute through
  the interpreter. Ordinary and authored callback cases cover retirement,
  same-slot replacement, generation/class/removal changes and caller retirement;
  source-only cases additionally cover storage faults and callback failure.
- Invalid signatures, nonfinite values, missing model/descriptor/attributes/
  behavior and stock calling fighters reject without result publication.
- Capture scheduler regression: **30,254 checks / 4,811 cases**.
- Combat regression: **37,048 checks / 12,240 RNG pairs / 30 native-body pairs**.
- Seven Windows targets build; `native_fighter_script` reference CTest passes.
- Ten-source read-only roster cohort retains six successful preparations.
  DK advances from release to `capture.initialize_hold` at line 451. Bowser still
  stops on positional projectile launch at line 315; Fox/Falco retain their
  metadata/asset preparation gates. No full DK/Bowser activation is claimed.

The item fixture had stale provider dependencies: duplicate `.c` suffixes,
repeated unavailable providers and missing newer binding-family leaves. These
were repaired with the existing explicitly unavailable test providers and real
extracted heavy-item/air-state helpers. The expanded raw destruction injection
now calls an authored Destroyed hook only when a source actor is attached.
These are fixture corrections, not production behavior changes.

## Receipts and limits

Private evidence is under `A:/FreakMelee-test-archives/`:

| Receipt folder | Independently verified SHA-256 | Input pins |
| --- | --- | ---: |
| `script-held-release-oct10-final-c` | `b08943577da4bc8b3c819957d2f8eade1ad6614a153ae89eff0ef5f68860adab` | 1,722 |
| `script-held-release-capture-oct10` | `0a5be34314c9834717c77e844bf2ac62930b6a5c383948e1adbb89ce9efcf954` | 1,755 |
| `script-held-release-combat-oct10` | `f3e5d24a7da7c6691efac26cd495bd10f800792a772853b80b69934baa57f299` | 1,680 |
| `source-held-release-oct10-final` | `a237a5f3a6414b1ac4b4461eca499560700dcccdc3921bb0dc1d76cf05cfce8e` | 27 |

All consumed pins were independently rehashed unchanged. The second-pass
summary is `held-release-independent-pins-oct10.json`. Earlier failed/partial
fixture outputs remain separate from final acceptance.

The native release dispatcher and sound selection are extracted production
bodies. Transform/detachment, damage-owner continuation and collision leaves
are controlled; full release physics, rendering, DAT loading and match gameplay
are not established by this fixture. No installed assets changed or game window
opened. Whole-fighter activation and compact whole-match rollback remain open.
