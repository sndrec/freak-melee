# Resource-GObj effects

Console proof for the generic resource position / position+facing statements.
This uses the snapshot compiler and frozen original geometry/efAsync bodies,
with explicitly recorded live SDK headers/compiler units for the new Host ABI.
The snapshot is never mutated.

```text
python -B tests/native/article_resource_effects/run.py --snapshot .local/baseline72 --output A:/FreakMelee-test-archives/article-resource-effects-root
```

The batch builds the real compiler, emits both action and motion-callback routes,
compiles each generated module, and reuses the production resource geometry,
shared joint and live generation-registry matrix. There are 29 compiler programs
and 28 generated retirement scenarios per route. The module tests validate both
required Host pointers and the lifetime pointer, single ordered operand
execution, unchanged continuation on a false dispatch result, retirement at every
operand and either effect call, address reuse, and inaccessible caller pages.
Fighter/model-effect contexts, missing resource declarations, Bool/Float handles,
wrong effect types, Bool coordinates/facing, bad arity and expression use reject.

The backend matrix dispatches against the actual node GObj/ItemLink, never the
article GObj or a fake Item. Thirty-one unchanged position-generator case bodies
and all four facing-generator bodies are extracted from actual efAsync_Dispatch;
the latter verify both generator selection and facing sign. It also tests both
direct-generator range endpoints, positive mismatched joint/model/facing IDs,
unknown/negative IDs, finite input enforcement, stale article/creator generations,
foreign author/logic, allocation failure, and cleanup requested by release,
Definition retirement, article recycle or creator retirement during dispatch.
The node remains available to the engine call until it returns, while its
collection is immediately unavailable to new callers.

Controlled boundaries are HSD allocation/destruction/draw, generator allocation
and particle evaluation, ECB responses/matrix arithmetic, and module Host effect
calls. The real resource backend is tested separately from the generated Host
boundary. The efAsync cases are actual complete case bodies inside a scoped
switch; efSync's range routing is checked at the controlled boundary, not claimed
as a complete dispatcher/asset-loading comparison. Existing geometry source
comparisons remain intact. No GX pixel, authored Samus solver, gameplay or full
character activation acceptance is claimed.

The supported payload contract is intentionally bounded. Unknown/JObj/model
recipes reject before variadic dispatch. No EF_SpawnResult or ambiguous model /
particle pointer escapes to script state. Backend success means dispatch occurred,
even if allocation returned no generator. The two Host fields alter the automatic
ABI fingerprint; mutable resource-pool layout and its existing rollback coverage
are unchanged (the deferred-use counter is renamed active_operations).

Root CMake tests: native_article_resource_effects_compiler/action/callback. The existing
native_article_resource_geometry target generates its two additional header/JSON
byproducts through article_resource_geometry_source_fixture.py.

Accepted console receipt: `A:/FreakMelee-test-archives/article-resource-effects-handoff/receipt.json`:
29 compiler programs, 881 generated checks per route, 3,276 backend/geometry checks.
The receipt was completed before a documentation-only correction of the CMake
names in this README; executable inputs stayed byte-identical. The namespace
source manifest records the final documentation hash separately.
