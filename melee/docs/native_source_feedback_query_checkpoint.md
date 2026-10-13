# Source feedback payloads and callback identity

Implemented 2026-10-08. Fighter position effects, Article root effects and Article
effect cleanup now use the direct interpreter. A generic pure callback identity
query replaces the old per-slot pointer comparisons. No borrowed behavior handler,
dummy model or generated C is required for these operations.

## Exact payload and ownership contracts

The effect ABI classifier now admits94 Joint,32 JointScalar and63 Position IDs.
Existing82 Joint IDs are preserved; twelve literal callers of the proven
`efLib_CreateGenerator_Attach_Scale` helper add the one-joint signature. That
helper deliberately retains/refcounts the attachment through the generator and
copies source-root Y scale after creation. Temporary position/scalar pointers
remain separate: admitted position consumers copy values synchronously. Unknown
forwarded/erased/extra payloads and generic fallback ranges stay unsupported.

`effects.spawn_at_position` validates the full integral ID and finite F32 position,
passes a local copied Vec3, and rechecks source ownership before publication.
`item.spawn_root_effect` passes the actual live engine root; `destroy_effects`
uses shared native cleanup. Neither requires a creator. Item program/spec/view/
storage, physical pool/generation, logical status and root/class identity are
checked before and after calls. Engine class metadata proves the JObj type/size;
there is no exposed per-JObj allocation-generation/span API, so arbitrary dangling
pointer detection is not claimed.

The shared generator creation path has native hooks. Its current registered
hook only installs later particle callbacks; the default-shape hook is NULL.
No synchronous script/destruction call was found in that creation path. The
native post-create scale read is preserved. This does not guarantee safety for
arbitrary future construction hooks or establish complete particle lifetime.

`callbacks.matches(Callback.slot, helper_or_None)` checks the actor's current code
ID with exact root-helper Void/no-argument ABI. It does not call the handler or
mutate callback metadata. Native handlers match neither helper nor None. Invalid
indices/signatures fail with outputs unpublished. Source-only admission prevents
caller helper indices from being mixed into another fighter's bank. Callback
constants cover the current twelve instance slots. Sheik cleanup now uses this
generic query and preserves replacement-handler decisions.

## Settled verification

Receipts under `A:/FreakMelee-test-archives/`:

- `script-position-effect-abi-settled-root/receipt.json`:131268 checks across all
  65536 IDs and30 actual helper-copy cases. Exact case bodies/helper/refcount proof,
  signedzero/NaN/infinity value-copy behavior; allocator/model leaves controlled.
- `script-position-feedback-settled-root/receipt.json`:67432 checks/137 cases,
  52 retirement mutations. Actual dispatcher/catalog/source guards/pool; native
  effect/wind/sample and source views are controlled. All63 Position cases route
  to the copied-position ABI; existing Joint/Scalar behavior remains checked.
- `script-callback-identity-settled-root/receipt.json`:2511 scheduler/actor checks;
  actual source/VM/actor/storage/snapshot/pool. Helper/None/native takeover,
  unchanged packed state during pure query, restored registrations and retirement
  are exercised; native contact/graphics/scene peers retain their documented controls.
- `script-article-feedback-settled-root/receipt.json`:282 checks/13 VM-to-native
  root spawns, actual Item actor/VM/storage/GObj pool and HSD class initialization/
  ancestry plus native effect destruction. Particle/model/allocator leaves are
  controlled; no Item DAT constructor, rendering or live particle acceptance.
- `script-feedback-query-catalog-settled-root/receipt.json`:93 typed frontend/VM
  routing checks with deliberately rejecting native capture leaves. Wide integral
  IDs/slots and owned helper/None code-reference marshalling are preserved.

The unified client/tool/affected fixture build passes, as do36 selected regression
CTests (`.local/source-feedback-query-build.log`,
`.local/source-feedback-query-ctest.log`). Full form preflight remains Zelda's
Article signal at145 and Sheik resources at1543; original private drafts unchanged
(`source-form-template-feedback-query-preflight-root/receipt.json`).

No game or installed-content migration was performed. Continue the connected
factory/Article reference/recipient-signal group, followed by complete Chain
resource/model/collision/matrix ownership and full paired source/runtime checks.
Complete explicit rollback state/performance and companion behavior remain open.
