# Netplay frame cadence

Run `run.py --compiler <clang.exe> --output <fresh-directory>` inside the
Windows SDK/VC environment. The fixture extracts the exact current match host
state definition and `nativeNetplayMatch_BeginFrame` / `EndFrame` bodies. It
links the actual confirmed-checkpoint, input-history, arena and rollback-audio
journal implementations.

Transport delivery/failure and clock/wait primitives are controlled. Gameplay
is a single integer accumulator in a captured arena; the digest is controlled.
Diagnostics and unrelated trace leaves abort if unexpectedly reached. Normal
input-filter packet reception is simulated explicitly rather than claiming
the full filter is executed.

The cases cover:

- A packet batch queued during reconstruction is deferred at the next
  `BeginFrame`, allowing one normal tick before further reconstruction.
- Normal input sampling can still receive that batch and initiate correction
  after the normal tick. The final state includes propagated predictions.
- A pending normal tick cannot bypass the fifteen-frame prediction window:
  it must receive/reconcile again when the contiguous confirmation frontier
  has not advanced, including an earlier hole before a confirmed later frame.
- Match ending still flushes delayed inputs and waits for confirmation before
  returning, including a predicted ending followed by reconstruction.
- Receive/send failure, timeout and retired arena lifetime do not admit a tick.

This is source-boundary correctness evidence. It does not measure performance,
exercise sockets/GPU/full Melee gameplay, or establish sudden-death acceptance.
The separate hidden local-UDP full-game cohorts provide gameplay/checksum and
production timing evidence.
