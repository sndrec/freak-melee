# Native audio integration boundary

Status: no native mixer or audible client playback yet. Aurora in this checkout
provides platform/GX support but no implementation of the AX voice API used by
Melee. Declared DSP types in Aurora are not evidence of an audio backend.

## Preserve existing game sound logic

- melee/lb/lbaudio_ax.c: game sound selection and sound-mode UI mapping.
- sysdolphin/baselib/axdriver.c: command streams, priority/channel handling,
  pitch and volume control, and auxiliary effect configuration.
- sysdolphin/baselib/synth.c: bank/stream loading, node/voice lifetime, fades,
  mix levels, source-rate ratios and the driver callback.

HSD_SynthInit calls AXInit, selects the DSP rate, allocates sample/stream ARAM
regions and registers HSD_SynthCallback. HSD_SynthCallback reaps finished voices,
updates sound state, calls the driver master-clock callback and services the
stream buffers. Its cadence must be established from the actual audio driver;
calling it once per game frame would be an unsupported substitution.

HSD_SynthSFXUpdateMix computes main and two auxiliary buses, stereo/mono routing,
and interaural delay. synth.c also reads/writes voice parameter fields directly
(pb.state, pb.itd and sync), so replacing only AX function calls is insufficient.
Native ownership and voice state must be updated at those accesses too.

## Implementation sequence

1. Trace sample bank and stream records through HSD_SynthSFXPlayWithGroup into
   voice address, ADPCM, loop, source-rate and envelope state. Establish native
   sample addresses independently from encoded GameCube addresses; decode file
   records explicitly rather than treating disk bytes as host voice structs.
2. Implement bounded native voice storage, sample decoding, looping, resampling,
   gains and completion/priority behavior. Update HSD call sites and direct voice
   field accesses to that native representation. Keep the original game sound
   command processing; do not implement empty AX functions to satisfy the link.
3. Feed mixed PCM to SDL audio with explicit ownership and timing. Read the
   project's required Multi-Core By Default article before implementing any
   threaded handoff. Determine callback timing from reference evidence first.
4. Integrate the two auxiliary buses/effects and streaming music, then test
   known sounds, looping, pitch, fades, pan, voice stealing and stop behavior.
   Validate actual audible playback through original startup and scene paths.

## Sound mode storage checkpoint

HSD's existing sound-mode variable is now the native authority, defaulting to
stereo. HSD_SynthGetSoundMode returns that value. The original setter still
updates all active nodes' mixes, and driver initialization no longer overwrites
it from console system storage. Removed OSGetSoundMode/OSSetSoundMode calls.
This is in-process state; persistence across application launches is pending.

The original client link now has 87 unresolved symbols. All seven existing
native tests pass, but none validates the unlinked audio runtime or sound-mode
changes. Evidence: build/native/audio-audit-build.log and client-link.log.


## Decoder reference inspection

Aurora does include lib/dolphin/thp/THPAudio.cpp: a THP-frame DSP ADPCM
decoder, with explicit big-endian coefficients/history and mono/stereo output.
It is useful reference code, but it accepts an 80-byte movie-audio header;
HSD SFX banks instead supply AXPBADDR, AXPBADPCM and loop state separately.
Calling THPAudioDecode on a sound-bank record would not implement SFX playback.
The SDK THPDec_8032F8D4/8032FD40/80331340/803313D0 routines are image-header,
workspace and image-decode routines, not the missing AX sound backend.

## Native voice field access

SFX start and music-buffer start now construct local source-rate parameters,
assigning the high and low halves of the 16.16 ratio explicitly. Writing a
32-bit value through ratioHi reversed those halves on Windows. Local zeroed
history/fraction fields retain the original initialization behavior without a
shared scratch object. Removed the music-header scratch ratio assignment that
was always overwritten before AXSetVoiceSrc consumed it.

Music ring-buffer tracking now reads pb.addr.currentAddressHi/Lo by name and
combines them explicitly. The former offset 0x1B2 was 0x138 (original AXVPB.pb)
+ 0x6E (AXPB.addr) + 0x0C (currentAddressHi); it is invalid after host pointers
enlarge AXVPB. Original SDK AXSetVoiceCurrentAddr and AXSetVoiceSrcRatio confirm
the high/low semantics (extern/dolphin/src/dolphin/ax/AXVPB.c).

Default build and twelve existing native tests pass. Those tests do not cover
audible audio or these complete synth paths. Full-client linking still fails
with 46 unresolved symbols. Evidence: audio-fields-build.log and client-link.log.
Sound-bank parsing and its overlapping GameCube metadata layouts still need
native structures; these field fixes do not make the audio backend operational.

## Native SFX banks and loading lifetime

Sound-bank groups, hash entries, and per-voice records now have explicit native
structures in sfxbank.h. The loader decodes big-endian file words instead of
casting disk bytes to AXVPB or overlapping pointer-bearing records. Playback
uses typed voice metadata. Group removal unlinks exact entry identities, so
removing an older group cannot unlink another group's duplicate sound ID.
Compaction updates native address fields and the actual bank allocation cursor;
the former write beyond the bank-list array is removed. Voice range stopping
also combines currentAddressHi/Lo explicitly rather than reading a size_t
across address and ADPCM fields.

Header, metadata, and sample reads now form a sequential callback chain with
one active request. This prevents queued sample cancellation from freeing a
metadata destination still owned by the DVD read. Metadata storage is temporary;
each decoded group is a single allocation made while loading, with two fixed
voice slots per entry. Runtime heap capacity still needs full-client validation.

native_sfxbank extracts metadata into a temporary fixture from every original
SSM on the supplied ISO and runs the production decoder: 110 banks, 2795 mono
and 290 stereo entries. It verifies identities, rates, ADPCM coefficients and
history, downward address relocation, full payload consumption, and rejection
of truncated records or excess voices. Runtime records reside above 4 GiB.
The temporary fixture is deleted after testing; no game data is committed.

native_sfxload exercises the production loader with controlled transfer and
heap providers, canceling at each stage while retaining destinations until
completion. It covers queued loads, duplicate IDs across banks, group unload,
and relocation requests/cursors after removing a group. Audio-device calls
fail if reached; this is not an audio backend substitute. This test does not
exercise Aurora's actual worker or mix samples. The existing Aurora DVD test
covers the separate transport boundary, not complete SFX loading integration.

The default build and fourteen native tests pass. Full-client linking still
fails with 46 unresolved symbols. Evidence: sfx-bank-build.log,
sfx-load-build.log and client-link.log. Streaming music headers, the mixer,
auxiliary effects, audible playback and complete native boot remain unfinished.

## Native music metadata

The music loader now decodes the HPS file header into explicit rate, voice
count, address and ADPCM fields. Chunk headers use native size, end address,
next-file-offset and per-channel loop history fields. A separate aligned
32-byte input buffer is decoded at completion before sample transfers begin;
raw bytes no longer overwrite runtime records. First and subsequent chunks
use the same decoder. SFX header input also has explicit 32-byte alignment.
The stream offset callback context now converts through uintptr_t rather than
casting an integer to a fictitious structure pointer.

The production decoders pass checks against all 98 original HPS files and all
3457 unique reachable chunk headers, following links until EOF or a loop.
Checks cover byte order, channel metadata, ADPCM coefficients/history, invalid
magic, and invalid zero-sized chunks. The fixture exists only in temporary
storage. Decoder acceptance is not streaming playback acceptance: scheduling,
actual sample mixing, loop timing, device output and shutdown are still untested.

The default build and all fifteen native tests pass. The full client still
fails to link with 46 unresolved symbols. Evidence: stream-header-build.log,
Testing/Temporary/LastTest.log and client-link.log under build/native.

## Stateful sample decoding foundation

Added an allocation-free DSP ADPCM decoder with caller-owned sample state,
explicit nibble cursor/end/loop addresses, coefficients and histories. Normal
sample loops restore recorded history; streaming loops preserve running history.
Split reads retain identical output and state. Inclusive end addresses stop
without reading a nonexistent next frame header. Unsupported formats or invalid
header-nibble addresses are rejected; out-of-storage reads stop the voice.

An ISO audit found format 0 (DSP ADPCM) on all 3375 SSM voice records. PCM formats
are not implemented by this decoder. This is a native playback component, not
an emulation of arbitrary DSP register edge cases. References consulted:
https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/DSP/DSPAccelerator.cpp
https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/HW/DSPHLE/UCodes/AXVoice.h
The local SDK AXSetVoiceAddr assertions also exclude frame-header addresses.

Tests cover known signed-nibble values, exponent changes across frames, both
rounding signs, positive/negative saturation, one-sample versus whole-block
reads, EOF at a frame boundary, loop history reset/preservation and invalid
storage/addresses. The default build and sixteen native tests pass, recorded
in audio-sample-build.log and Testing/Temporary/LastTest.log. These are synthetic
sample vectors, not a comparison of decoded original audio against Dolphin.

The decoder is not yet connected to synth voice playback. Resampling, voice
allocation/mixing, effects, audio-device submission and their acceptance remain
unfinished; this addition does not remove the unresolved audio linker symbols.

## Native voice ownership and synth integration

The synth now owns HSD_AudioVoice records instead of SDK AXVPB records. A fixed
64-slot pool implements free-slot reuse, stealing from the lowest eligible
priority, and oldest-first selection within that priority. Reprioritization
moves a voice to the newest position, matching the source SDK queue behavior.
A stolen slot is reserved during its callback. No heap allocation is used.

All synth AX voice operations were replaced at their callers: envelope, mix,
source-rate, address and loop fields are written directly; pool operations and
sample start use native functions. DSP synchronization flags are removed.
Native sample reads publish updated cursor/history and completion state back
to the voice for the original synth's lifetime and stream-buffer bookkeeping.
Mix/source/ITD value structures remain plain data, without the AX runtime API.

Tests cover pool exhaustion, equal-priority refusal, reprioritization age,
lower-priority stealing, reuse, whole-pool release, raw sample playback, EOF,
and live loop-address changes. The production synth start/stop path also runs
against a decoded fixture through the real native pool in native_sfxload.
That test checks pitch initialization and node/voice release. Complete stereo
stealing integration, resampling, gain/delay rendering, release depopping,
auxiliary effects, device output and timing remain unfinished. Raw decoder
output is not yet a complete mixer or audible playback.

The default build and seventeen native tests pass. Full-client linking now
fails with 27 unresolved symbols (down from 46), including audio scheduling,
effects/device services and remaining console calls. Evidence:
audio-voice-build.log and client-link.log in build/native.

## Native polyphase resampling

Native voices now have a four-tap, 128-phase resampling path using persistent
16.16 fractional position and four-sample history. It uses Melee's untouched
coefficient selector zero; source review found no game calls changing that
selector. Decoder reads and filter history span output blocks without allocation.
The implementation currently exposes resampled voice output; a device mixer
still needs to call it and handle final ramps/delay/effects.

The 4096-byte table was copied from the user's installed Dolphin at
A:/programs/Dolphin-x64/Sys/GC/dsp_coef.bin to ../.local/dsp_coef.bin.
SHA-256: dece17b0dd97ca1d1937aba597c70865a579eafb062ecc88e02a92dbe9ee7997.
CMake's MELEE_DSP_COEFFICIENTS selects this local input. The build converts the
first 512 big-endian signed coefficients into an untracked generated include.
Neither source table nor generated game data is committed. Missing input fails
the build; no substitute filter is selected. A portable installation/import
flow for this additional asset remains unfinished.

The reference is Dolphin's AXVoice.h ResampleAudio polyphase branch and
AX.cpp LoadResamplingCoefficients, inspected from its upstream source:
https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/HW/DSPHLE/UCodes/AXVoice.h
https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/HW/DSPHLE/UCodes/AX.cpp
Tests compare output to direct scalar convolution of a known source sequence
for ratios 0, 0.5, 1, 1.25 and 4, whole versus single-sample calls, and a live
pitch change. This is source-derived mathematical validation, not Dolphin
capture parity or audible acceptance. The default build and eighteen native
tests pass (resample-build.log and Testing/Temporary/LastTest.log).

## Native envelope and bus mixing

Added fixed 32-sample (1 ms at 32 kHz) voice mixing into nine signed 32-bit
accumulation channels: main L/R/S, auxiliary A L/R/S, auxiliary B L/R/S.
Resampled input receives the signed GameCube envelope gain, then unsigned bus
gains. Each multiplication saturates to 16 bits before wide accumulation;
envelope and bus ramps update with 16-bit wrap semantics. Per-bus last output
is retained for future release smoothing. No allocation occurs in the mixer.

The original synth's mix updates now configure native bus masks. The masks
follow the local SDK AXSetVoiceMix standard mode: main L/R are enabled, aux
pairs depend on their current gains, and surround routing depends on the
surround gains. Mask selection persists across ramps until the next mix update.
DPL2 mode, interaural delay, release smoothing, aux effects and final device
output are not implemented by this checkpoint. The mixer is not yet driven by
the audio scheduler, and must not be treated as audible client acceptance.

Tests exercise every bus, signed envelope gain, negative/wrapping ramps,
per-voice saturation, disabled auxiliary routing, clearing inactive output,
and wide accumulation across all 64 voices. They use the production decoder
and resampler, with independent scalar gain calculations. The default build
and nineteen native tests pass. Full-client linking still fails with 27
unresolved symbols. Evidence: audio-mix-build.log and client-link.log.
Source reference for envelope signedness and mix arithmetic:
https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/HW/DSPHLE/UCodes/AXVoice.h
The local SDK AXOut.c uses 160-sample frames, consistent with five 1 ms blocks;
callback ordering and native device pacing remain to be integrated and tested.

## Stereo stealing queue lifetime

A production-path stress test exposed overflow in the deferred stereo-partner
release queue: removing a reused voice replaced its entry with NULL without
reducing the queue length. A burst of replacement stereo sounds could append
past its fixed 64 slots before the next synth callback. Removal now compacts
the remaining entries and decrements the count, preserving their order. An
assertion checks the bound before each append.

The new test first failed at the capacity check. It now starts 132 stereo
sounds through the original synth without servicing its audio callback between
starts, verifies the surviving 32 pairs, and checks that callback cleanup does
not release reused voices. A separate full-pool case steals only one member
and verifies that the callback releases its unstolen partner while preserving
the borrowed slot. Default build and nineteen native tests pass; the extended
loader test also passes after adding the single-partner case. Evidence:
stereo-steal-build.log and native_sfxload in CTest. These are lifetime checks,
not audible stereo or playback-timing verification.

## Release-tail smoothing

Stopping or stealing a running native voice now retains its last mixed sample
on each bus in fixed studio accumulators. The mixer applies the SDK AXSPB.c
fade: choose signed sum/160, cap to +/-20, and render value + sample_index*delta
across the next 160 samples. Residual magnitudes below 160 are cleared at the
next frame boundary. New releases during a frame join the next boundary;
reusing a voice slot does not overwrite the retained tail. Accumulation uses
explicit 32-bit wrap arithmetic to avoid C signed-overflow undefined behavior.

The mixer tests cover all nine buses, both fade signs, small residual cutoff,
maximum slopes over multiple frames, stolen-slot reuse without double counting,
and releases between frame boundaries. The default build and nineteen tests
pass; the extended mixer test passes after adding the mid-frame case. Evidence:
release-fade-build.log. This validates arithmetic and ownership, not audible
click suppression. Device scheduling, delay/effects and full native boot remain
unfinished. DSP buffer initialization order was cross-checked against:
https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/HW/DSPHLE/UCodes/AX.h

## Native auxiliary delay

The normal audio startup delay now uses HSD_AudioDelayConfig and a native
effect state instead of embedding runtime pointers in its configuration.
Initialization partitions the caller-owned effect heap; processing allocates
nothing and uses no console interrupt or allocation hooks. The existing
auxiliary buffer callback contract remains until the audio scheduler is ported.
The three AXFX delay entry points are no longer client link dependencies.

The processing algorithm comes from the local SDK delay.c: independent
160-sample channel rings, quantized percentage gains, and 32-bit multiplication
wrap before signed shifts. Configuration rejects zero-length rings, percentages
outside 0..100, and sizes beyond the signed audio heap interface. Insufficient
or unaligned storage leaves the previous state unchanged. The startup heap
check now measures delay parameters as delay, instead of reading them as reverb
parameters. Its default 260/310/6 ms configuration needs 72,320 bytes and fits
the original 71 KiB allocation.

The native delay test checks independent channel echoes and feedback decay,
actual startup echo positions, exact-capacity storage and guard bytes, rejected
configuration, signed rounding and overflow. The default build and all twenty
native tests pass. A fresh full-client link fails with 24 unresolved symbols,
down from 27. Evidence: delay-build.log, native_audio_delay in CTest, and
client-link.log. This does not establish audible playback: auxiliary scheduling,
reverb/chorus, native device output and the full-client boot remain unfinished.

## Native standard reverb

The standard reverb configured during normal audio startup now uses native
configuration/state and caller-owned float storage. AXDriver and lbAudioAx
call it directly; the three AXFX standard-reverb entry points are no longer
client dependencies. Initialization needs 53,304 bytes for the startup settings,
within the existing aligned 53 KiB heap. Processing allocates nothing.

The algorithm was translated from the local SDK reverb_std.c assembly: two
feedback combs, two allpass filters, a lowpass accumulator, and wet/dry gains
scaled by 0.6. Explicit fmaf operations preserve its fused arithmetic, with
automatic contraction disabled. Native indices replace byte offsets and the
callback uses all three supplied buffer pointers instead of assuming they are
contiguous. Output conversion truncates and saturates explicitly.

The assembly's predelay wraps at allocated_samples - 1, so the normal 64-sample
allocation retains its 63-sample effective delay. A one-sample setting is
bounded to one sample instead of reproducing the original out-of-bounds path.
Invalid/non-finite settings or insufficient/unaligned storage are rejected
without replacing initialized state. No standalone SDK settings/shutdown
facade was added; effect transitions reset state and reuse the owned heap.

Tests cover silence, exact heap capacity/guards, separate channel buffers,
first comb echoes, feedback recurrence, positive/negative symmetry, predelay
including the one-sample edge, immediate and delayed colored allpass response,
and the original zero-wet 0.6 dry gain. The default build and all 21 native
tests pass. A fresh full-client link reports 21 unresolved symbols, down from
24. Evidence: reverb-build.log, reverb-test-build.log, native_audio_reverb in
CTest and client-link.log. This is arithmetic and storage evidence, not
audible playback or full native boot acceptance. High reverb, chorus, audio
scheduling/device output and other platform dependencies remain unfinished.

## Native auxiliary bus ownership and scope

A complete game-source call-site search found only the two startup effect
configurations: standard reverb on A, delay on B. Nothing selects high reverb
or chorus. Their unused AXDriver branches, runtime arrays, enum entries and
generic allocation hooks were removed instead of porting unused SDK features.
The original SDK source remains reference material outside the native build.
Earlier notes listing those two effects as required porting work are superseded.

HSD_AudioAuxConfigure now owns two fixed native effect states, with typed
configuration unions and caller-owned storage. Startup supplies the original
parameters directly and checks initialization success. Reset/disable discards
old state; failed configuration preserves the active effect and its tail.
HSD_AudioAuxProcess dispatches the native filters with typed 32-bit channel
pointers, clearing returns for disabled buses. The AXFX buffer structure and
AX callback registration are no longer used by game/HSD source. No compatibility
callback or allocator facade remains.

The bus test exercises reverb and delay on separate buses, echo preservation
after rejected changes, bus-local disable, reconfiguration into reused storage
and reset. Existing filter tests also run through the typed buffer interface.
Default build and all 22 native tests pass; full-client linking now reports 12
unresolved platform/audio symbols, down from 21. Evidence: aux-build.log,
native_audio_aux in CTest, and client-link.log. Configuration is wired into
game startup; processing is currently exercised by tests and awaits connection
to the 160-sample frame scheduler and native device output. Audible playback
and the complete versus boot are still unverified.

## Native PCM output and first full client link (2026-09-12)

The native mixer now renders 160-sample stereo frames, rotates the auxiliary
DSP/CPU effect buffers, and submits native-endian LR PCM to an SDL3 audio stream
at 32 kHz. The game owner polls device demand and advances the existing synth
callback after each frame. Work is capped at four frames per poll, with a 20 ms
input queue target; this is not a bound on total device latency. SDL owns its
internal queue allocations. Device latency, underruns, audible fidelity, ITD,
DPL2, and rollback audio reconciliation remain unverified or unfinished.

The default build and explicit melee_native target both pass. All 23 native
CTest cases pass, including the new frame test for LR ordering, clipping,
surround exclusion, and delay returns through the triple buffer. This isolated
fixture supplies mixer inputs; it does not prove live playback or exact DSP
pipeline timing. The full client links for the first time. Logs:
build/native/client-link.log and build/native/audio-frame-build.log.

## Native sound-command metadata (2026-09-12)

SEM loading now validates the five counted tables and file-relative stream
ranges, converts big-endian words in place, and retains native pointers to the
SFX bank/offset tables. Command selection resolves a byte offset into the native
word buffer; no host pointers are truncated into file entries. Removed unused
relocated table globals. The existing command interpreter consumes converted
words without a replacement interpreter or per-frame allocation.

All 24 native tests pass. New parser coverage checks truncated tables/payload,
invalid counts/indices/offsets, unchanged input on truncation, bank selection,
and word endianness. The full client successfully parses its original SEM and
enters gm_801A4510. It next stops in HSD_MemAlloc during first-scene text storage
preparation, not in audio parsing. Live command playback, full command-stream
semantic validation and audible fidelity remain unverified. Logs:
build/native/sem-native-build.log, sem-tests-build.log, sem-native-boot.log.

## Sustained event-sound lifetime (2026-09-20)

The public grouped-play function returned a generated synth handle but was
declared as `bool`. Native C therefore reduced every successful handle to one
before the AX driver stored it. Synth completion callbacks carried the original
handle, could not find their driver owner, and left the 96-entry sound-command
pool permanently asleep. Once that pool filled, gameplay stopped issuing hit,
movement, and voice sounds while the separately managed music stream continued.

`HSD_SynthSFXPlayWithGroup` now returns the complete integer handle. A focused
test requires grouped playback to return a handle that resolves to its exact
synth node. In a live four-fighter Final Destination match, the broken build had
all 96 driver entries stranded with handle 1 and 62 of 64 low-level voices free.
After the correction, a later snapshot had eight active driver entries and 52
free low-level voices, and a new event after the former cutoff returned handle
`0xDBA1`. This establishes sustained command allocation and completion beyond
the failure window; final audible fidelity still requires listening acceptance.

## Rollback voice interruptions (2026-11-15)

Character voice routines stop their current audio track before starting a new
utterance. Sound playback continues in real time while gameplay is rolled back.
The start journal reused an utterance already playing from a predicted frame,
but replayed track stops could cut it off before that frame was reached again.
The reused handle then referred to a stopped voice.

Each of the 96 AX command slots now records the exact generated handle and its
start frame outside the gameplay snapshot. Replayed single, track-wide and
global stops preserve voices whose starts have not yet been reached on the
corrected timeline. Stops after a reached start still work, and reconciliation
still cancels sounds absent from the corrected frame. Forward-play stops remain
immediate. The stop check takes constant time and uses fixed storage.

Start identity uses the sample, track, channel and occurrence order. A corrected
volume or pan updates the existing voice. Match quality logs include a
`protected-stops` counter alongside reused, new and canceled starts.

The production-driver regression failed before this fix on an earlier replayed
track stop. It now covers earlier and same-frame stops, genuine interruptions,
canceled predictions, repeated corrections, changed spatial mix, newly corrected
starts and pool-slot generations. The fixture compiles the actual AX start/stop
functions with a counting synth boundary. All ten native audio tests pass.

Hidden, muted two-peer runs with six frames of artificial outbound delay also
completed more than 3,700 replay ticks per peer and more than 900 confirmed
checksum comparisons without divergence. Both stock and scripted fighters
produced non-silent PCM captures. These runs exercised start reconciliation but
did not encounter the protected-stop branch; that branch is verified by the
driver regression. Remote audible acceptance remains a separate check.
