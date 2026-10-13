# Explicit C oracle construction

The reference-only generator takes an optional NativeFighterScriptConstruction with an explicit fighter root, first motion and owned count. It checks that tuple before emitting C. NULL selects the special/action-only reference mode; a checked tuple selects owned-state reference mode. No base declaration is parsed, inferred or accepted by the source frontend.

All 65 GenerateC call sites were adjusted directly. Mixed action/state fixtures pass the corresponding explicit tuple; standalone exports and special-only cues remain in their original mode. State/action CMake comparison commands supply their fixture tuples directly. Four ordinary Peach/GW/Popo/Nana templates no longer carry redundant construction hints. Their actual sound/friction/visibility and common motion policy declarations remain.

The hidden sampler compiles the actual oracle dependencies and executes twelve existing mixed-mode source/compiler fixtures. CLI cases check explicit tuple emission and failed-request output preservation. Ordinary lifecycle action containers work under explicit construction. Popo/Nana use two named moves, IceShot and Blizzard, retaining all four special entry callbacks; an out-of-table action phase rejects under the same bounds. Special entries remain restricted to named moves in the direct frontend. This does not introduce named-move aliases or loosen its one-entry-pair rule.

This is C comparison-oracle parity, not direct-VM/game acceptance. The owned recipe and common-motion-policy stages have separate actual DAT/prepared-program evidence. All output is isolated under a fresh A: directory and subprocesses use CREATE_NO_WINDOW.

Run `python tests/native/script_oracle_construction/run.py --compiler <clang.exe> --output A:/FreakMelee-test-archives/<fresh-name>`.
