# Common Catch callback dispatch source fixture

Run from the repository root with a matching frozen SDK/compiler:

```powershell
python tests/native/catch_dispatch/source_test.py --snapshot .local/baseline44 --baseline .local/baseline43 --output .local/catch-dispatch44-fresh
```

Use a fresh output directory. `--prepare-only` extracts and hashes the actual function definitions without compiling or running. `--source` can select the checkout containing the modified production bodies. The runner verifies both immutable SDK/compiler inventories before and after execution, and records every source input and command in `receipt.json`. No game, global build, package activation or production mutation occurs.

The fixture extracts whole definitions from Catch, CatchPull, CatchWait, CaptureWait and the original Samus input veto, renaming only function identifiers to run both source versions together. Its extractor requires a real bool/void definition prefix, preventing an `if (function(...))` call site from being mistaken for a definition. Production `native_callback_lifetime.c` is linked directly; actual Fighter and GObj layouts, inline accessors and callback pointers are used.

Coverage includes absent/empty Definition fallback across five fighter kinds; held-item throw priority before authored eligibility; rejected input and countdowns; standing/dash animation replacement; Catch entry and exit; CatchWait pummel/directional-throw short circuit; frame selection, anchor binding and optional follow ordering; ready notifications; capture setup/entry; and the original common no-follow accessory policy for Link, Young Link and Samus. The ordinary follow body is actually executed for a generic fighter, including hidden victim, distance cut and clamped ground movement branches.

Hook tests exercise both Bool results and removal, same-address generation replacement, kind changes, Definition changes and retained-victim unlink/replacement. Separate VirtualAlloc pages become PAGE_NOACCESS after retirement, so a cached GObj or Fighter read faults. Recycled objects are filled with poison and compared afterward to detect writes into replacement state. Optional and already-unlinked victim cases are included. Nested capture setup is tested through the actual captured entry functions. Motion-entry and accepted pummel/directional-throw retirement are controlled boundaries followed by actual production continuation guards.

A separate child process executes frozen43's existing frame-hook retirement path; the runner requires its access-violation rejection as a negative control. Error dialogs are disabled in that console process. This verifies that the continuation fixture can detect the pre-guard defect rather than merely accepting both implementations.

Controlled boundaries: active Definition lookup; motion engine internals; the three held-item throw-priority operations; legacy Link/Samus schedule/teardown functions; article retract operations; animation availability/rate application; capture registration; pummel/directional-throw engine decisions; world-transform outputs and semantic bone mapping; async effect allocation; common hurt/capture flag setters; Yoshi enclosure internals. Full original decisions around those boundaries are retained. This is source dispatch/continuation evidence, not complete move, engine-constructor, renderer or gameplay acceptance.

Original Link/Samus Pull anchor entry with a null native article dereferences null; those undefined configurations are excluded from fallback comparison. Authored anchor replacement, missing victim and unlinked victim have separately defined coverage. No source UB is imitated by invented layouts or stock-handler delegation.

Frozen44 scoped acceptance: 154,694 cases, 2,930,526 checks and 175 lifetime cases at `.local/catch-dispatch44-ready/receipt.json`; the frozen43 negative control exits with `0xC0000005`. The rejected native pummel/directional-throw paths are pure input reads and cannot retire a caller; retirement is tested only on their accepted transition paths. Both Bool results remain covered for authored hooks. Generated headers, executable, logs and receipts stay under the requested `.local` output.
