# Synchronous effects between bones

`effects.spawn_between(effect, first_part, second_part)` creates a synchronous effect spanning two bones. It returns Void. Supply an integer effect ID and two signed Int [bone IDs](bone_queries.md#bone-ids) belonging to the fighter. Use `bones.index(role)` to get a named role's ID. The supported two-joint effects have IDs 1167, 1211, 1212 and 1291. An unsupported effect ID or malformed argument produces a script error.

Both bones belong to the source fighter. A missing bone produces no effect. Call this from the fighter's own callback.

The operation calls actual synchronous `efSync_Spawn`. Native construction creates the first model, attempts the second only when the first succeeded, then evaluates queued initial model animations in native LIFO order. First allocation absence creates neither; second absence preserves the first. This is neither an async request nor an atomic two-model replacement. Native effects already created remain created if retirement during animation evaluation prevents returning to authored code.

The operation does not set `effects.active`, install pause callbacks or clear accessory callbacks. Authored code retains those decisions. `effects.pause_during_hitlag()` is a separate existing operation which replaces both owned PreHitlag/PostHitlag helper IDs with the shared native `efLib_PauseAll`/`ResumeAll` callbacks.

The focused `tests/native/script_effect_between` cohort compares unchanged native dispatcher/attachment bodies with controlled allocation and animation leaves, and tests compiler/VM routing and retained lifetime rejection. It does not claim complete game or rendering acceptance.
