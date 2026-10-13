# Typed form presentation/transition primitive catalog

These are generic Fighter-only Void calls with native invalidation flags:

| Surface | Formal types |
| --- | --- |
| `effects.spawn_on_part(effect, raw_part)` | Integral effect (raw NumericAny), Int |
| `effects.wind_at_position(x,y,z,frames,strength,decay,frequency)` | Float, Float, Float, Int, Float, Float, Float |
| `state.enter_at_speed(motion, flags, frame, rate)` | Integral motion (raw NumericAny), UInt, Float, Float |
| `effects.spawn_with_scalar(effect, semantic_part, value)` | Int, UInt, Float |
| `effects.spawn_on_joint_with_scalar(effect, raw_joint, value)` | Int, UInt, Float |

Raw joint indices and semantic parts remain distinct. Scalar values are typed
numbers; no script native pointer is introduced. Their actual native effect ABI
admission belongs to the Effect/Feedback implementations; scalar copied-local
lifetime belongs to Feedback.
Wind coordinates are copied scalars, independent of the existing bone/facing
offset operation. State entry preserves flags/frame/rate explicitly; animation
startup is separately authored and is not added by the catalog.

The motion formal follows existing entry calls: no float motion is allowed,
and its integral value reaches native transition validation without narrowing
through I32/Float. The runtime enforces the actual owned motion domain.

Explicit dispatch routes canonical `spawn_on_part` to Effect, the other effect
calls to Feedback, and state entry to
transition before broad enum categories. No aliases, case-specific joint mapping
or character shortcuts are added.

Run `run.py --compiler <MSVC-target clang.exe> --output <fresh A: folder>`.
`A:/FreakMelee-test-archives/script-form-primitive-catalog-oct08-scalar/receipt.json`
passes **56 checks** against the actual catalog, typed statement/IR, bytecode
and top-level Invoke dispatch. It covers exact formals, invalid Float index/
frame-count/motion arguments, wrong source context and preserved argument/output
bits on explicitly unavailable native leaves, including a wide U64 motion.

This fixture does not execute native feedback/transition, validate an effect
graph or claim package/render/game acceptance. Runtime workers separately test
actual engine signatures, source lifetimes and transitions. No authored scripts
are rewritten by the catalog worker.
