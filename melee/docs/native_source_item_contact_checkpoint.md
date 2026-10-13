# Interpreter Item contact and resource-collection boundary

This is the earlier contact-batch checkpoint. Its collection boundary is
superseded by [the source-owned resource checkpoint](native_source_resource_checkpoint.md).
The contact evidence below remains specific to its recorded input snapshots.

## Executable contact operations

The interpreter now dispatches six generic zero-argument Item operations directly
to their native implementations: floor sweep, wall/ceiling contact flags, wall
reflection, floor alignment and velocity-facing alignment. The wall and ceiling
calls update the selected surface; they are not pure queries. Reflection keeps
native wall precedence and damping, including damping when moving away from a
wall. Floor alignment resolves the actual native bone-table entry or primary
joint and admits its class before entering the native setter. Output publication
retains the exact owned Item/program/storage identity.

Three read-only observations expose the current contacted Fighter handle,
reflection damage-owner-transfer policy and the Fighter action's item-detachment
policy. The latter uses the generic recipient spelling
`attachment_owner.fighter.action_detaches_items`. It applies to any Fighter,
not a Link-specific move. Missing initial contact returns zero; a captured target
changing during publication is an error.

Link and Young Link's remaining attached-owner expressions now use the actual
revocable attachment receiver, rather than legacy generated-emitter aliases.
Absent-value defaults and damage-owner eligibility remain explicit in source.
Original creator provenance is not substituted for an attachment pointer.

## Collection implementation boundary

Hookshot, Sheik Chain and Climber string resources are distinct ItemLink GObjs
with their own collision histories and model graphs. They cannot be implemented
by assigning extra Item model-slot numbers. The current resource backend still
looks up legacy NativeFighterDefinition/ArticleLogic identity; interpreter Item
programs must instead own immutable collection recipes and retained scene
registrations directly.

The next implementation must cover the following connected contracts:

- Prepare exact collection capacity and optional external-part reservation in
  the owning Item program; validate reservations against the final fighter rig.
- Reserve fixed ItemLink/node backing and resource graph/controller capacity at
  scene transitions. Existing secondary Item graph reservations do not reserve
  collection nodes. Normal ticks must not allocate a new backing arena.
- Retain physical node/owner generations, exact program/schema identity and
  package leases. Construct/replacement needs actual node graph registrations,
  separate from secondary Item slots.
- Preserve deferred collection release while sweeps/effects are active. Stop
  native continuations when the caller, node or graph retires.
- Retain the actual borrowing Fighter identity independently of the revocable
  logical attachment. Cleanup must clear matching external-part, capture-anchor
  and hitbox references before freeing their graphs, including actor teardown.
- Include collection registry, free metadata, ItemLink/CollData and mutable graph
  backing in actual history restoration. Immutable package data stays pinned.

The existing six-collection/ninety-node limits are historical backend limits,
not proof of the required four-player paired-fighter capacity. Capacity must be
checked against the admitted scene workload before interpreter activation.

## Current whole-source preparation

The refreshed actual `native_content_tool source-check` census prepares Mario,
Dr. Mario, Peach, Game & Watch and Zelda. Link, Young Link, Sheik, Popo and Nana
still reject their `resources` metadata. No declaration was deleted or skipped
to obtain a pass. The first metadata error does not validate subsequent bodies,
and preparation alone does not accept a whole fighter's gameplay behavior.

Receipt: `A:/FreakMelee-test-archives/source-item-contact-census-oct09/receipt.json`,
SHA256 `fb0f2063f72cf7874c3de7bd6c6d5c2034cc0cdcd22209a0d572fc0fc7fb88c0`.
The actual client/content/source-assets targets rebuild, and the eight selected
allocator/Item runtime CTests pass. No game, GPU, audio or visible launch was
needed for this batch.

## Grouped execution evidence

`A:/FreakMelee-test-archives/script-item-observations-six-oct10-final/receipt.json`
passes 8,595 checks in 1,325 cases, with 1,129 native Item/JObj payload comparisons
and 58 VM calls. The six new operations execute through the actual frontend,
catalog and VM. Native kernels, vector math, JObj dirty propagation, fixed pool
and lifetime validation are real; Item spec/view publication and stage geometry
are controlled fixture boundaries. Operation-specific post-complete observers
test rejected publication after identity changes; they are not native authored
callbacks. Receipt SHA256:
`2e4809a52d93503386da3dd3c8f1084c3018d6658bffb8ea8775292188ebdbd4`.

`A:/FreakMelee-test-archives/script-article-motion-observations-oct09-final/receipt.json`
passes 6,426 checks, retaining the fixture's 3,078 native comparisons. The three
new properties execute through actual receiver projection, VM and Item actor
storage. Contact tests retain full-width creation handles, missing/stale initial
targets, captured target removal/reuse and unchanged failed outputs. Recipient
detachment tests read stock Fighter fields and preserve explicit revoked-owner
defaults. Native package/assets and raw stock payload storage are controlled;
unselected common-property model/class paths reject explicitly. Receipt SHA256:
`2fe71e65768c019ddc83a2b8ba07f89bb2c6211f75e940f791084822bf941089`.

Root independently verified all 1,651 and 1,670 recorded input fingerprints
against current files, plus the ten census source hashes and actual tool hash.
These grouped fixtures accept bounded interpreter operations, not DAT Item
construction, full fighter activation, real stage geometry or gameplay parity.

Resource collection ownership and compact whole-match snapshots remain
unfinished. The full project goal stays active.
