# Owned held-open state entry

Connected fixture for bounded `motion.id` writes and `state.held_open(blend)`. The write changes only the native current ID, without a transition. Entry preserves previous-ID capture, actual held-motion hook selection, motion/animation call ordering and common timer/flags.

The actual frontend/catalog/Prepare/Publish/Fighter actor/Execution/VM/storage/GObj pools, held selector and mechanically extracted common entry execute. The native reference is the complete pre-batch entry from HEAD `45ac45dce`; healthy calls use the same actual selector. The copied reference and source hash are retained with each receipt. Motion change and animation are controlled call boundaries, imported asset/rig/package lookup and Fighter payload storage are controlled, and unrelated engine methods reject. This does not prove rendering, whole transition/animation execution, full Peach or actual Item request kernels.

The cohort covers 864 paired entry combinations: default/custom/invalid-return selection, three previous IDs, all eight flag combinations, three fractional/signed common timers and four exact blend values. Four separate field-write pairs prove no transition occurs on assignment. Actual source callbacks observe the previous motion and requested common motion, with source-side callback counters proving both VM executions per pair. An independently compiled bank without a held-motion hook uses the common motion. `vm_calls` records 1,732 explicitly checked invocations; extra reached fault-selector calls are not added to that count.

Admission cases reject wrong context, generation/epoch/class, owned table/schema substitution, missing/nonconvertible common timer data and malformed inputs without changing native fields or output sentinels. Fault/retirement/substitution during selector, motion and animation stages stop later stages and timer writes. Motion entry and animation remain controlled boundaries, so the cohort makes no actual held-Item request/cleanup or dynamic animation admission claim. Held-slot changes are not frozen by this backend: the entry caches no Item, and real native held access retains the instance selected at each access.

```powershell
python tests/native/script_held_state/run.py --compiler .local/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-held-state-final
```

Requires fresh A: output; subprocesses are hidden, warnings strict and all inputs fingerprinted.
