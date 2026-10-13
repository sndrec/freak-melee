# Shared initialization policies

Generic authored configuration replaces the Ice Climbers' stock-ID exceptions:
ordered costume/bank sound-ID ranges, costume KO cues, surface friction, and
strict Bool statements for the existing healing-pickup/companion-Sleep flags.
No sound callback or allocation is added to gameplay.

Four console CTests build from the normal CMake compiler source list: declaration
and invalid-input compilation, actual generated action and motion routes, and
complete actual old/current sound resolver/KO/friction bodies. The original bodies
are extracted from immutable SDK71 and committed with their source SHA manifest;
the build does not require a private baseline directory. The current bodies are
extracted directly from the live source. Definition publication, surface query,
audio canonicalization/bank lookup and ordinary fallback sound mapping are
controlled boundaries. No audio backend, game, rendered match, whole-family
lifecycle or removal-during-Sleep acceptance is claimed.

```powershell
cmake --build build/native --target native_script_common_init_compiler_test native_script_common_init_action_test native_script_common_init_state_test native_fighter_common_init_source_test
ctest --test-dir build/native -R '^native_(script_common_init_|fighter_common_init_source)' --output-on-failure
```

The optional `fixture.py --reference PATH` reproduces the old extracted bodies
from the actual immutable reference SDK. Without it, the committed originals are
hash-checked. Both generated modules retain the source base unchanged, exercise
all 32 costume mask bits, preserve other banks/IDs and reject invalid declarations
and overlapping KO masks. Actual source pairs additionally retain the original
canonicalization/query order and cover both ordinary and ignore-surface policies.
