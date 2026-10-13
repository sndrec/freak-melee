# Normal attack engine source contracts

Owned source fixture files are `source_fixture.py`, `source_test.c`,
`run_source.py` and this document. The adjacent compiler/module fixtures are
separate root-owned evidence.

```powershell
python tests/native/normal_attack_contracts/run_source.py --root . --original-sdk .local/baseline57/sdk --candidate-sdk .local/baseline58/sdk --compiler .local/baseline58/compiler/clang.exe --output .local/normal-attack58-source
```

The extractor mechanically preserves actual immutable SDK57 and SDK58 source
bodies, changing identifiers only to allow both versions in one translation
unit. It records source/body hashes and line numbers. All forward declarations
come from the actual selected signatures. The console executable links the
real current `native_callback_lifetime.c` and checks full-width creation IDs.
No game, DLL or global build is launched by this runner.

Healthy comparisons cover jab startup and pickup priority, raw jab/rapid/down
tilt/aerial family dispatch with policies absent, rapid entry/update, landing
initialization, and normal attack held-item throw/swing/gun priority. Authored
dispatch, package motion remapping and callback removal/recycling/kind or
Definition changes are separate current-source contracts. Invalid remaps keep
the ordinary selected aerial motion; a declared policy suppresses raw fallback.

Pickup, motion publication, animation start/length/rate, private retail-family
entries, item-use classification and stick edge/angle queries are controlled
engine dependencies. Startup field values and ordered calls are compared
individually, including exact float bits; no private union/padding hash is used.
Old bodies are never executed after source invalidation. These tests do not
establish generated-script or gameplay parity; root's adjacent fixtures cover
the compiler/module layer. A healthy pair does not compare the return value of
old `checkAttack11` (which returned void); the new Bool result is separately
checked against pickup success, with the original startup fields/order compared.

The final matrix contains 324 original/current pairs, 8,508 checks and 164
separate authored-policy/safety cases. Directional aerial selection checks all
five common motions with both ordinary and C-stick input, package remap lower
and upper boundaries, negative/huge invalid IDs and selected-motion retention.
Safety covers all four removal/recycling/kind/Definition changes after the
relevant new jab/rapid/landing/aerial cached continuation boundaries. A full
high-bit generation `0xFEDCBA98` is used throughout.

Known pre-existing boundary: `doAttack12` and `doAttack13` call
`on_jab_restart` and then continue without checking source generation, kind or
Definition. Healthy no-policy raw restart dispatch is compared here; invalidation
from that existing hook is intentionally not called a passing safety contract.
Root will handle the two real sites in a separate follow-up after SDK58. This
fixture does not assert that all normal-attack continuations in the engine are
protected.

Results and explicit coverage counts are recorded in the fresh output's
`result.json`. All source/header/compiler inputs and generated outputs are
hashed so live candidate source evidence remains reproducible.
