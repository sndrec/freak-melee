# Source Article lifecycle fixture

This fixture executes the actual source compiler, descriptor catalog, bytecode,
Fighter/Item actors, storage, receiver guards, and registered GObj pool. It
compares relation changes and pickup setup with the actual native pickup path.
Original creator and mutable attachment relation are actor-owned; damage owner,
owner-bound flag, and Fighter auxiliary pointer remain separate engine fields.

The extracted production bodies are `Item_8026AB54`, `it_802742F4`, pickup sound
and tail, creator bootstrap, existing Item self property read/write, and the
selected-source Fighter guard. Graphics,
audio, collision and transform leaves have controlled implementations. A field
write inside the real interpreted picked-up callback injects lifetime changes;
the fixture does not claim those unrelated engine leaves invoke scripts.

Run with the MSVC-target archive compiler and a fresh A: directory:

```powershell
python tests/native/script_article_lifecycle/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-article-lifecycle-review01
```

No game, full build, DAT item constructor, graphics/audio initialization, or
user project is used. All compiler/test subprocesses use CREATE_NO_WINDOW.

`attach_to_owner` uses a raw imported joint index and defaults auxiliary
registration to false. Representable negative/out-of-rig indices are healthy
no-ops, matching the previous emitter; floats or values outside native 32-bit
width fail without truncation. The engine pickup body sets owner_bound and
runs the real interpreted picked-up hook. Post-hook retirement/fault stops
auxiliary assignment and leaves the return output untouched. Ordinary relation
clears inside that hook do not invalidate the captured live Fighter.

Clearing attachment leaves immutable creator, current damage owner, owner_bound,
and Fighter held/auxiliary slots unchanged. Detach clears damage and attachment
relations only; it does not invoke physical release. Auxiliary binding requires
the exact original creator and bank, irrespective of reflection. Missing handles
return false; a matched faulted Item fails explicitly. The still-live destruction
window retains admission, as the native reference/bind behavior did.

The matrix includes 16 direct pickup comparisons, full signed/unsigned handle
widths, default/explicit registration, invalid models and joint indices, initial
and callback-time faults, Item/Fighter retirement and pool reuse, and actor
snapshot restore with stale creator references. JObj pointers rely on engine-owned
typed rig/model lifetime; there is no per-JObj allocator generation proof.
