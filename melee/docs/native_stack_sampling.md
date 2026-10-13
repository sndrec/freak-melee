# Windows native call-stack sampling

`tools/native_stack_sampler.cpp` is a separate, single-threaded diagnostic
program. It opens one caller-specified process, verifies its executable path,
finds its oldest live thread and samples that thread's current instruction and
unwound callers. It never sends keyboard/controller input or attaches a debugger.
Every successful suspension is followed by a resume, including partial stack
walks. Sampling ends when the process exits or after forty seconds; startup waits
two seconds before enumerating modules. The process must remain alive then.

Build from a Visual Studio x64 developer prompt, choosing a scratch output path:

```bat
cl /nologo /O2 /EHsc /W4 tools\native_stack_sampler.cpp /Fe:C:\FreakMelee-test-archives\native_stack_sampler.exe /Fo:C:\FreakMelee-test-archives\native_stack_sampler.obj /link dbghelp.lib winmm.lib
```

Run with the exact PID and absolute executable path of a task-owned client:

```bat
C:\FreakMelee-test-archives\native_stack_sampler.exe 12345 C:\gayms\freak-melee\melee\build\native\melee_native.exe C:\FreakMelee-test-archives\owned-client-stacks.jsonl
```

Choose a fresh output filename. The sampler writes newline-delimited JSON:
module identity/ranges and QPC frequency, then QPC timestamps, pause duration,
previous suspend count, context error and the instruction/caller addresses.
The final record reports sample count and any suspension failure/thread status.
DbgHelp is used only by this sampler thread, with a local symbol search path.
The unwind contract follows Microsoft's
[StackWalk64 documentation](https://learn.microsoft.com/windows/win32/api/dbghelp/nf-dbghelp-stackwalk64).
No symbol-server downloads are configured. Offline native symbolization should
use the exact captured executable/PDB, subtracting one from caller return
addresses to identify the calling instruction. Preserve inlined frames.

This sampler perturbs execution. Brief pauses, initial unwind/symbol loading,
timer-resolution requests and scheduling affect both peers. Sample occupancy
and inclusive stack counts are diagnostic, not elapsed CPU costs; inclusive
counts overlap. Do not use these runs for the 10/12-ms performance gate.
Run a separate unprofiled cohort for that measurement. Non-native leaf frames
may need system symbols for finer attribution; module grouping alone does not
identify whether a kernel frame is sleep, I/O, preemption or another wait.

## 2026-10-13 evidence

Kernel ETW profiling first rejected the circular-trace configuration with
invalid flags, then denied the simplified kernel trace access. Neither started
a trace. Those attempts are retained in `stack-profile-oct13-a`; no other
logging session was stopped. The owned-process sampler is an independent path.

`stack-sampling-oct13-live-a` collects 682 / 679 / 690 stacks across stock-zero,
stock-delay and interpreted-delay cohorts. The denser timer-resolution version
in `stack-sampling-oct13-live-b` collects 3,827 / 3,683 / 3,675 stacks. Both
cohorts complete six hundred controller samples, retain peer checksums, reject
zero packets and retain the interpreted endpoint/RNG. Sampling reports validate
source/binary/log pins before subsequent edits, preserve native inlining and
resolve only the captured native executable. Both versions and their binaries
must remain associated with their own receipts.

The denser interpreted sample contains 1,233 stacks in process traversal,
611 in display traversal, 304 in the VM, 79 native leaf samples in callback-source
liveness and 30 in the fighter actor lookup. These overlapping counts locate
work; they are not a millisecond breakdown. Model animation, joint matrices,
collision, material setup, FIFO synchronization and UI also remain visible.
Shutdown logging and explicit pacing appear in the sample and are not part of
the active full-cycle performance population.

Each sampler eventually records `SuspendThread` error 5 near completion while
the queried thread reports `STILL_ACTIVE`. Its exact cause is unconfirmed.
All owned game and sampler processes exit. The denser interpreted sampler's
median pause is 365.7 us, with a 52.38-ms maximum and about 1.50 s of total pause:
another reason not to treat its cycle timings as performance acceptance.
