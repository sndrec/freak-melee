# Generic source jump execution

The fixture compiles real FMS into a complete typed bank, prepares actual Fighter
hook/motion IDs, and executes its jump entry, native property reads/writes,
root rotation and full jump input phase through the VM and production catalog.
The source view/asset/actor-dispatch boundaries are controlled; this is not an
actual actor attachment, DAT import or game acceptance test.

Shared jump begin/basic entry, both native aerial-entry routing sites and full
jump IASA are extracted from current authoritative bodies. Walljump is compiled
from its actual production unit. The owned source primitive is compared against
the real shared jump-begin kernel on a registered reference fighter across reset
timer/worn-sound combinations and exact F32 velocities. Full input ordering is
compared to the shared reference body, including command-0 gating and terminal
interruption.

Coverage includes:

- Prepared `aerial_jump` code-ID dispatch at basic and multi-jump entry sites;
  no second eligible jump with maximum two.
- Absent hook preserves the common basic/multi-jump modes. The raw observed
  Rook frame word/count retain the one-jump horizontal/vertical launch values.
- `jump.begin` preserves flags, velocity, counter, input timer and sound order;
  animation-driven mode and out-of-layout targets reject before mutation.
- The UInt8 engine used-jump counter is exposed as Int, checked on writes, and
  retains the shared kernel's increment wrap. Maximum jumps remains read-only.
- Add-rotation changes the selected Euler root; quaternion roots reject.
- Motion callback fault, physical removal and logical storage retirement stop
  remaining engine stages and authored completion writes.
- Actual walljump decision/entry and its post-entry guard preserve healthy
  timer/counter updates, while removal or deferred logical retirement stop that
  tail and preserve the caller's output.

Motion installation, audio, input predicates, moving-wall geometry, scaled
knockback leaf, asset selection and source view/controller are explicit
boundaries. Unrelated binding families return unsupported. No fighter-specific
native handler is executed. A small fixture host composes the actual script
storage and native binding methods; authored behavior remains bytecode.

```powershell
python tests/native/script_jump/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-jump-review
```

Use a fresh directory on A:. Subprocesses use `CREATE_NO_WINDOW`; the runner
archives sources, commands and results and rejects changed inputs.
