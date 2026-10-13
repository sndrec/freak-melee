# Interpreted fighter callback fixture

`native_script_fighter_test` compiles FMS directly, publishes a kind program,
attaches an actual registry fighter and executes production bytecode/storage.
The build mechanically extracts complete current damage/hitlag dispatch bodies
from `ftcommon.c`, `fighter.c` and the common special input files; those bodies are not rewritten.

191 assertions cover all eight instance slots, wrong signatures/events, immutable
program pinning, shared native assignment, source-matching reset masks, respawn
identity with persistent variables, one-shot state change motion ownership and named state/phase aliases, all eight original special
input routes, and actor retirement
inside damage/pre-hitlag callbacks. Retirement prevents subsequent death1 delivery
and hitlag flag mutation. Engine cleanup/presentation primitives and one native
position-write boundary are controlled; no asset or all-roster claim is made.

Packed snapshots include declared root variables, program/layout/parameter IDs,
actor slots, clocks/faults, motion functions, instance callback IDs and GObj pool
references. They restore retired actor views after peer ownership is restored,
rebuild internal execution/storage pointers, and reject truncated records, wrong
identities, callback/phase mismatches, stale peers and scene epochs before writes.

The optional live benchmark check `FREAK_MELEE_SCRIPT_SNAPSHOT_CHECK=1` compares
captured owner records to the broad restore, deliberately reloads later script
state, rewinds it with the packed record and checks the resulting full replay.
This is a script-owner migration diagnostic; other match state still uses the
broad oracle. It does not select a complete explicit snapshot backend.

```powershell
cmake --build build/native --target native_script_fighter_test
ctest --test-dir build/native --output-on-failure -R '^native_script_fighter$'
```
