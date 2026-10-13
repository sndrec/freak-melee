# Synchronous effects between raw joints

`effects.spawn_between(effect, first_part, second_part)` is a Fighter-only source operation returning Void. The effect ID accepts an integral value without narrowing before admission; both raw rig slots are signed Int. The admitted native two-joint ABI currently covers IDs 1167, 1211, 1212 and 1291. Unsupported IDs or malformed arguments fault before native work. These are raw indices, so use `bones.index(role)` explicitly when starting from a semantic role.

Both slots and the creator scale root belong to the selected source rig and remain retained through native dispatch. Negative, out-of-range or NULL slots are healthy no-ops under the canonical raw-part policy. NULL admission avoids the original attachment primitive's invalid NULL-position dereference; no placeholder joint is supplied. Retained receiver projection is unavailable because this operation is SourceOnly.

The operation calls actual synchronous `efSync_Spawn`. Native construction creates the first model, attempts the second only when the first succeeded, then evaluates queued initial model animations in native LIFO order. First allocation absence creates neither; second absence preserves the first. This is neither an async request nor an atomic two-model replacement. Native effects already created remain created if retirement during animation evaluation prevents returning to authored code.

The operation does not set `effects.active`, install pause callbacks or clear accessory callbacks. Authored code retains those decisions. `effects.pause_during_hitlag()` is a separate existing operation which replaces both owned PreHitlag/PostHitlag helper IDs with the shared native `efLib_PauseAll`/`ResumeAll` callbacks.

The focused `tests/native/script_effect_between` cohort compares unchanged native dispatcher/attachment bodies with controlled allocation and animation leaves, and tests compiler/VM routing and retained lifetime rejection. It does not claim complete game or rendering acceptance.
