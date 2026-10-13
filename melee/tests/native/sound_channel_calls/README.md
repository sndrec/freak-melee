# Direct authored sound-channel calls

This console fixture generates both complete action and motion-callback modules.
`compiler.c` emits `sound_channel_action.inc` and `sound_channel_callback.inc`
when passed those two output paths. It checks typed/arity/context rejection and
all seven required Host slots. `module_test.c` links the actual generated code.

The module fixture covers all seven channels, five full-width IDs (including
stop `-1` and IDs above float's exact range), three volume inputs with native
byte narrowing, four invalid integer channels, and written argument order.
Retirement at each of the four argument helpers and at final dispatch must stop
all subsequent source reads/writes. One mode protects both GObj and Fighter
pages with PAGE_NOACCESS; the other recycles the same addresses with a new
generation. Missing Host table, each missing slot and missing liveness query
must reject module initialization.

The seven sound callbacks and basic-landing callback are explicit controlled
engine boundaries. The fixture tests generated dispatch and source lifetime,
rather than replaying the already accepted native mute/stop/mapping matrix.
That separate evidence is `tests/native/fighter_sound_mute_policy`. No audio,
renderer, game process or acoustic equivalence is claimed here.

CMake builds native_script_sound_channel_compiler_test and the action/callback
module tests. Acceptance is recorded only after running the coherent suite
against the matching live Host; frozen SDK65 cannot compile this new ABI.
