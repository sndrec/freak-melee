# Caller-side frame-end profiling

Set `AURORA_PROFILE_FRAME_END=1` before launching the client to record the
caller-side `end_frame` path. The profiler stores 1,024 records in fixed host
storage and prints them during shutdown, after the FIFO and rendering worker
have drained. It does not allocate or print during measurement and does not
participate in game snapshots. With the option unset, phase clocks are disabled.

The outer fields partition FIFO drain/decoding, texture maintenance, graphics
recording completion, overlay recording, and rendering-worker submission.
Overlay subfields distinguish ImGui freezing and RmlUI setup, context update,
frame preparation, geometry recording and completion. These nested fields are
included in `overlay_us`; they must not be added to the outer totals again.
Texture counters are read only after the FIFO decoder has drained and before
texture frame maintenance resets them. All profile writes stay on the caller.

CPU-only frames use a different entry point and produce no frame-end record.
Record indices count calls to the normal frame-end entry point, including any
warmup. The profiler does not time GPU completion or scanout. Worker scheduling
and caller preemption can affect wall-clock durations; a submission measurement
is not a GPU fence.
