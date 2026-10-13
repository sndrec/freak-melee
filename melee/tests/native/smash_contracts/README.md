# Common smash charge contracts

This unregistered fixture validates semantic charge storage and its interaction
with the unchanged common charge update. It does not activate or claim acceptance
of Ness's seven smash states.

The compiler test covers exact property/constant names, writable versus read-only
fields, integral versus floating assignment types, bounded output and model-effect
context rejection. Fighter item notifications remain fighter contexts and can use
these properties. The compiler routes must reject model-effect property reads too.

The console runner uses the supplied immutable SDK/compiler/content tool. It
checks the snapshot manifest and pinned original `ft_0DF0.c`, extracts unchanged
function bodies, generates both action and motion-callback forms, then compiles
actual generated callbacks with Clang and the frozen SDK. No game is launched.

```powershell
python tests/native/smash_contracts/run.py --snapshot .local/baseline41 --output .local/smash-contracts41
```

Each route checks generated initialization and typed reads, 60 value/count cases
for mode/index/period writes, and 240 common update/input pairs. Invalid modes,
negative/out-of-range byte values, and invalid shake counts leave storage unchanged.
Common source keeps normal charge clamping/rate restoration distinct from manual
charge mode 4, preserves the sound latch behavior, wraps the shake index, and
resets only the mode on motion exit. Configuration leaves the existing damage
multiplier, sound latch and unrelated charge storage untouched.

Presentation calls are recorded ordered boundaries: animation rate changes also
update the fighter field, while color, rumble and sound record their arguments.
The source functions run unchanged on generated storage and independently assigned
expected storage; explicit assertions also verify normal/manual branch differences.
This is a storage/interoperability comparison, not a replacement of the common
charge algorithm or a proof of dispatcher timing/rendering.

Root integration: add `tools/native_fighter_script_smash.c` to the normal compiler
source list; register `compiler.c` with the same includes/definitions/compiler
sources as other compiler contract tests. Run the console runner after the matching
snapshot contains `NativeFighterCommon_SmashShakeCount` and all dispatch integrations.
