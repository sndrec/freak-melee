# Direct owned motion transitions

The fixture compiles `probe.fms` as a complete transactional program bank with an
owned `NativeScriptMoveLayout`, including indexed events, parameter defaults,
typed bodies, contextual lowering, bytecode, execution records and owned storage.
Its layout and controlled selected source package/bank agree on the eight installed
special-state slots. The numeric table is controlled fixture data, not a claim
that an arbitrary fighter's actual DAT layout has these slots.

Every positive case executes authored source through the actual
`NativeScriptExecution`, VM, bindings, storage and native fighter-list lifetime
checks. Tests observe fighter state and the motion installation boundary rather
than inspecting instructions. Coverage includes:

- Contextual `state.to_air()` and `state.to_ground()` use the move's owned targets
  and U32 flags, preserving the current animation frame.
- Named `state.enter(NeutralSpecial)` chooses its ground/air callback context,
  enters at frame zero and performs startup animation. Numeric `enter` starts
  at frame one; its frame/rate overloads use their authored values.
- Explicit helper switches, `enter_flags`, `enter_at` and `begin` preserve their
  respective frame/startup contracts and unsigned bit 31.
- Missing layout/special ownership, contextual helper/hook misuse, conflicting
  special ownership and unknown events fail compilation while the accepted
  bank and all its buffers remain byte-for-byte unchanged. An out-of-table
  numeric runtime target fails before motion installation.
- Motion-change retirement and each of the four startup stages stop execution
  before the authored completion write. Both removed/poisoned actors and changed
  generations are exercised. Retirement from the actual custom-fighter landing
  callback stops before the following motion change. Result values remain
  unchanged and VM/payload pools unwind fully.

The current `ftCommon_8007D5D4`, `ftCommon_8007D6A4`, `ftCommon_8007D7FC`, ECB unlock,
ground clamp and `ftAnim_8006EBA4` bodies are extracted from authoritative sources
for this test. The fixture checks jump/velocity/position/ECB side effects of the
plane changes too. Extraction records source hashes and does not call a generated
FMS C compiler. `Fighter_ChangeMotionState` is a controlled boundary that records
motion/flags/frame/rate and can retire the actor. Geometry, immutable source package/view/status
observation and animation stage leaves are also controlled; unrelated binding
families abort if accidentally invoked. This is direct interpreter integration
evidence, not a game launch or complete engine/asset acceptance.

```powershell
python tests/native/script_transition/run.py `
  --compiler C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe `
  --output A:/FreakMelee-test-archives/script-transition-fresh
```

The runner uses hidden subprocesses, writes only to a fresh directory on A:,
and verifies source hashes after execution. CMake can generate the additional
plane header with `extract_planes.py --output <dir>/script_transition_planes_actual.h`,
alongside the existing state/kinetic extracted headers. Run the compiled test
with the absolute `probe.fms` path as its one argument.

Neutral/Wait is explicitly unavailable in this synthetic primitive fixture. It
no longer extracts ft_8008A348; accidental entry aborts. Every existing numeric
transition/plane/startup case remains. Actual neutral/Wait scheduler ordering and
retirement are covered separately by script_fighter_movement_scheduler with
real actors, fixed pool identity and VM storage.

The existing fixed Fighter GObj payload is now admitted by the actual
nativeMatchPool_Bind/Allocate implementation, with a new scene epoch for each
reset. Its classifier and preinitialized native payload are preserved across
allocation. Existing receiver fixtures retain their already allocated real
pool instead of rebinding it. Policy actor callbacks remain explicitly absent;
this pool admission validates the actual shared collision continuation fence,
not a successful synthetic Run or complete actor scheduler.
