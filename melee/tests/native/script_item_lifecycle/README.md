# Owned Item expiry, physics refresh and removal

Run from the checkout:

```powershell
python tests/native/script_item_lifecycle/run.py --compiler .local/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-item-lifecycle-final
```

The runner uses CREATE_NO_WINDOW for compilation and execution, bounded console
processes, fresh A: output and input SHA-256 verification. It launches no game.

Actual source frontend/metadata/program preparation, bytecode, VM, Item actor,
GObj/proc pools and packed actor snapshot units execute alongside mechanically
extracted current `it_802750F8`, `Item_802697D4`, `Item_80269978` and removal/
state/cleanup bodies, plus actual common expiry/manual/countdown helpers. Tests cover healthy arithmetic/callback order, hitlag,
accepted state/owner changes, physics/collision faults and removal, replacement
of native model/data/generation or owned bank/schema, output preservation,
reentrant removal from Destroyed and actual deferred GObj proc deletion.

Owned storage remains writable during Destroyed. The removal operation stops a
reentrant Destroyed VM by retiring its still-matching storage, without repeating
native cleanup or touching replacements. A deferred physical GObj retains its
program pin until userdata cleanup, but has no callable owned storage.

Asset descriptors, native geometry/animation/graphics/audio/effect leaves and
publication lookup are controlled. PositionX writes inject synchronous native
removal, errors and peer changes through an actual VM host call. No substitute
actor Run/metadata success boundary is used. Packed actor roundtrips explicitly
restore the native Item peer; they do not establish a complete match snapshot,
retail DAT/render behavior or gameplay acceptance.

The related legacy callback-engine fixture now binds its four fixed GObj
payloads to the real NativeMatchPool implementation. Its absent source actor
lookups return false/NULL and unselected execution leaves abort. Historical
Dr. Mario results and demo presentation runners still extract their explicit
frozen SDK input; they are unchanged native reference evidence.

The expiry group compares complete native Item bytes between VM calls and the
actual shared helpers across ten timer inputs (signed zero, negatives,
fractional/ordinary values, infinities and NaN), including exact common timer
overwrite, unchanged half-life, manual-world flag and Bool countdown policy.
Missing common data rejects before writes and stops VM continuation. Context,
scene/generation and fault negatives preserve output and native Item bytes.
Timer snapshot roundtrips restore the native Item peer explicitly.
