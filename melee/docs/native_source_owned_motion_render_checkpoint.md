# Owned motion policies, drawing and fighter item controls

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Canonical Peach, Game & Watch, Popo and Nana scripts no longer contain a donor
construction hint. The C comparison oracle receives its root/first/count tuple
explicitly; all 65 callers were adjusted directly. Normal packages remain
interpreted source and need no C compiler or behavior DLL.

Climbers' neutral and down specials now have separate named move containers,
IceShot and Blizzard. The original bodies are byte-for-byte unchanged after
reversing only the headers/base removal. A named move still has one special
entry pair; neither an ambiguity nor a missing behavior is silently accepted.

Common motion policies prepare canonical shared animation/interrupt/physics/
collision callbacks in the actual owned motion rows. Numeric assets, flags and
camera remain unchanged, and authored script phases take precedence. The whole
candidate is checked before mutation; metadata revalidation compares the actual
rows and pinned common table. No artifact contains borrowed fighter handlers.

Source Article drawing now selects Standard from its own immutable program;
Workshop items select Billboard. The constructor registers the chosen callback
before model load and Spawned, preserving native timing while avoiding the
physical-kind renderer table. Standard uses the native primary-model renderer.
Secondary models are still unsupported.

The native HSD topology admission was moved into a substantive shared unit.
Both command imports and source entity preparation call it directly, including
indexed models with no DATA script. Instance links, postorder descriptor IDs,
duplicate IDs, the 100-slot table and nullable slots retain their native rules.
Particles may use their native anchor without requiring a mesh.

Fighter controls explicitly distinguish originally created items from restoring
foreign held/auxiliary items. Attachment preserves revoked control; scale follows
the separately retained current damage owner. These operations use full integer
handles and actual source actor identities. See [Fighter item controls](scripting/held_item_controls.md).

## Evidence boundaries

Private receipts under `A:/FreakMelee-test-archives/`:

- `oracle-construction-two-move-final`: 12 existing oracle samplers/nine CLI
  cases; explicit construction, ordinary action parsing, owned motion bounds and
  failed-output preservation. This is the test-only C oracle, not game execution.
- `climber-two-move-source-proof`: two exact unchanged-body comparisons.
- `source-owned-declarations-ctest-91328db695024681aaebec042aea3900`: 508 checks/
  97 cases/four real DAT imports, including actual common policy rows, prepared
  script overrides, tampering rejection and transactional malformed-policy cases.
  Whole-match phase execution is not covered.
- `source-article-standard-render-final-second`: 18 checks/three cases through
  the actual constructor/actor/VM/renderer bodies; HSD/graphics leaves controlled.
- `source-article-standard-render-import-final`: 113 checks/16 cases, real
  Link/Young Link owned DAT imports and HSD primary graphs, six joints/two roots,
  metadata negatives and no-DATA 100/101-node admission. No GPU frame proof.
- `item-model-admission-split-first`: 2669 checks/47 cases, 185 Articles/281
  streams and all 29 production stage specifications after the direct split.
- `script-fighter-item-controls-review7`: 281 checks/34 cases, ten native scale
  comparisons and 25 actual VM calls across all nine controls. Physical/logical
  identity changes, cleanup windows, foreign restoration and snapshots are covered;
  package/rig publication and graphics leaves remain controlled.
- `script-article-lifecycle-fighter-items-review5`: 321 checks, 16 native pickup
  comparisons and 42 picked-up callbacks; all earlier auxiliary scenarios remain.
- `script-held-continuation-final`: 71 checks/12 cases, one real mutating
  request-helper VM fault and controlled pure status identity observations.

Client/content-tool/reference builds and all 37 selected interpreter/content/
pool/lifecycle CTests pass after integration. The raw held C-policy fixture now
uses actual pool identities and retains its original hazards/depth checks.
It passes 566 checks. Held continuation additionally uses the actual Item/Fighter
actors, receiver dispatch, common status/request bodies and motion-entry segment.
The Parasol parent stops before subsequent setup after a real Fighter helper VM
fault; pointer/classifier identity changes also stop cached continuation. Held
status stays read-only. The fixture covers the request's mutating helper and
status's pure observations, not an unsupported read-only helper call.

Actual package diagnostics progress past the closed cases, but whole fighters
remain unaccepted. Dr. Mario next needs `model.base_scale`; Peach needs held-ID
queries; Game & Watch needs costume/color primitives; Popo/Nana reach secondary
resource metadata. Link/Young Link still need secondary trail models. These are
first errors, not an exhaustive missing-operation inventory. Sheik resource
ownership, remaining native schedulers and roster/paired runtime checks remain.

No game, installed content, original ISO or user draft was changed. The full goal
remains active and local commits only are authorized.
