# Source Article capsule and contact fixture

The actual source compiler, descriptor catalog, VM, Item actors, storage and
GObj pool execute. Capsule methods operate on Item.x5D4_hitboxes; contact reads
use Item.x378_itemColl and native masks. No Fighter layout is reinterpreted.

Native damage setter, both disable methods, capsule frame updater, scaled damage
math and stale-move math preserve mechanically extracted native statements.
The setter adds only the explicit test completion observation described below.
Player stale
table input and common scale data are controlled. The geometry leaf uses a
controlled joint translation instead of constructing a rendered model/world
matrix. Active capsule joints have pointer-presence validation only: native
engine attachment joints are legitimate, and no owned-graph/dangling-pointer
proof is claimed.

Damage retains native U32 conversion, current-owner scaling and stale damage;
undefined float conversions fail explicitly. In the scaling branch only, the
actual pure scale calculation is repeated before the real setter to validate its
F32 result can convert to U32. Defined infinity with zero scale factor remains
allowed. Radius uses F32, accepts finite nonnegative values including -0, and
ignores negative/nonfinite values as the emitter did. Disabled capsule radius is
still readable/writable. Single disable updates remaining native active slots;
all-disable preserves exact native flag behavior.

Mutations are injected at an explicit completed-native-setter observation, after
every actual native field write and before the binding's post-call guard. These
pure kernels have no authored callbacks; the fixture does not invent one.
Unrelated native operations explicitly reject. No DAT constructor/game/runtime
package-loading acceptance is claimed.

```powershell
python tests/native/script_article_combat/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-article-combat-review01
```

Use a fresh A: output directory. All subprocesses use CREATE_NO_WINDOW.

The matrix checks every 16-bit Item contact flag combination, all five capsule
states, disabled-capsule radius and F32 edge cases, 64 native damage scale/stale
comparisons and 20 native single/all-disable frame comparisons. It includes
fractional/negative/64-bit defined damage conversions, undefined conversion and
scale branch rejection before mutation, missing geometry joints, index bounds,
invalid/faulted caller, and post-setter source/owner retirement, data/kind/native
generation changes and pool reuse with unchanged failed outputs.
