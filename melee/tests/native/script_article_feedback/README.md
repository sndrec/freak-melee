# Owned Article feedback contracts

```powershell
python tests/native/script_article_feedback/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-article-feedback-review
```

Fresh A: output, hidden subprocesses, stable input hashes. No game/global build.
Actual source compiler/catalog/VM/Item actor/view/storage and GObj pool lifetimes
are linked. Entity publication/specs, model allocation backing and particle/render
leaves are controlled; no successful authored native behavior substitutes the VM.
JObj/class initialization and ancestry checks use extracted actual native bodies.
The root allocator is a fixed fixture pool, not an asset import.

spawn_root_effect admits IntegralAny IDs only if the production classifier proves
exact Joint ABI; no F64 conversion or low-word truncation. The native call receives
exactly the source GObj and its current root. Source IDs1272/1273/1274 forward through
actual efLib_CreateGenerator_Attach_Scale (one JObj), retaining generator-root
association and reading source root scale. Current registered construction hook
only installs later callbacks; the default-shape hook is NULL. This does not
promise safety for arbitrary future synchronous construction hooks.

destroy_effects invokes actual efLib_DestroyAll; source membership, logical status,
spec/program/view/schema and root/class are checked before and after both calls.
Destroyed callbacks may operate while storage is still live. Creator presence is
not required. Missing or mismatched roots reject before effects. Engine root
obj_kind/class/obj_size establish typed extent; HSD has no exposed JObj allocation
span/generation query, so this does not claim arbitrary dangling-pointer detection.
Physical/logical retirement, fault, root/class and schema replacement preserve
outputs and stop following VM statements. Effect/render appearance and full Item
constructor/scene parity remain separate acceptance.

Final result: 282 checks, 13 actual VM-to-native spawn invocations. Article and
Workshop actors admit1272..1274 without a creator. Scalar/Position/unknown IDs,
floats, negatives and oversized integers reject pre-spawn. Both real effect-list
destruction loops and parent-root cleanup are exercised, with controlled cancel
reentry; following authored state writes stop on failure. HSD_New/JObj allocation
and initializer/ancestry source execute with fixed allocator backing.

## Position effects

The source compiler and actual Item actor VM invoke the position operation for
all 63 native Position IDs, both with a root and without one. The source-derived
dispatch corpus separately verifies context requirements: five IDs read root
scale and one additionally installs a fighter-parent callback. Item admission
rejects that fighter-only ID and missing roots where required; root-independent
particles and model effects accept a rootless owned item. The controlled spawn
leaf copies the exact stack Vec3 argument, not a JObj argument. It does not
execute the full particle dispatcher or renderer. The companion
`script_position_effect_abi` fixture executes native position-copy helpers.

Source retirement, schema/model replacement, logical fault and native deletion
stop following VM statements. Root-independent effects intentionally do not
inspect the root's class. Oversized/negative/float IDs, wrong ABI and non-finite
XYZ reject before any native spawn. Original root-effect and destruction cases
remain included. Grouped result: 1,465 checks and 140 VM-to-native spawn calls.

## Position plus scalar effects

The current matrix additionally runs an authored reflected callback through the
real Item actor and interpreter for all audited PositionScalar recipes, with and
without a root. The controlled spawn leaf captures the exact stack Vec3 and
Float in their native argument order. Fighter-parent recipes are rejected;
root-reading recipes require an admitted root. Non-finite coordinates/scalars,
wrong ABI/type/width/count and retired sources reject before spawning. Logical,
physical, schema and model changes during spawn prevent subsequent state writes
and result publication. The receipt supplies current grouped counts; the earlier
counts above describe the preceding Position-only matrix. Unselected bindings
return failure, and no particle appearance or rendered gameplay is claimed.
