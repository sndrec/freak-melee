# Samus owned authoring and locomotion checkpoint

2026-10-10. Samus now creates a real owned stock-asset draft. The canonical
interpreter source remains rejected: the full 172-body audit reports 14 failing
bodies, down from 28 at the beginning of this batch. This does not establish
Samus gameplay parity or an independent, usable clone.

## Asset facts

The earlier census pointed at a nonexistent Samus draft because draft creation
failed before compilation. The Samus authoring catalog had no semantic bone map;
owned asset validation requires one. The catalog now contains the exact retail
v1.02 PlCo mapping: 41 semantic roles, 60 runtime joints and no inserted parts.
The new `tests/native/samus_rig_stock_compare.py` decodes both directions of the
retail map and compares every binding with the production catalog. It walks all
five costume graphs (Bk, Gr, La, Nr, Pi), each with 60 physical joints.

`clone-stock` now succeeds into the private
`A:/FreakMelee-test-archives/samus-source-authoring-oct10-b` draft. The user's
installed collection was not modified. Catalog stock callbacks are still
authoring references; adding the rig map does not translate their behavior.

## Source translation batch

- Missile and bomb accessory decisions are root script functions installed
  directly with `callbacks.accessory`. The obsolete `track_accessory` calls and
  nested accessory bodies are removed; no native special handler is forwarded.
- Charge Shot angle/fraction locals use Float storage. Its release signal names
  the declared charge article, and maximum charge uses the native float-to-int
  conversion when copied into its integer counter.
- The charging sound identifier no longer shadows the `sound` namespace.
- Missile landing reproduces `ft_80082B1C`: above the common vertical-speed
  threshold enter Wait, otherwise enter basic Landing. It does not unconditionally
  substitute Landing for the original decision.
- Charge Shot's release effect uses the existing synchronous world-position and
  one-scalar recipe, matching `efSync_Spawn(1158, gobj, &position, &facing)`.
- Four common locomotion properties expose walking acceleration/speed and
  ground-jump momentum/horizontal limits from the physical fighter's common
  attributes. Morph and bomb physics use those exact fields. These are generic
  properties, not Samus attributes, stock-kind lookups or special DAT offsets.
  See the [movement reference](scripting/movement.md#common-locomotion-attributes).

The audit collects the first error in each body, so the remaining 14 bodies can
contain additional gaps. They cover action metadata, article attachment/effects,
owner state and detachment, Item movement, target queries and collision handling.
Grapple and remaining lifecycle decisions also require translation; the source
does not declare `behavior independent` merely to bypass that work.

## Verification

- Seven native build targets pass, including the game and content tool.
- `native_fighter_script` and `native_fighter_parameters`: 2/2 CTests pass.
- Retail rig comparison: all 41 bindings and all five costumes pass.
- Connected Fighter-observations cohort: 21,415 checks, 2,593 cases,
  3,691 native field comparisons and 1,592 VM calls pass. New cases exercise all
  four locomotion values across 32 input sets, read-only writes, live stock
  observations and stale/missing/wrong-class actors. They compare physical fields
  and real interpreter delivery; they do not run walking or jumping gameplay.
- Source audit: 172 bodies, complete metadata/body coverage, 14 diagnostics,
  no outer package diagnostic, **package rejected**.

The observations fixture needed maintenance for newer unselected stage, Item
write and smash-table routes. Those leaves remain unavailable. Initial new test
expectations incorrectly required pure physical observations to reject stock
actors and a manually changed program identity; the corrected tests follow the
actual physical-observation contract. Production admission checks were not relaxed.

Sealed evidence:

- `A:/FreakMelee-test-archives/samus-source-authoring-oct10-c/receipt.json`
  combines rig/audit evidence, draft and tool hashes, build/test logs and closure
  of all 1,730 dependency pins in the observations cohort.
- `A:/FreakMelee-test-archives/samus-locomotion-observations-oct10-d/receipt.json`
  contains the connected cohort's commands, hashes and controlled-boundary limits.

Earlier failed observations runs a/b/c remain separate from the passing d run.
No Samus game was launched, accepted package installed, whole-roster census
promoted, or rollback performance claim made in this checkpoint.
