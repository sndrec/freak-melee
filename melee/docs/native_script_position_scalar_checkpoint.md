# Scripted geometry and position/scalar effects — 2026-10-10

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Fox and Falco's fire-bounce effect, throw-shot angle and gun muzzle effect now
use the existing generic engine primitives plus one generic owned-Item effect
operation. The sources prepare these bodies through the interpreter. This is
not full fighter admission or gameplay acceptance.

## Authored behavior

`effects.spawn_with_scalar_at_position` already provides the original
`efSync_Spawn(id, source, &position, &scalar)` ABI. Fox/Falco fire-bounce and
Ness thunder-rebound calls now name that operation directly. Their effect ID,
world position and angle expressions remain unchanged. The Ness default is the
same PositionScalar recipe 1030; this batch does not claim a full Ness audit.

Throw shots query the two stock local offsets on the animated right-thumb joint,
then compute `atan2_float(muzzle_y - barrel_y, muzzle_x - barrel_x)` in source.
Bone queries take raw indices, so `bones.index(RightThumb)` resolves the semantic
part first. There is no new angle helper, special handler or native fallback.

The gun's accessory callback queries its original creator's mapped thumb joint
and emits recipe 1166 at that world position. Its zero rotation retains stock's
same-muzzle-point subtraction, documented in the script. Initial launch factories
and recoil signaling remain unchanged.

## Generic Item effect operation

`item.spawn_with_scalar_at_position(effect, x, y, z, scalar)` accepts audited
PositionScalar recipes. The scalar's meaning belongs to the effect recipe,
such as angle, scale or facing. The interpreter passes a copied Vec3 then a
copied Float in native order. IDs are integral and coordinates/scalar finite.
Fighter-parent recipes reject for Items; root-reading recipes require the
admitted source root. Other recipes permit rootless owned Items. Source,
schema, physical identity and native-root requirements are retained across
the spawn. Retirement prevents result publication and subsequent script writes.
No per-call heap allocation or fighter-specific dispatch was introduced.

The API is documented alongside [joint feedback](scripting/joint_feedback.md)
and [item observations](scripting/item_observations.md).

## Grouped evidence

- The actual Item actor/compiler/interpreter feedback fixture passes **2,223
  checks / 212 native spawn-leaf calls**. It exercises all admitted
  PositionScalar recipes with and without roots, exact argument capture and
  retirement/schema/model guards. Particle spawning is a controlled leaf.
- The source-derived effect ABI matrix passes **524,631 checks**, compares all
  **65,536 IDs**, and runs **48 native helper-copy cases**. It independently
  checks recipe argument/context requirements, not rendered appearance.
- The actual Fighter actor/compiler/interpreter bone-query matrix passes
  **3,815 checks / 232 cases / 154 native comparisons / 195 VM calls**. Thirty-six
  new cases compare the scripted throw-angle composition bit for bit against
  native matrix queries and `atan2f`, over hierarchy/quaternion/facing changes.
- Seven Windows targets build. Script and parameter CTests pass.
- Final isolated audits visit all **256 Fox / 253 Falco bodies**, completing
  metadata and traversal with **6 failing bodies each**, down from 9. Neither
  package is admitted. Remaining first failures are air-jump input, four
  stage/conversation bodies and trail secondary-model duplication; later errors
  in those bodies can still be exposed.

Receipts:

- `A:/FreakMelee-test-archives/fox-falco-item-scalar-feedback-oct10-d/receipt.json`
- `A:/FreakMelee-test-archives/fox-falco-item-effect-abi-oct10-a/receipt.json`
- `A:/FreakMelee-test-archives/fox-falco-scripted-throw-angle-oct10-b/receipt.json`
- `A:/FreakMelee-test-archives/fox-falco-position-scalar-audit-oct10-b/receipt.json`
- `A:/FreakMelee-test-archives/fox-falco-position-scalar-independent-oct10-a/receipt.json`

Failed feedback attempts `-a`/`-b` exposed stale fixture dependencies; `-c` exposed
an incorrect fixture expectation that a reflected callback returns Void rather
than Bool. The corrected callback explicitly returns false. The initial geometry
attempt `-a` caught the semantic-part/raw-index distinction; both production
source call sites were corrected. The initial audit preceded those final bone
index corrections. Failed attempts are retained. Unselected async effect enqueue
aborts if reached; other unavailable bindings return failure.

No game process, normal installed package or user draft was changed. Full fighter,
rendered match and compact rollback acceptance remain open. Goal active.
