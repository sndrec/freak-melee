# Air input operations

The connected fixture runs the actual frontend, compiler, catalog, preparation/publication, Fighter and Item actors, Execution, VM, storage and GObj pools for:

- `controls.aerial_attack_motion()` -> I32; the common selector, without the authored aerial mapping hook.
- `input.cstick_attack_edge()` -> Bool; only the native threshold edge.
- `state.air_gun()` -> Void; native Laser Gun, Fire Flower and Super Scope air entries.
- `controls.try_air_item_throw()` -> Bool; native input selection and handled-input result.
- `controls.try_air_special()` -> Bool; the selected owned direction hook must exist before any facing reversal.

Selectors, input gates, air gun entries, throw setup and Dropped continuation are mechanically extracted. Healthy decisions and physical fields compare against independent native bodies pinned to pre-batch `7b077ac85f75a68df8f99feaf97fe8298c7609bc`. This includes threshold boundaries/NaN, both stick sources, facing, ammunition, use category, stick ages, special direction precedence and typed VM results.

Actual interpreted callbacks exercise Fighter/Item faults, detach, physical retirement/reuse, native generation, userdata, classifier, schema and parameter substitution. A successful Item self-removal still handles input when its Fighter survives. Cached Item tails stop; a live Item fault or substituted identity rejects publication. Changed held slots remain legitimate; the retained Item is never reacquired from a recycled slot. The actual common throw/drop functions return an optional retained Item record. A fall callback replacing A with B is observed before drop; B is captured before its Dropped hook and its successful retirement or fault is validated after the Fighter survives. The combined replacement-B fault case and an already-faulted B admission are part of the cohort. A chosen object whose capture fails is distinguished from the empty no-Item outcome without dereferencing or reacquiring it.

The fixture controls imported rig/common motion/Article metadata, motion and animation entry, accessory release, parasol status, Item timer, native drift clamp, facing update, and geometry/render/audio/damage-owner tail leaves. It uses an explicit position/velocity property write observer for synchronous mutation. Unrelated bindings reject. Holdable Dropped is explicitly unavailable: this cohort exercises the actual Article Dropped route, not the Workshop holdable protocol.

The existing Super Scope entry still uses its stock byte-offset `ItemScopeVars` overlay. Comparing that body does **not** prove those offsets match the widened native Fighter; a typed Super Scope state cleanup remains separate. No DAT import, game/render, actual animation/throw release, complete air IASA order or gun RNG acceptance is claimed.

Run hidden isolated consoles:

```powershell
python tests/native/script_air_inputs/run.py --compiler .local/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-air-inputs-final
```

The output must be fresh and on A:. The runner uses `CREATE_NO_WINDOW`, strict warnings, separate host-only runtime state, and verifies all recorded inputs remain unchanged after the run.
