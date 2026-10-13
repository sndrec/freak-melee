# Actual companion special comparison

Runs the real native game with rendering enabled, hidden and muted, once with
stock Ice Climbers and once with an installed interpreted Popo/Nana family for
each of the four grounded specials. No desktop input is sent. Each game consumes
360 synthetic controller samples; up-B includes rope creation, scheduling and
ordinary return. Only task-owned children are killed on the 180-second deadline.

Supply a private accepted family store and its source draft. The runner checks
both draft scripts and the packet against the current full canonical sources.
It records commands, hashes, exit/EOF/fault status, both actors' action timelines,
every emitted Fighter scalar/geometry field, visible Item fields and separate
detailed Item digest differences. It requires 360 Nana trace records per run.
The initial motion ranges follow `ftPopo/forward.h`; Belay begins at 347.

```powershell
python tests/native/source_companion_game/run.py --exe build/native/melee_native.exe --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso" --content-root PRIVATE_STORE --source-draft PRIVATE_DRAFT --package CONTENT_ID --output A:/FreakMelee-test-archives/FRESH_DIRECTORY
```

The generated input uses the native replay controller file format, but runs via
`--replay-inputs`: its signatures and expected state/RNG digests are deliberately
unset. This is a stock/source runtime comparison, not deterministic replay or
rollback acceptance. Detailed Item hashes may include different storage policy;
those differences remain visible in the receipt instead of being silently
normalized. Injury, interrupted/airborne specials, separated/dead partners,
match end, image fidelity and whole-match save/load require other checks.

Use `--scenario cpu-injury` for the longer capture regression: 1,200 samples,
three special attempts and a level-9 Mario opponent. All four cases must record
damage; neutral/up must also record a capture entry on P1 or its companion.
Both runs must reach controller EOF without faults and have matching action
timelines and emitted Fighter/visible Item fields. Tracing can stop before EOF
when the match enters results, so this mode requires equal nonzero companion
trace counts rather than assuming the entire playback remains in gameplay.
See the [capture/results checkpoint](../../../docs/native_source_companion_capture_results_checkpoint.md)
for the actual reproduced failures, fixes, results images and remaining limits.
