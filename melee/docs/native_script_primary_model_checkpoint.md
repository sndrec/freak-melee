# Independent Article model cloning — 2026-10-10

All 256 Fox and 253 Falco bodies now compile in the interpreter audit.
Neither fighter package is admitted yet: the first remaining package diagnostic
is the unmigrated `wait_input` scheduler contract. This checkpoint does not
establish full fighter behavior or rendered trail parity.

## Engine contract

`item.clone_model(slot)` constructs a separate graph from the Article's owned
primary descriptor into secondary slot 1, 2 or 3. It copies the live primary
root rotation and applies the Item's uniform scale, matching stock trail
construction. Translation, children, materials and animation state begin at
descriptor defaults; the operation does not snapshot the live primary pose.
Missing primary, occupied slot and pool exhaustion return false.

Preparation discovers clone use in the Article's own bytecode, retains the exact
primary descriptor and reserves model storage before gameplay. Construction uses
the existing scene pool and ownership/generation registry directly. Package,
Item and graph guards remain in place, including matrix claims and teardown.
There is no stock-special delegation or legacy clone-helper fallback.

The previous reservation budget multiplied all 512 possible Item actors by three
models. That exceeded allocator slot bounds for ordinary multi-material graphs
(the actual Link primary has three joints and five materials). Secondary model
storage now has an explicit shared capacity of 128 simultaneous graphs. Each Item
still has three secondary slots. A vacant request fails cleanly at capacity;
replacement reserves a separate temporary graph and preserves the prior graph on
failure. Unsupported graph features still reject preparation.

Sources: `native_article_model.c:nativeArticleModel_Clone`,
`kinds/itfoxillusion.c:it_8029CD78`, the actual HSD constructors, model census,
reservation and scene ownership code. Authoring documentation:
[Article models](scripting/article_models.md).

## Grouped verification

- Seven Windows build targets pass; script and parameter CTests pass (2/2).
- Actual-library scene fixture uses owned Link, Fox and Falco DATs with real
  source metadata, compiler, Item actor, VM, HSD graph constructor, matrix
  traversal and teardown. Link: 630 checks / 13 cases / 137 graphs. Fox and
  Falco: 96 checks / 6 cases / 4 graphs each. Tests cover three simultaneous
  independent clones, occupied/missing-primary cases, exact root transforms,
  stale generations, heap stability, 128 admitted secondary graphs, exhaustion
  and replacement at capacity.
- Runtime slot/main-heap transition tables are controlled fixture inputs.
  Minimal owned Article scripts exercise construction; the complete fighter
  sources are audited separately. No renderer, full physics/scheduler scene,
  initialized rollback history, Workshop admission or whole-match acceptance.
- Early failures were fixture assumptions (a freed graph's address can be
  reused; Falco's declaration is Phantasm), stale metadata expectations and the
  production reservation-size issue. Failed evidence is retained separately.
- Final scene receipt independently rehashes 3,119 inputs; final audit receipt
  rehashes 20. Shipping sources, seven binaries and build/CTest logs are also
  verified. No game launch, installed-content changes or user-draft changes.

Evidence under `A:/FreakMelee-test-archives/`:

- `fox-falco-primary-clone-scene-oct10-i/receipt.json`
  SHA256 `5c2c044eb4c4b9dbc186264f2c65a8781a8ea0e79b0bb598dbe3218bfde62847`.
- `fox-falco-primary-clone-audit-oct10-b/receipt.json`
  SHA256 `9fb4136b66432435e658a15b02fc727f987b6a34f1a0a288165ea360be2d7bcc`.
- `fox-falco-primary-clone-independent-oct10-a/receipt.json` records the
  independent rehashes and archived logs.

Next: migrate generic input/action-status hooks, then prepare the complete
packages and compare connected interpreted behavior against native references.
Full fighter and compact rollback acceptance remain open. Goal active.
