# Actual article surface and async engine contracts

Console fixture for the generic surface operations. No scripts, gameplay,
renderer, package activation or stock fighter policy delegation is involved.

```powershell
python tests/native/article_surface_engine/run.py --snapshot .local/baseline65 --output A:/FreakMelee-test-archives/article-surface-engine65-review
```

`--source-root` selects actual production bodies; the receipt records their
SHA256 independently from the immutable SDK/compiler manifest. The runner
extracts complete unchanged bodies, including `it_802765BC` and its angle and
normal-selection dependencies, and the actual async queue implementation.
It does not replace those bodies with behavioral replicas.

Coverage includes all floor/ceiling/left/right flag combinations, both-wall
selection priority and contact-index side effects, restitution and inward /
outward / zero velocity, oblique and Z normals, three floor rotation axes,
both facings, five slope normals and absent floor. The original shared mirror
changes XY only although the entry dot test includes Z; this fixture preserves
and tests that behavior.

Async mode 0/3 resolves primary root, first child and its sibling using the
actual path resolver. Actual `efAsync_Spawn` copies facing into the allocated
packet for mode 3. Changing the item facing before actual queue flush therefore
must not change the dispatched scalar. Null/missing inputs and invalid modes,
allocation failure, prepend ordering, clear-without-dispatch, and synchronous
dispatch for no invoked proc or `s_link >= 9` are checked. Item queue payloads
have no authored policy; that separate fighter policy branch is asserted
unreachable in this fixture.

Boundaries: allocation is a fixed test pool, matrix dirty propagation is an
observed sink, and `efSync_Spawn` observes dispatch arguments rather than
constructing particles/models. Actual HSD inline rotation setters execute.
This proves packet scalar ownership through dispatch/release, not safety of a
joint or item retired before flush; ordinary packets still retain the joint.
No camera/position queue mode, particle execution, pixel or gameplay claim.

CMake integration can register the Python runner as a console CTest with a
matching SDK/compiler snapshot and fresh A output. Dependencies for an emitted
header/executable target are `test.c`, `run.py`, `itmaplib.c`, `lbvector.c`,
`native_model_joint.c`, `native_article_model.c`, and `efasync.c`; include the
snapshot `src` / `extern/dolphin/include` and generated output directory.
No extra production object needs linkage because these exact bodies are
compiled in the fixture translation unit.
