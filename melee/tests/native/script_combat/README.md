# Damage/status/effect/RNG binding fixture

The controlled native Fighter now has real fixed `NativeMatchPool` identity and
scene epoch for effect controls. This fixture has no interpreted Fighter actor;
`pause_during_hitlag` therefore rejects without changing its output/callbacks.
Positive authored/native hitlag replacement is covered by the connected
`script_effect_hitlag` cohort. RNG, damage and hurt-status comparisons remain.

From the repository root:

```powershell
python tests/native/script_combat/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-combat-review
```

Use a fresh output directory on A:. The runner uses hidden compiler/process
creation, strict C11 warnings-as-errors, O2 and disabled FP contraction. No
game opens. Source extraction, commands, hashes, stdout and exits are recorded
in the output receipt.

Production source/index/compiler/catalog/bytecode/VM execute the authored
fixture. Native bodies and RNG come from the actual checkout. The historical
exclusion algorithm is an independent selection oracle. Player bookkeeping,
color channels, JObj traversal and effect destruction leaves are controlled.
Callback bindings use the setup fixture boundary; the real owned callback
registry is separately tested in `script_fighter`.

The test covers no-draw RNG cases and maximum defined bound, Float damage
marshalling, mutable/read-only fields, status caps, both effect lists,
destructor retirement and generation replacement, source context rejection,
and read-only RNG rejection. It does not prove live game or effect-spawning
behavior.
