# Held lifecycle compiler and common entry evidence

This namespace owns only test sources and frozen original evidence. It does not activate a fighter template or run gameplay.

Compiler implementation: `on neutral_enter` is Void with Float `event.blend`; `on air_resume_motion` is Int with Int `event.previous_motion` and implicit return of the original value; `on held_timer_expired` is Bool with implicit false. `article ... on holder_neutral` is an independent Void notification and does not require held-status/request callbacks. The existing status/request pair rule remains intact.

`items.notify_held_neutral()` is a Bool synchronous Host operation, emitted as `engine->notify_held_neutral(gobj, NULL)`, with generated source-generation continuation checks. `items.reopen_blocked` reads/writes the common `x2221_b5` flag using ordinary DSL truth conversion. `event.interrupt_allowed` captures the stored common landing flag at entry, and is available only directly in `on landing_action`; a helper can receive its value through a typed Bool parameter. Helpers and other contexts cannot directly read that union field.

## Frozen original54

`original54.inc` preserves four complete unmodified bodies: neutral entry, air-resume selector, landing entry and `Fighter_8006A360`. `original54_manifest.json` pins full source/body SHA256 and the matching baseline54 freeze manifest. Original mushroom caller source/header files are retained under `original54_callers/`, with source hashes in that manifest. Their propagation is inventory evidence; the test does not claim execution of all eight mushroom entry callers.

The original landing function has two conditional switch openings under native/non-native preprocessing. Extraction uses its actual column-zero closing brace, preserving both original branches; no body text or branch policy is normalized. At comparison time only original function identifiers are renamed to coexist with real current entries.

## Reproduction

Run from the repository root with a fresh output directory:

```powershell
python tests/native/held_lifecycle_protocol/run.py --root . --compiler .local/baseline54/compiler/clang.exe --output .local/held-lifecycle55-root-compiler
python tests/native/held_lifecycle_protocol/run_source.py --root . --compiler .local/baseline54/compiler/clang.exe --output .local/held-lifecycle55-root-source
```

The clang executable is a compiler toolchain; live source/headers are the new55 ABI input. Every receipt hashes those input sources and headers. This is not a claim that54 runtime accepts a55 module. The scoped runner compiles the compiler fixture, generated C for both parser routes, and a separate loaded DLL. No native whole build or game process is launched.

Compiler negatives cover event/landing context, helper context, read-only landing property, non-Int resume returns, notification arity/article context, illegal Void article returns, and scalar properties used as calls. Generated callbacks check exact Float blend propagation, all-bit Int resume defaults, stored landing Bool passing, common flag write/read, independent article notification, Host null rejection and source retirement/recycling stopping the post-notification write.

## Actual source boundaries

`source_fixture.py` extracts three complete current common entries and the exact timer `if (fp->x2221_b4)` block. The timer is placed inside a minimal fixture function that supplies the already-live source, captured generation and a tail marker. It is not a replacement tick or a full `Fighter_8006A360` test.

The real `native_callback_lifetime.c` Fighter/Item registry and dispatch-depth implementation is linked. Model effects are explicitly absent in this console fixture. Definition lookup, ground setup, motion change, camera selection, item neutral dispatch, cleanup, and selected fall/throw calls are controlled dependency boundaries. The fixture compares typed decisions, callback order, state changes and exact Float bits. It does not compare struct padding or mask semantic digests. New authored callback contracts and source retirement/recycling/kind/Definition safety are counted separately from original54 healthy pairs.

Root owns separate actual held-dispatch/provenance tests. A controlled `NotifyNeutral` boundary in this fixture must not be presented as another test of the real author/held-item dispatcher. Full animation, item cleanup internals, item transitions, mushroom caller propagation and gameplay remain outside this source fixture.

## CMake integration recipe (root-owned)

- Compiler target: `compiler_test.c` + `NATIVE_FIGHTER_SCRIPT_SOURCES` + actual `native_fighter_parameters.c`; argv: source `fixture.fms`, action generated header, base generated header.
- Generated action module: `module_test.c`, include binary output directory containing `held_lifecycle_fixture.h`.
- Generated base module: same actual test source with `LIFECYCLE_BASE=1`, including `held_lifecycle_base_fixture.h`.
- Loaded DLL: compile generated action C as a DLL; compile the test source with `LIFECYCLE_DLL=1` and pass the DLL path.
- Source extraction: Python `source_fixture.py --root <repo> --output <binary fixture dir>`, outputs `original_entries.inc`, `candidate_entries.inc`, `source_extraction.json`. Dependencies: script, frozen original manifest/inc and current `ft_08A1.c`, `ft_0D27.c`, `ftCo_Landing.c`, `fighter.c`.
- Source target: `source_test.c`, `effect_absent.c`, actual `native_callback_lifetime.c`; native defines, source/Dolphin includes, generated fixture include directory. No additional engine libraries.

Worker console results: 73 compiler checks, 106 action-module checks, 106 base-route module checks and 110 loaded-DLL checks; 1,661 original54 healthy pairs, 20 authored contract cases and 46 safety cases across 41,471 actual-source checks. The landing safety suite includes removal/recycling/kind/Definition changes inside stock cleanup, checked before the authored callback. The source suite also checks the real64-dispatch limit and balanced restoration.

Final worker receipts: `.local/held-lifecycle55-worker-receipt/result.json` and `.local/held-lifecycle55-source-receipt/result.json`. These are source/console evidence; root full-build/grouped-check/frozen55 acceptance remains a separate gate. Earlier diagnostics remain separately preserved in `.local/held-lifecycle55-*`.
