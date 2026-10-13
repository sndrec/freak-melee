# Global sound in the native interpreter

`audio.play_global(id, volume, pan)` is shared by authored Fighters and
Articles. Its three arguments must be Int expressions; use an explicit `int()`
conversion when needed. It returns the native Int sound-instance handle, including
`-1` when the native engine cannot start the sound. It does not write a handle into
Fighter or Item sound state. A caller can store the result in a local or state field,
or discard it.

The binding calls `lbAudioAx_800237A8` directly. For `id < 0x83D61`, the unchanged
`fn_80023750` doubles volume and pan, clamps each to 0 through 255, and passes
them to `AXDriver_8038CFF4` with track 0 and channel 7. For example, `(127, 64)`
becomes `(254, 128)`; negative values clamp to zero and 128 clamps to 255.
The interpreter admits ordinary volume and pan inputs from -1073741824 through
1073741823 so signed multiplication by two remains defined. It does not restrict
them to byte or conventional slider ranges.

For `id >= 0x83D61`, native code substitutes ID `0x83D60`, volume 0 and pan 64
before scaling. Therefore the AX call receives `(0x83D60, 0, 128, 0, 7)` and
ignores the original volume and pan; every signed 32-bit value is admitted for
those ignored arguments. Every signed 32-bit sound ID is admitted. The native
driver compares its unsigned bank and stream counts before dereferencing those
tables; negative computed indices fail those checks. Its decoded sound tables
remain an engine-owned prerequisite.

Argument values are sampled once in authored order. The binding captures the
current physical source, owned actor, program and storage identities before the
native call, then checks them before publishing the result. The Article destroyed
hook may call this operation while its owned actor is still live. A failed guard
does not undo a native sound already started.

The inspected helper, AX start and rollback reuse/record paths call no authored
source callback. They use the existing fixed sound-instance pool and bounded
rollback start journal. Source-mutation probes in the silent fixture establish
publication behavior at a controlled boundary; they do not claim arbitrary
callback containment or actual audio playback verification.

The retained full sources currently require this Audio namespace operation in
Sheik's chain contact and Link/Young Link's hook floor contact. Their separate
`sound.play`, `sound.channel`, `sound.loop`, `item.play_sound` and `item.loop_sound`
calls use their existing actor-local contracts.

The focused fixture is `tests/native/script_global_audio`. It mechanically
extracts the unchanged native helpers and replaces only the AX call with a silent
argument/result sink. It also exercises the actual compiler, Prepare/Publish,
Fighter/Article actors, VM and source ownership checks. No audio device is opened;
sound-bank loading, mixer output and gameplay remain outside that proof.
