# First production native binding family

`native_script_bindings.h/.c` provides an immutable semantic catalog and direct
IR resolver. IDs occupy the native domain below `0x10000000`; namespaces and
members resolve from their explicit parent IDs. Unknown names, contexts,
selectors, indexes and signatures fail. No native field offsets or callable
machine addresses enter the program IR or bytecode.

Descriptors provide result/formal argument types, access/effect flags, argument
modes and output publication policy. `DescribeCall` supplies the shared
bytecode call layout. The explicit int/uint/float conversion intrinsics use a
validated NumericAny input and an Invalid formal-type sentinel to preserve the
input's actual integer bits rather than passing them through Double.

`native_script_binding_properties.c` executes the source-checked fields in
`docs/native_script_bindings.md`. Fighter and article position/velocity share
semantic IDs with context-specific native storage. Fighter-only properties
cannot read item storage. Article `facing` and fighter `facing.direction` remain
distinct. Bool reads normalize masks/bitfields, Bool writes are typed and
preserve surrounding bits. `airborne` changes classification alone, without
calling ground/air transition helpers.

The opaque native runtime source context carries object class and full creation
generation. Actual callback registry membership/generation is checked before
native storage access. Reads/intrinsic results publish only after a valid source
check. Failed reads, wrong family/type, unsupported selectors and retirement do
not publish a neutral value or silently execute another binding.

The initial math family directly uses the corresponding native scalar math:
Float abs/sqrt/sin/cos/min/max, bit-based signbit, shared bounded angle wrapping,
and explicit scalar conversions. Fighter/effect `atan2` uses Double as in the
existing fighter expression path; article `atan2` uses Float. `atan2_float`
explicitly uses Float. `radians` preserves the existing Float factor with Float
or Double multiplication selected by the argument type. Pi/tau/pi_half are
Double constants. These are generic native primitives, not authored handlers.

The console test compiles actual Fighter/Item types, the production source
registry, direct resolver, typed values and native binding units. Across 257
field-value variations it verifies context-specific storage, all vector axes,
Boolean normalization/adjacent bits, read/write permissions, frame truncation,
full-width source generations, explicit conversion bits and Float/Double math
behavior. Removed-source data is poisoned to prove membership is checked first.
The effect registry is a controlled absent-object boundary.

Common field coverage adds terminal velocity, smoothing, separate ceiling/wall
normals, ECB offsets, instance/native model scale, unsigned spawn identity and
pickup policy. Model scale and facing sign compare against extracted shared
native getter bodies. A direct source/bytecode/VM slice exercises the new fields
and pickup assignment; nil common data, wrong class, readonly writes and retirement
retain output-failure semantics. The configured CMake fixture source list is used
by the standalone runner so unavailable families match the actual test target.
The grouped jump-observation checks read reverse/tap-jump/downward-action
thresholds and the Fighter horizontal jump multiplier through the actual catalog,
common-properties backend and VM. They preserve F32 bits for signed zeros,
finite values, infinities and NaN; enforce Fighter-only/read-only access; and
reject missing common data or retired sources without changing outputs.
An exact extracted `getFloatDir` body provides the signed comparison reference,
including equality's backward branch. This uses native field types and controlled
registered objects, not the full Peach handler, asset loader or game scene.

Rendered `model.root_scale_y` compiles and executes through the same actual
frontend/VM/property path. Its F32 bits compare with the extracted native root
getter for base and derived JObj classes, including NaN, infinities and signed
zero; a live absent root returns1. The actual extracted class ancestry routine
admits controlled class metadata. Unrelated/null/undersized classes, wrong
object identity, generation retirement and mutation during class initialization
reject with unchanged output. No pinned-program equality is required, and the
descriptor remains available to generic Fighter receivers. This fixture does
not execute the receiver pool or construct an owned renderer graph; its
registered Fighter/JObj metadata and class-initialization mutations are controlled.

Two additional direct FMS bodies compile through source, statement, typed
bytecode and production VM units into real Fighter/Item field reads/writes.
They check Nana-style Double interpolation before Float storage, math arguments,
short-circuit Bool state and distinct article facing. Retired sources stop before
any subsequent write/result publication, and VM frame/register pools unwind.

```powershell
python tests/native/script_native_bindings/run.py `
  --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe `
  --output A:/FreakMelee-test-archives/script-native-bindings-example
```

All outputs stay on `A:` and subprocesses use `CREATE_NO_WINDOW`. The fixture
suppresses the existing duplicate MTXPerspective array-bound declaration warning
in the Dolphin header and the catalog's existing omitted trailing-zero aggregate
initializer warnings; the new units retain the other strict warning checks.
This is bounded native binding acceptance. Complete engine catalogs, program
loading, bytecode/game behavior and rollback acceptance remain separate gates.

The common effect-size observation executes the production parser, bytecode,
VM and property router. Its scalar is the first float of the common effect
attribute block, independently of instance, common model and rendered-root
scale. Non-Fighter reads and writes to this read-only property are rejected.
No effect constructor or rendered Yoshi roll is exercised here.

Horizontal smash-input history is writable through the real property router.
All 255 valid values are read back for signed/unsigned 32/64-bit arguments;
the entire Fighter is compared to detect writes to neighboring input state.
Pre-conversion bounds reject wide integers that could wrap, malformed Bools,
floating values, wrong actors/selectors and retired generations. The actual
frontend/VM also executes the explicit history reset. This fixture does not
run the native per-frame input scheduler or a rendered egg roll.

Cached ECB observations expose X/Y on all four vertices: top, bottom, left and
right. The suite compares all eight read-only properties with actual collision
cache fields, including signed zeros, subnormals, infinities and NaN payloads.
Distinct neighboring values detect component mix-ups; whole Fighter comparisons
detect mutation. Actual frontend resolution rejects Article/Effect contexts,
and runtime checks reject writes, selectors and stale generations. This is a
cached-geometry read contract, not collision solver or wall-bounce acceptance.
