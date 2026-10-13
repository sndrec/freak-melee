# Ground special checks

These Fighter-only, zero-argument operations return Bool:

| Operation | Native check | Cached input age |
| --- | --- | --- |
| `controls.try_side_special()` | `ftCo_SpecialS_CheckInput` | `x688` |
| `controls.try_up_special()` | `ftCo_Attack100_CheckInput` | `x686` |
| `controls.try_neutral_special()` | `ftCo_800D6824` | `x689` |
| `controls.try_down_special()` | `ftCo_800D68C0` | `x687` |

The checks use the native cached input age being zero. The shared input counter
updates those ages from B/stick input before arbitration; these operations do
not replace that counter with a fresh raw input test.

A handled input dispatches the corresponding authored `special_s`,
`special_hi`, `special_n` or `special_lw` entry synchronously. The script controls
ordering and move decisions. Side entry also preserves its native facing
threshold and ground momentum retention, including surface friction selection.
The attached source does not delegate to a stock character's special handler.

The existing ground kernels deliberately remain unchanged: an attached source
with a missing entry hook still reports true when its age is zero. The hook is
a no-op; side entry can still reverse facing and adjust ground momentum. This
differs from `controls.try_air_special()`, which checks hook presence before its
facing changes. Authors that call these ground checks should provide the entry
hooks they intend to expose.

The caller must retain its exact live source Fighter, program, owned storage,
motion layout and scene lifetime. A legitimate motion change inside the entry
hook is allowed. If nested authored code faults or changes the retained source
identity or metadata, no Bool output or following script continuation is
published. Native mutations already performed are not rolled back.

The connected `tests/native/script_ground_controls` fixture executes the actual
generic kernels, compiler, Prepare/Publish, Fighter actor, nested VM, storage
and fixed GObj pools over controlled engine leaves. Its grouped comparisons
cover the four checks alongside the existing ground controls. No game, audio
or renderer launches, and complete Link/Young Link special behavior remains
outside this proof.
