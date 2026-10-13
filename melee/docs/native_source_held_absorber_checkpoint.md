# Interpreted held-item observations and absorption

This checkpoint implements shared engine controls used by authored fighters.
Fighter-specific decisions remain in their scripts. Ordinary runtime execution
uses the interpreter; the C reference path is retained for comparisons only.

## Held-item observations

`items.held_id`, `items.held_kind`, `items.held_motion` and `items.held_frame`
observe the Fighter's current physical held slot. The same namespace works
through captured recipients, such as `holder.items.held_motion`. The code directly
validates Fighter/Item class, full pool identity, native generation, registry
membership and retained slot before publishing a value. It does not require the
held item or observed fighter to have an authored program or match the caller's.

Missing, unlinked or removing slots yield the native -1 / 0.0 defaults. A live
wrong-class/malformed object or a changed captured identity is an error. Native
ID bits, frame NaNs and signed zero are preserved. A faulted item script does not
turn an otherwise live physical field observation into authored-code execution;
actions on an authored item still require their own healthy actor admission.
The old FighterItems query branches have been relocated, and callers were
changed directly rather than retaining forwarding compatibility functions.

## Absorption

`absorber.create` creates the existing native absorption volume from a raw
joint, local XYZ and radius. Creation uses the fighter's bound imported rig;
numeric indices outside that owned span preserve the reference healthy no-op.
An in-range missing joint or malformed rig remains an error.

`active`, `hit` and `skip_position_update` are editable native flags. `damage`
and `hits` are signed integer observations, and `direction` is Float. Simple
fields can be observed/edited on ordinary physical Fighter recipients, while
creation requires the owned rig. The native helper changes the volume and its
enable/hit flags; scripts own phase transitions, stored charges and healing.

`on absorb` now has an explicit interpreter hook ABI. The actual fighter update
routes attached actors to that hook and validates retained Fighter/program/state
identity before continuing. An absent authored hook does not fall back to stock
fighter behavior. Ordinary non-interpreted fighters retain their original branch.
The native update clears its damage/hit/direction accumulation after notification
in its existing order. `damage.heal` uses the already implemented shared helper.

The common `controls.horizontal_stick_deadzone` is also exposed directly from
current common data, without copying it into fighter parameters or normalizing
its bits. Existing generic property/VM checks cover read-only access, source
contexts, missing common data and non-finite values.

## Integration evidence and current boundary

The consolidated native client, content tool, reference exporter and selected
test builds passed. Forty integration tests and three C-reference tests passed.
Private source preparation census:
`A:/FreakMelee-test-archives/source-held-absorber-census-oct09-final2/receipt.json`.

Mario, Dr. Mario and Zelda still prepare. Peach advances to recipient export
dispatch at line490; Game & Watch advances to hitbox state/damage operations at
line548. Link/Young Link, Sheik and Popo/Nana secondary-resource boundaries are
unchanged. Source preparation alone is not complete fighter or gameplay proof.
Game & Watch's palette calls were migrated to actual typed creator-owned helpers,
without legacy `creator.call` aliases.

Grouped native fixtures:

- Held observations: `A:/FreakMelee-test-archives/script-held-observations-oct09-final/receipt.json`
  passes 1243 checks / 115 cases, including 42 inherited native comparisons,
  28 exact held-getter comparisons and eight compiled Item signal/receiver
  executions. This uses the actual compiler, VM, actors, receiver and pool;
  owned publication/assets/graphics/constructor leaves are controlled.
- Absorber: `A:/FreakMelee-test-archives/script-absorber-final/receipt.json`
  passes 275 checks / 67 cases / 47 native comparisons. Creation compares
  whole-Fighter changes against the native helper. Actual VM callbacks and the
  extracted native Absorb branch/reset order cover missing hooks, retirement,
  fault and foreign recipient fields. Assets, position/statistics leaves and
  property entry are controlled; the complete collision loop does not run.

Both fixtures exercise retained source identity rather than accepting a matching
fighter kind alone. The final shared build passed, followed by all 40 selected
integration and three reference tests. Packed actor restoration explicitly needs
its native Fighter peer restored; actor snapshots alone do not prove rollback
of absorption geometry.

No game-window, particle appearance or whole-fighter match acceptance is claimed
by this console checkpoint. The overall project goal remains active.
