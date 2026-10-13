# HSD control-state restore fixture

Uses production HSD memory/constraint evaluators and Aurora OSAlloc through the
current native libraries. A clean synthetic joint supplies translation channels
to an actual function expression; its callback also modifies the persistent
argument-buffer tail so payload restoration cannot pass on pose equality alone.

Checks HSD/OS selector independence, active expression/update rejection, null
and allocated buffer snapshots, exact re-save, rewind across first lazy
allocation, coalesced backing, malformed candidate refusal without writes,
buffer overlap, and heap retirement. The separate allocator fixture checks
carved holes, physical extent bounds and allocator mutation.

No window, audio, whole-match restore, borrowed graph closure or rollback
performance acceptance. A range check establishes physical heap provenance;
aggregate validation must also prove allocation/owner consistency against the
candidate OS Cell records before writing payloads.

Run from a configured x64 Visual Studio shell after rebuilding the production
native libraries:

```powershell
python tests/native/native_hsd_control_snapshot/run.py --compiler C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe --output A:/FreakMelee-test-archives/NEW-DIRECTORY
```
