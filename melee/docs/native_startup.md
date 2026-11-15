# Original client startup integration

> Historical native startup/porting evidence. For new work, use the [current build instructions](native_build.md).

The next acceptance milestone is the original game entry point reaching its
scene scheduler on Windows. The model viewer remains a diagnostic and will not
be grown into an alternate game client.

Build the actual entry point with:

```powershell
cmake --build build/native --target melee_native -j 8
```

The target links src/melee/gm/gmmain.c and the existing melee_runtime archive.
It is excluded from the default build while unresolved native dependencies
remain. It currently DOES NOT LINK and has not been launched. The original main now initializes Aurora through aurora_main. The original scene loop now polls Aurora events and submits frames, and main
shuts Aurora down after the mode loop returns. These paths remain runtime
unverified; link completion alone will not prove startup acceptance.

## Original execution path

1. gmmain.c main initializes OS/VI/DVD/PAD/CARD, selects debug settings, allocates
   framebuffer/FIFO storage, and calls HSD_InitComponent.
2. initialize.c sets up HSD heaps, video, GX state, IDs and object allocation.
3. main initializes audio, pad/retrace callbacks, asset heaps, DVD/ARAM loading,
   save/snapshot/movie/text services, and shared game data.
4. gm_801A4510 in gm_1A3F.c initializes game modes, selects the boot mode, and
   repeatedly calls runGameMode. Preserve this game-mode/scene orchestration.

## Current link evidence

The direct link first stopped on duplicate expf/powf symbols. The original game
approximations now use lbExpf/lbPowf, including bytecode, sound pitch and wall-tech
callers; Windows/Aurora retain their own standard math symbols. Original game
bodies are unchanged. Tests include negative exponential and the game's 0^0=0.

Direct native C matrix/vector calls replace the remaining linked PS/MTX SDK
spellings. After these changes the unresolved-symbol list fell from 117 to 97.
These counts describe this link, not total remaining porting work. Full client
link log: build/native/client-link.log. Existing seven native tests pass and
the model diagnostics relink; this is not game runtime or visual parity proof.

## Work order

1. Adapt main/HSD video ownership to Aurora initialization, frame presentation,
   event handling and exit, keeping the existing game/scene callbacks. Replace
   console reset/progressive-scan/development-device paths at their callers;
   do not fill the link with empty SDK substitutes.
2. Establish the native arena and original game heap lifecycle, then original
   disc/archive loading with host pointers and decoded records. Existing DAT
   import routines are building blocks, not a complete replacement startup.
3. Integrate audio scheduling/voices and movie startup dependencies, preserving
   the requested versus audio and scene behavior. Resolve GX and remaining
   runtime operations by inspecting their actual call sites.
4. Run original boot/scene initialization with bounded diagnostics; follow
   real failures through original fighter creation and scheduled callbacks.
   Completion gate is a running original scene, then a playable versus match.

## Aurora entry checkpoint

Original gmmain.c now uses Aurora's main entry and initializes the window/runtime
before the original OS/HSD/game initialization sequence. Startup no longer waits
for a GameCube USB development server; the scene loop no longer polls that
server. Removed unused PPC register/EABI setup and the development-console
48 MB arena carveout. Other development services remain to be audited.

The training speed conversion now uses an ordinary u64 cast for the existing
positive tick calculation; the menu sqrt routine uses standard NAN/INFINITY
instead of external compiler-library data. No gameplay math was substituted.

All game sources compile, the entry point resolves, and seven native tests
pass. There are currently 95 unresolved symbols, including development services
still referenced elsewhere. No client executable has been launched. Evidence:
build/native/client-link.log and build/native/startup-entry-build.log.

## Original scene loop submission and exit

The original gm_801A4D34 loop polls Aurora events while waiting for the original
pad queue. Window close sets gmMain_ExitRequested, unwinding the scene and mode
loops, retaining their exit callbacks, and returning to main for Aurora shutdown.
The existing input evaluation, scene on_frame, pre-GObj callback, GObj scheduling
and draw callbacks remain in place. The render section uses aurora_begin_frame
and aurora_end_frame; it retains HSD clear/filter/gamma/update settings, and
finishes the scene with GXDrawDone instead of waiting for a console XFB queue.

This code compiles but has NOT executed: the client still has 95 unresolved
symbols. The existing seven tests pass but do not cover this scene loop. Original
alarm-driven input pacing and VI initialization still need native adaptation;
window close during startup/loading, screenshot capture, display changes and
actual cleanup/presentation require runtime validation. No frame pacing or boot
acceptance is claimed. Evidence: build/native/scene-loop-build.log and client-link.log.

## Native HSD video initialization

HSD_InitComponent now initializes HSD video settings without console retrace
callbacks or a startup retrace wait. HSD_VIInit directly initializes the native
state, configures Aurora VI dimensions, and sets the clear color/depth. main
uses GXInit(NULL, 0). Removed unused console XFB/FIFO allocation functions,
options and storage, and the former retrace/draw-done startup callbacks.

The model-runtime test executes this initializer, verifying render settings
and empty console queues/callbacks. Seven native tests pass. This does not
validate actual client presentation. Legacy movie/error-screen/screenshot XFB
consumers remain to be adapted; native black-screen/display-change semantics
also need integration. Their remaining functions are not evidence that native
screenshots or movies work. The client still does not link.

## Native startup diagnostics and launch input

Remaining executable OSPanic call sites now directly call HSD_Panic, preserving
fatal termination and adding the existing native panic observer. Console stack
sentinel scanning is replaced with Windows GetCurrentThreadStackLimits and
GetCurrentThreadId; addresses use pointer-width formatting. Removed the console
scheduler integrity check from the scene loop.

Launch-button detection now pumps Aurora events and takes a synchronous PADRead
snapshot, selecting the first valid controller as before. No-controller input
is zero. Removed retrace waits and the discarded hardware card-probe wait;
Aurora PADRead synchronously reports connected/disconnected devices and card
probing is file based. An early window-close event returns to main for shutdown.

All seven native tests pass, including actual fatal termination and callback
verification. These tests do not execute launch input or Windows stack reporting.
Full client startup still cannot link: 89 unresolved symbols remain. Evidence:
build/native/startup-debug-build.log and build/native/client-link.log.

## Audio dependency audit

This Aurora checkout has no AX voice mixer. docs/native_audio.md records the
original callback, bank, stream and direct voice-state dependencies and the
implementation sequence. HSD now owns its existing sound-mode value, with a
stereo default, retaining active-node mix updates. Cross-launch persistence and
all audible playback remain pending. The client has 87 unresolved symbols.

## Unresolved symbols at this checkpoint

- AIInit
- AISetDSPSampleRate
- AISetStreamVolLeft
- AISetStreamVolRight
- AXAcquireVoice
- AXFXChorusCallback
- AXFXChorusInit
- AXFXChorusShutdown
- AXFXDelayCallback
- AXFXDelayInit
- AXFXDelayShutdown
- AXFXReverbHiCallback
- AXFXReverbHiInit
- AXFXReverbHiShutdown
- AXFXReverbStdCallback
- AXFXReverbStdInit
- AXFXReverbStdShutdown
- AXFXSetHooks
- AXFreeVoice
- AXInit
- AXRegisterAuxACallback
- AXRegisterAuxBCallback
- AXRegisterCallback
- AXSetVoiceAddr
- AXSetVoiceAdpcm
- AXSetVoiceAdpcmLoop
- AXSetVoiceCurrentAddr
- AXSetVoiceEndAddr
- AXSetVoiceItdOn
- AXSetVoiceItdTarget
- AXSetVoiceLoop
- AXSetVoiceLoopAddr
- AXSetVoiceMix
- AXSetVoicePriority
- AXSetVoiceSrc
- AXSetVoiceSrcRatio
- AXSetVoiceState
- AXSetVoiceVe
- AXSetVoiceVeDelta
- DCFlushRange
- DCInvalidateRange
- DCStoreRange
- FIOExit
- FIOFclose
- FIOFopen
- FIOFwrite
- FIOInit
- FIOQuery
- GXNtsc480Prog
- GXSetCopyClamp
- GXSetTevClampMode
- GXWaitDrawDone
- MCCClose
- MCCEnumDevices
- MCCExit
- MCCGetConnectionStatus
- MCCGetFreeBlocks
- MCCGetLastError
- MCCInit
- MCCNotify
- MCCOpen
- MCCRead
- MCCStreamOpen
- MCCWrite
- OSCancelAlarm
- OSCreateAlarm
- OSDisableInterrupts
- OSGetConsoleSimulatedMemSize
- OSGetProgressiveMode
- OSGetResetCode
- OSGetResetSwitchState
- OSInitAlarm
- OSResetSystem
- OSRestoreInterrupts
- OSSetAlarm
- OSSetPeriodicAlarm
- OSSetProgressiveMode
- PADSetSamplingRate
- THPDec_8032F8D4
- THPDec_8032FD40
- THPDec_80331340
- THPDec_803313D0
- VIGetDTVStatus
- VISetBlack
- VISetPostRetraceCallback
- VISetPreRetraceCallback
- VIWaitForRetrace


## Still-image decoder call contract

The original lbMthp8001FAA0 image loader now calls Aurora THPVideoDecode with
encoded image bytes and its three Y/U/V destination planes. The former call
used Melee's SDK-specific five-argument contract (header, output word, scratch,
encoded bytes, context), which is incompatible with Aurora despite the shared
symbol name. Removed the obsolete decoder context/scratch allocations,
second-stage decode calls, and CPU cache invalidation from this caller.
Decode failure now reports an HSD assertion. THP declarations match Aurora's
const input and BOOL initialization result.

The movie caller in lbmthp.c still requires conversion, including its truncated
frame pointers and encoded file fields; its old THPVideoDecode arguments are
not native-ready. This checkpoint does not claim movie or image runtime
acceptance. All game sources compile and seven existing native tests pass;
those tests do not execute this image loader. Actual client linking still
fails, now with 86 unresolved symbols (client-link.log). The removed image-only
THPDec_8032F8D4 dependency accounts for the reduction from 87.


## Movie decoder and file storage conversion

lbmthp.c now passes the encoded frame and Y/U/V planes directly to Aurora's
THPVideoDecode. Removed SDK decoder context and scratch allocation and the
second-stage decode functions. Frame buffer entries are native byte pointers;
allocation and placement use sizeof(pointer), and device requests retain the
full address. Header fields and frame size words are decoded explicitly from
big-endian bytes. Encoded frame storage uses the same aligned stride in its
allocation and placement. Obsolete CPU cache operations on decoded planes
were removed. Decode errors fail through HSD diagnostics.

ISO inspection checked every .mth header and the first 32 frame boundaries,
capacity limits and JPEG markers; evidence: build/native/movie-file-audit.json.
This validates file assumptions, not decoded pixels or playback. The original
alarm-driven progression, stream requests and remaining platform calls still
need integration and runtime acceptance. Seven existing native tests pass;
they do not exercise movie playback. The actual client still fails to link on
83 unresolved symbols, with the remaining three THPDec helper dependencies
removed. Evidence: movie-build.log and client-link.log.


## Obsolete TEV clamp calls and screenshot dependency trace

Removed four GXSetTevClampMode calls in ftafterimage.c, lbcollision.c,
lbgx.c and lb_0146.c. The supplied SDK GXTev.c implementation only asserts
that the operation is unavailable on this hardware; it does not write GX
state. All four callers retain their real TEV configuration. Three use
GX_PASSCLR, whose Aurora implementation explicitly sets both color and alpha
operations with clamping enabled; lbcollision sets those operations directly.
Seven native tests pass, and all game sources compile. No visual runtime
comparison was performed for these render paths. Client linking still fails
with 82 unresolved symbols (tev-build.log, client-link.log).

The MCC/FIO dependency chain is retained by db_TakeScreenshotIfPending through
hsd_80393A5C. That caller still reads a console XFB and sends it through the
GameCube USB development interface. Aurora exposes no public screenshot API
in this checkout. Native screenshot work must obtain the actual framebuffer
and write it locally before removing the legacy transfer subsystem; merely
replacing FIO writes would leave the invalid XFB capture in place.


## Native boot destination

Startup now enters GM_BOOT without checking the GameCube digital-video cable
or console progressive setting. Removed the progressive-scan scene, mode,
registrations and debug-menu item. Remaining scene/mode numeric IDs are kept
explicit so existing numeric consumers are not silently renumbered.

Per the user's clarified preference, boot requests GM_MENU and menu entry from
GM_BOOT selects MENU_KIND_VS with SEL_VS_MELEE highlighted. This is the Versus
Mode menu before character selection. The existing save-data boot step and
mode initialization remain; opening movie and title presentation are skipped.
Native save/profile replacement remains pending, as does actual menu display
acceptance. The native client still does not link: 79 unresolved symbols.
All game sources compile and seven existing tests pass, but those tests do
not execute this startup route. Evidence: native-boot-route-build.log and
client-link.log.


## Native video reconfiguration

HSD_VISetConfigure now resides in video_state.c and directly calls Aurora's
VIConfigure after copying the mode into HSD storage. It no longer marks a
change for the removed retrace callback to apply. Existing 640x480 and deflicker
presets remain; removed the unreachable progressive branch, preset and mode
flag after removal of the console cable prompt. Aurora controls native display
presentation; the retained GX preset names do not make Windows interlaced.

Extended the native model-runtime video check to verify mode replacement,
preservation of clear settings, copying rather than retaining caller-owned
mode storage, and no pending retrace flag from this configuration operation.
All seven native tests pass. The check does not create a window or establish
visual fidelity, and the remaining legacy black/XFB paths still need native
integration. Actual client linking remains incomplete with 78 unresolved
symbols (video-config-build.log and client-link.log).


## Native screenshot integration

Replaced the debug XFB/USB export with Aurora GPU readback and local BMP saving.
Removed unused hsd_392C/hsd_3933 development-device sources and headers.
The diagnostic D3D12 run saved an inspected 960x720 rendered image; seven
existing tests pass. Full-client linking now has 60 unresolved symbols.
See native_screenshots.md for ownership, supported scope and runtime limits.


## Native screen blanking

HSD_VIInit and HSD_VISetBlack now set Aurora presentation blanking directly.
The latter moved to video_state.c and no longer leaves a pending retrace flag.
Remaining direct VISetBlack callers now use HSD_VISetBlack. Aurora copies the
setting into each submitted frame; the render worker clears the presentation
surface to opaque black instead of drawing the GX image or UI overlays.
Game rendering and simulation continue, and disabling blanking restores normal
presentation. Initialization resets the Aurora setting for a new session.

Seven existing native tests pass, including HSD blank/unblank state checks.
The viewer's optional --test-blanking argument blanks at frame 60 and restores
at frame 180. A D3D12 run completed 300 frames; visible-window captures at
frames 30, 120 and 240 were inspected and show model, black, model. The desktop
taskbar overlaps the top edge of these screen captures; the unobscured game
area demonstrates the transition. Evidence: video-blank-runtime.log,
blanking-30.png, blanking-120.png and blanking-240.png under build/native.
The GX screenshot API intentionally remains a capture of the pre-presentation
image, not a test of blanking. Other backends and full-game transitions remain
unverified. Actual client linking still fails with 59 unresolved symbols.


## Console reset removal

Removed physical-reset-button sampling/latches from HSD controller input and
the console exception display. Removed the hardware reset branches from disc
error servicing and the original scene/mode loop, including console reboot,
retrace waits and callback manipulation used solely for that reboot path.
Normal scene exit, state exit, mode unload and temporary-mode restoration now
run without the obsolete reset-state condition. The controller-reset routine
whose only caller was this hardware path is removed.

Removed reset-only game flags and OSGetResetCode startup detection. Cold-start
power-count behavior remains; native restarts no longer depend on a console
reset reason. This does not add a new restart shortcut or alter the separate
match/menu controller actions. Existing window-close event handling remains.

All game sources compile and seven native tests pass. These checks do not
exercise full scene transitions, disc errors or controller behavior in the
unlinked client. Actual client linking remains incomplete with 54 unresolved
symbols, five fewer than the preceding checkpoint. Evidence:
native-reset-build.log and client-link.log in build/native.


## Native heap diagnostics

Removed the old leak.c/leak.h scanner and its debug-input caller. Its sole
state table was initialized to NULL with no registration/initialization path
in this source tree, and its scan relied on fixed GameCube address regions.
This is not replaced by a claim of native leak detection. Native lifetime/leak
instrumentation remains future work. The existing heap, class, object, entity
and thread reports remain available from the debug diagnostic action.

Updated lbHeap_80015DF8 to use native-width formatting for counts and report
negative or out-of-range free-space results as invalid accounting instead of
performing misleading used/free arithmetic. Heap size storage itself remains
u32; this change does not claim expanded heap capacity. All game sources
compile and seven existing native tests pass. Those tests do not invoke this
debug report. Actual client linking remains incomplete with 53 unresolved
symbols. Evidence: heap-report-build.log and client-link.log in build/native.

## Native disc-error rendering

The original disc-error loop now polls Aurora window events and submits its
text through native frames. Standalone text drawing takes a typed HSD_Text
pointer instead of packing that pointer into the GObj callback's integer pass.
Its orthographic projection now uses Mtx44 storage, matching C_MTXOrtho's
16-float output. The ordinary GObj text callback retains its pass-2 behavior.

Removed the old asynchronous XFB copy machinery after migrating its last
caller. Remaining movie/console XFB queries and waits still need review.
All game sources compile and seven existing native tests pass; these tests
do not exercise the error screen or full client. Actual client linking still
fails with 51 unresolved symbols. Evidence: error-screen-build.log and
client-link.log in build/native. The intended normal startup destination is
the Versus Mode menu with Melee highlighted, before character select.

## Movie shutdown texture lifetime

Movie shutdown now calls GXDrawDone before freeing decoded movie buffers.
The former HSD_VIWaitXFBFlush returned immediately with native nb_xfb == 0;
it provided no lifetime protection. Removed that unused yielding XFB wait.

In this Aurora checkout, GXDrawDone drains the FIFO processor. Draw processing
resolves sampled textures; new_static_texture_2d converts/uploads their bytes,
and queue_texture_upload_data copies them into owned frame upload storage
(including the overflow path). Thus the drain protects CPU source lifetime;
this is not a claim that GXDrawDone waits for physical GPU completion.
Console exception-display XFB queries remain separate unfinished work.

All game sources compile and seven existing tests pass. They do not exercise
movie shutdown, asynchronous movie reads or playback. Actual client linking
still fails with 50 unresolved symbols, chiefly audio and scheduling services.
Evidence: movie-shutdown-build.log and client-link.log in build/native.

## Native input cadence

The scene loop now polls controller sampling on the game thread using
SDL_GetTicksNS. lb_0195 no longer creates a periodic console alarm or changes
PAD hardware sampling registers. Speed-setting callers pass game speed
directly, eliminating their console bus-clock conversion. The scheduler uses
integer nanoseconds and retains separate game-speed and base-60-Hz phases.
Card-presence probes run alongside each real input sample. Idle scene polling
yields for 1 ms while continuing to process window events.

Accelerated gameplay renders on base-channel updates so vsync does not impose
one display wait per accelerated simulation step. Debug-paused scenes still
render. Timing state remains fixed storage; no worker or per-frame allocation
is introduced. All discovered pad queue producers are game-thread calls, so
controller queue interrupt guards are removed rather than replaced with an
interrupt emulation layer.

Small scheduling jitter retains the deadline phase. A missed full period
produces one fresh sample and resumes the cadence without manufacturing input
history. Long stalls therefore slow local simulation instead of catching up
with invented past input. Online rollback scheduling and recorded-input
ownership remain unfinished, as do controller bindings and calibration.

Eight tests pass, including a new test compiling the production input clock
with hardware/card boundaries substituted. It checks 600 normal-speed steps,
half/double speed channel phases, service counts, jitter, long-stall recovery,
speed changes and stopping. It does not test real controller polling, visible
game-speed behavior, debug pause or complete scene execution. All game sources
compile; full-client linking still fails with 49 unresolved symbols. Movie
alarms, audio and other console services remain. Evidence:
input-timing-build.log and client-link.log in build/native.

## Native handle allocator startup

lbMemory_8001564C now initializes heap free-list links through the actual
Handle array. The previous fixed offsets (0x638 through 0x688) wrote inside
unrelated fields after native pointer widening. Allocation nodes now use
Handle storage directly instead of casting a shorter MemEntry pool.

Allocation, freeing and free-space reporting use uintptr_t/size_t. Best-fit
placement and 32-byte size rounding remain, including choosing the later gap
on equal fit. List insertion/removal uses pointer-to-link traversal instead
of fabricating a Handle over the heap's child pointer. Heap-range checks no
longer truncate native pointers when distinguishing ARAM offsets.

The separate, still-unported asynchronous compaction implementation and its
private state now live in lbmemory_compact.c. Its callback pointer packing,
console alarms and address-space-specific copies remain unresolved work.
No claim is made that allocation sizes above 4 GB are supported throughout
the full game or by compaction merely because the handle allocator can track
them.

Nine tests pass. The new native_lbmemory test runs production initialization,
all five externally available heap descriptors, all 131 allocation nodes,
free/reuse, best-fit placement, and accounting with synthetic addresses and
sizes above 4 GB. Only ARAM range discovery and reporting/assertion boundaries
are substituted. These tests do not execute full startup or compaction.
All game sources compile; full-client linking still fails with 49 unresolved
symbols. Evidence: handle-allocator-build.log and client-link.log in build/native.

## Immediate native heap compaction

Superseding the compaction limitation above, lbMemory_Compact now walks the
allocated blocks and performs overlap-safe memmove operations before updating
each handle address. It uses native-width addresses and sizes. ARAM heaps use
Aurora's existing ARGetStorageAddress backing store; native RAM heaps use their
host addresses directly. Heap bounds are checked before each move.

Removed the copy alarm, chunk-delay state, recursive completion chain and
pointer-to-integer callback arguments. lbHeap_Compact and preload cleanup now
use immediate completion directly. Cache loading proceeds after compaction
returns, without re-entering the preload scheduler from a completion callback.
This introduces no worker or per-copy allocation. Large compactions can stall
their caller; preload/transition performance and outstanding DVD request
ownership still need runtime validation.

Nine tests pass. The allocator test now additionally copies actual patterned
bytes in native RAM above 4 GB and a substituted ARAM backing store. It verifies
overlapping moves into a smaller gap, subsequent block contents, relocated
handles, unchanged free-space totals and repeated compaction. This verifies
the copy algorithm, not the complete preload pipeline. All game sources compile;
full-client linking still fails with 48 unresolved symbols. Evidence:
compaction-build.log and client-link.log in build/native.

## Native ARAM reads and transfer declarations

The fighter animation loader's two synchronous ARAM reads now call lbArq_Read,
which validates the requested range and copies directly from Aurora backing
storage. Removed the unused callback/pending-list machinery and its startup
initializer. Reads copy the requested archive bytes without DMA alignment
padding. Other fighter animation pointer layouts/import steps remain unfinished.

Melee's ARQPostRequest declaration now preserves uintptr_t source/destination
arguments, matching Aurora. Aurora's callback declaration and invocation now
use ARQRequest* directly, matching the actual Melee callbacks. No callback
adapter is inserted. General devcom callback argument packing and concurrent
DVD completion ownership remain separate work.

Ten tests pass. native_lbmemory verifies an unaligned-length direct ARAM read
into a native buffer without overwriting the following byte. A new
native_aram_transfer test uses Melee's declarations and the actual Aurora AR
provider, verifying a write/read round trip between host buffers above 4 GB
and ARAM, plus callback request identity. It initializes memory-only ARAM
storage without a window. These checks do not establish fighter animation
playback. Full-client linking remains incomplete with 48 unresolved symbols.
Evidence: aram-read-build.log, aram-transfer-build.log and client-link.log.

## Startup memory configuration and clock domain

The client now requests a 128 MiB Aurora main-memory arena and 16 MiB ARAM
backing store. Both were previously left zero in AuroraConfig, disabling the
regions required by HSD_OSInit and the existing audio/asset allocation paths.
The larger main arena provides room for widened native structures; this does
not remove the remaining fixed asset-heap partitions or establish their final
capacities. ARAM remains the existing offset-addressed asset store for now.

Native OS clock macros now use Aurora's declared 162 MHz conversion domain
(40.5 million timer ticks per second), rather than dereferencing GameCube
addresses 0x800000F8/FC. This is a time-unit convention, not a claim about host
CPU frequency. Aurora ARAlloc's debug assertions now reference its actual
native allocation state instead of undeclared SDK globals.

The default build and all ten tests pass. native_aram_transfer now calls the
actual Aurora OSInit, verifies a native arena above 4 GB with the requested
capacity, checks timer conversion, and exercises ARAM allocation/free stack
behavior before its transfer checks. This remains a memory-only test, not
HSD initialization or full game boot. Full-client linking still fails with
48 unresolved symbols. Evidence: startup-memory-build.log and client-link.log.

## Device completion context types

HSD_DevComCallback now takes a void* context, and all seven dispatch sites
pass the stored context without 32-bit casts. File, movie, stadium and synth
callbacks use the matching signature. Synth callbacks previously invoked
through incompatible function-pointer casts now declare all four parameters
directly; their call sites no longer cast away signature mismatches.

Preload completion receives its PreloadEntry pointer directly instead of an
index packed into a pointer and unpacked through int. The stream-header
callback retains its numeric file-offset context using uintptr_t explicitly.
This does not change callback thread ownership, add synchronization or provide
an audio backend. Existing DVD worker access and other native asset layout
issues still require porting.

The default build and ten existing tests pass. They do not execute devcom
dispatch, so validation here establishes complete source signature agreement,
not runtime transfer/cancellation coverage. Full-client linking still fails
with 48 unresolved symbols. Evidence: callback-build.log and client-link.log.

## Synchronous transfer completion ordering

ARAM queue submissions now set busy state and advance chunk bookkeeping before
calling ARQPostRequest. Aurora invokes its completion callback synchronously;
the previous ordering could restart an unchanged request recursively or leave
the queue busy after completion. DVD reads likewise mark their queue busy
before submission. DVD-to-ARAM chunk bookkeeping and final-request unlinking
now precede ARAM submission, and final completion keeps a local request pointer
instead of rereading a relay slot that a nested callback can reuse.

Eleven tests pass. New native_devcom tests compile the production queue and
substitute deterministic synchronous DVD/ARAM boundaries. They cover chunked
zero fill, RAM-to-ARAM, ARAM-to-RAM, scratch-buffer delivery, chunked ARAM copy,
DVD-to-ARAM, DVD-to-RAM, DVD scratch delivery and a completion that submits
another transfer. Payload bytes, callback context/count and idle queue state
are checked. These tests model Aurora's already-verified synchronous ARAM
contract but do not exercise actual concurrent DVD workers or cancellation.
The unresolved interrupt guards and queue thread ownership still require
native implementation; this change does not establish thread safety.

The default build succeeds; full-client linking still fails with 48 unresolved
symbols. Evidence: transfer-order-build.log and client-link.log in build/native.

## Transfer cancellation lifetime

Canceled queued requests and canceled queue heads now return to the free list.
They are unlinked before notification so callback re-entry cannot rediscover
the canceled request. Pending relay cancellation honors the callback/context
replacement flags consistently. In-flight DVD requests remain alive until
their completion, retaining buffer lifetime.

The delayed-provider test first failed because canceled nodes were not recycled.
It now passes 64 queued cancellations with re-entrant cancellation attempts,
callback/context replacement, deferred in-flight completion, and 32 canceled
lower-priority queue heads, without additional request-pool allocations.
All eleven tests pass. These are deterministic lifetime tests, not proof of
thread safety with actual concurrent DVD callbacks. Full-client linking still
fails with 48 unresolved symbols. Evidence: cancellation-build.log and
client-link.log in build/native.

## Game-thread DVD completion consumption

The DVD provider callback now only publishes its result into one atomic slot
with release ordering. HSD_DevComPoll acquires that result on the game thread
and runs the existing memory/relay completion processing there. The queue
already permits only one outstanding DVD read, so no dynamic completion queue
is required. Queue mutation, cancellation and game callbacks now share the
polling thread; obsolete interrupt guards within devcom are removed.

Normal scene/error servicing, synth loading waits and movie shutdown waits now
poll completions. Preload/file waits already use scene servicing. ARAM remains
synchronous on the calling thread. Future native audio integration must keep
queue requests on this owner thread rather than invoking them from its mixer.

All eleven tests pass. The transfer test now checks that provider notification
does not run game callbacks until polling, including a real Windows worker
thread writing a read and publishing its result. Game callbacks verify their
thread identity. Existing immediate, delayed, nested and cancellation checks
also pass. This establishes the tested handoff; actual Aurora DVD/preload
integration, failure recovery and complete client shutdown remain unverified.
Full-client linking still fails with 48 unresolved symbols, including interrupt
guards elsewhere. Evidence: dvd-owner-build.log and client-link.log.

## Actual Aurora DVD transfer verification

The transfer queue now uses native CPU memory coherence instead of GameCube
cache store/invalidate calls. A new integration test opens the supplied ISO
through Aurora's actual DVD worker and transfers 473504 bytes of PlMrNr.dat
through HSD_DevCom to native RAM and ARAM. Both destinations match a reference
DVD read, and completion callbacks verify game-thread ownership.

The default build and all twelve native project tests pass. Unfiltered CTest
also discovers ten unbuilt Aurora dependency test targets, which are not run;
the project suite is selected with -R '^native_'. The full client still fails
to link with 46 unresolved symbols, primarily audio and console services.
This does not verify the complete preload system, asset parsing, failure
recovery, or a native client boot. Evidence: status-build.log,
dvd-integration-build.log and client-link.log in build/native.

## Client disc startup ownership

The original gmmain entry point now requires `--iso "path/to/disc.iso"` and
opens the image through Aurora before DVDInit or game asset queries. It checks
GALE/01 revision 2 to match the USA v1.02 data used by this port. Missing,
unreadable, or mismatched input exits before graphics initialization. Both
normal game-loop exit and the early startup quit close the disc worker before
Aurora shutdown. No build-machine path is embedded in the client.

The default build and twelve native tests pass (disc-startup-build.log).
The executable remains unlinkable due to the previously documented platform
and audio dependencies; these command-line and full shutdown paths have not
been exercised in the actual client. Disc identity checks are a revision
check, not a content hash or proof of unmodified assets. A user-facing import
flow and persistent installation settings remain future work.

## First native launch and controller status fix (2026-09-12)

The full client now links. Its first launch failed with a DEP exception after
PADRead: Melee allocated four original 12-byte PADStatus values, while Aurora
writes four native 16-byte values including extButton. The native declaration
now includes that field. LLDB verified startup proceeds through HSD and audio
initialization to gmMainLib_8015FBA4 / lbAudioAx_80028690 sound-bank loading.

The next observed stop is the DevCom source alignment assertion: the native
SFX loader requests metadata at DVD offset 16, which the old request check
rejects as not 32-byte aligned. No menu or gameplay acceptance yet. Evidence:
build/native/startup-pad-debug.log, pad-fixed-boot.log and
startup-assert-debug.log. All 23 native tests pass after rebuilding; they do
not establish live audio, controller gameplay, or scene acceptance.

### DVD metadata offsets (2026-09-12)

DevCom now distinguishes DVD file offsets from DMA source addresses. Native DVD
reads may start inside a 32-byte block; ARAM source and destination buffer
alignment checks remain. The real-ISO DVD transfer test verifies offset 16
against an independent full-file read. Client startup passes the SSM metadata
request and next faults in AXDriver_8038DA70 while parsing smash2.sem with
original big-endian/32-bit pointer assumptions. Evidence:
build/native/dvd-offset-build.log and dvd-offset-boot.log.

### First scene preparation reached (2026-09-12)

With native SEM decoding, startup prints the game banner and enters the original
game-mode loop. The next observed failure is HSD_MemAlloc returning null during
HSD_SisLib_803A6048(0x4800) in preloadState. Heap creation/ownership across scene
transitions needs investigation. The Versus menu is not rendered yet. All 24
native tests pass. See build/native/sem-native-boot.log for the LLDB backtrace.

### Native scene heap bounds (2026-09-12)

Preserved low-heap end addresses in lbHeap_80015900 now use uintptr_t instead
of s32. Previously ASLR could produce either an ignored/truncated bound or a
sign-extended bound above arena_hi. Replaced the console offset-view overlays
with direct Heap pointers while preserving transition ordering.

The handle allocator test now exercises actual lbHeap/lbMemory partitioning at
0x120004000 and 0x180004000, preserved low/high heaps, repeated transitions and
release back to the full arena. HSD/OS allocation is substituted in this range
bookkeeping fixture; synthetic addresses are not dereferenced. All 24 native
tests and the default/full-client builds pass. Runtime advances past text
allocation to lbArchive_InitializeDAT, where the original parser rejects the
big-endian file header. No menu rendered yet. Evidence:
build/native/scene-heap-build.log, scene-heap-test-build.log,
scene-heap-full-build.log and scene-heap-boot.log.

### Rumble data imported natively (2026-09-12)

LbRb.dat is retained as raw preloaded data and decoded through the immutable
HSD_Dat view. The importer creates native pattern pointers and host-endian u16
commands in scene-owned HSD storage. The original interpreter now extracts
opcodes from word bits rather than the first host byte. No per-frame import or
allocation was introduced. The original archive has 40 patterns and 256 words;
its single-root table layout is checked explicitly.

The real-ISO transfer test verifies every original record and command word,
insufficient capacities, and a timed motor on/stop program through the actual
interpreter. All 24 native tests and both builds pass. Actual device vibration
and complete pattern playback are not verified. Client startup passes rumble
loading and next enters gm_Scene_MemCard_OnEnter, where LbMcGame archive parsing
fails. Inspect native boot initialization/routing before porting console card
UI: the intended destination remains the Versus Mode menu before CSS.
Evidence: build/native/rumble-import-build.log, rumble-import-test-build.log,
rumble-import-full-build.log and rumble-import-boot.log.

### Direct native menu startup (2026-09-12)

The mode loop starts GM_MENU with GM_BOOT as its previous-mode selection hint,
so the existing menu route selects Versus / Melee. Startup still runs the full
gmMainLib_8015FBA4 defaults and mode on_init functions. It skips the console
memory-card boot scene and does not install its card-driven mode override or
Pikmin-save trophy check. Native profile persistence remains unfinished.

Main-menu entry no longer allocates card work or loads memory-card icons.
Power-time accounting uses the existing game clock directly, independent of
card UI enablement. Snapshot storage and its card assets initialize lazily on
snapshot-menu entry; pointers are cleared each main-menu scene so scene heap
resets cannot retain stale snapshot storage. Snapshot functionality remains
unverified and outside the current versus acceptance.

Native game-mode transitions do not flush or poll the legacy GameCube card
command queue. The direct route has no card-backed state to commit, and native
profile persistence will replace that queue. Character-select entry likewise
does not allocate card work or load the memory-card status art. The console
build retains both operations.

LLDB now reaches mnMain_Scene_OnEnter and stops loading MnMaAll through the
original archive parser. The menu has not rendered. All 24 native tests and the
default/full-client builds pass. Evidence: build/native/direct-menu-build.log,
direct-menu-full-build.log and direct-menu-boot.log.

### Menu camera and fog descriptor conversion (2026-09-12)

The native DAT graph importer now handles camera descriptors, linked world
positions and up vectors, fog, and fog adjustment matrices. Camera projection
payloads are sized by projection kind; invalid kinds/truncated extended payloads
fail conversion. Existing graph identity caching preserves shared references.
These are native HSD descriptors, not relocated 32-bit archive pointers.

Both original MnMaAll.dat and MnMaAll.usd now pass real-ISO camera/fog import
checks: 640x480 viewport/scissor, near/far 1/5000, linked eye/interest positions,
fog range 60/250, color, shared descriptor identity, and rejection of an invalid
projection. All 24 tests pass and melee_native links. Scene loading is not yet
connected: menu light/shape descriptors and native root ownership/lookup still
need work before replacing its original archive path. No additional rendered
menu acceptance is claimed. Logs: build/native/menu-camera-build.log and
menu-camera-test-build.log.

### Menu light descriptor conversion (2026-09-12)

Native DAT scene decoding now handles HSD light descriptors, linked lights and
world positions, and point/spot/raw attenuation payloads using the selection
rules in the original LObjLoad. Both original regional menu archives pass
checks for their three light descriptors, list terminator, flags, attenuation
flags, RGBA bytes and position links. Point/spot parameter branches are source
ported but are not exercised by these menu fixture checks. All 24 tests and
both builds pass. The client menu still uses its original archive path; native
menu root ownership and shape animation conversion remain before integration.
Evidence: build/native/menu-light-build.log and menu-light-test-build.log.

### Complete menu model/animation root import check (2026-09-12)

Added typed shape-animation joint, DObj and shape/AObj links to the native DAT
work queue. The original compressed FObj payload is retained for its native
interpreter; empty and shared links use the existing relocation/identity rules.

Real-ISO tests now import every named shape, model, joint-animation and material
animation root in both menu archives: MnMaAll.dat has 57 shape roots and 171
other model/animation roots (8239 total imported descriptors); MnMaAll.usd has
56 and 169 (8237 descriptors). All fit the shared 4 MiB test arena with 16384
record capacity. Camera, light and fog checks also pass. All 24 tests and both
builds pass. This proves descriptor conversion, not rendering/animation parity;
the original menu archive call path still needs native ownership and typed root
lookup integration. Evidence: build/native/menu-shape-build.log,
menu-shape-full-build.log and menu-shape-test.log.

### Native menu asset ownership connected (2026-09-12)

mnassets.c owns the original menu DAT bytes, a 4 MiB native descriptor arena,
16384-record import context and cached root pointers for the menu scene. Menu
and submenu callers now request native roots directly; removed the shared
mn_804D6BB8 archive pointer and its original variadic lookup calls. Known menu
root naming/layout conventions select the typed decoders; unknown schemas fail
explicitly. Name-table roots use native pointer arrays bounded by subsequent
root/data boundaries. Their UI behavior remains unverified.

Added light-animation, world-position-animation and animation-table imports.
The real client now completes the main menu's model, camera, fog, and animated
light lookups, then stops in HSD_SisLib_803A62A0 loading SdMenu.usd through the
original archive path. Menu text data conversion is next; no menu has rendered.
Both builds and all 24 native tests pass. Evidence:
build/native/menu-assets-build.log, menu-assets-full-build.log and
menu-assets-boot.log. Submenu navigation and scene re-entry remain unverified.

### Native SIS formatting stack (2026-09-12)

Moved the original text formatting push/pop implementation into sislib_state.c.
Return addresses now occupy native pointer-sized slots and return as pointers
at both interpreter call sites. Spacing and scale restore explicitly read the
big-endian bytes written by the original push operation. Pop starts at the last
occupied byte instead of reading one byte beyond an exactly full stack.

The new native_sis_state test covers nested addresses above 4 GiB, stack growth,
removal of an entry underneath a saved return address, non-removing restores
with different flag bits, signed spacing, unsigned scale, RGB/alignment state,
empty stacks and exact-capacity storage. Both builds and all 25 native tests
pass. Evidence: build/native/sis-state-build.log, sis-state-full-build.log and
CTest's LastTest.log. This is formatting-state coverage, not rendered text.

Inspection of the original ISO's SdMenu.dat and SdMenu.usd found one root,
SIS_MenuData at data offset zero, 1604 relocations and a 1604-entry pointer table
ending at 0x1910. Entry 0 is glyph image data; entry 1 is kerning byte pairs
(the existing SIS field names/types are reversed relative to actual use);
remaining entries point to encoded text. Neither archive has unaligned
relocation slots. Native table ownership and opcode operand conversion are
still required. Raw jump/call operands still use the old 32-bit archive pointer
representation; this commit changes only the interpreter's process-local
formatting stack. The startup blocker remains SdMenu.usd in the old archive
loader; no menu/gameplay acceptance is claimed.

### Native SIS scalar operands and line measurement (2026-09-12)

Both text interpreter passes now decode unaligned big-endian glyph IDs, scales,
spacing, delays and placement values explicitly. Extracted original line
measurement into sislib_measure.c and removed its 32-bit kerning pointer casts;
kerning is read directly through native pointers. The process-local formatting
stack shares the same scalar reader. Raw archive jump/call targets still need
native resolution when SIS ownership is implemented.

Extended native_sis_state with encoded text fixtures covering mixed standard
and custom glyphs, scale restoration, half-width spaces, negative spacing and
preservation of pre-existing formatting state. Both builds and all 25 native
tests pass. Evidence: build/native/sis-measure-build.log,
sis-measure-full-build.log and CTest's LastTest.log. Rendering branches compile
but have not been visually exercised. Startup still stops at the old SdMenu
archive loader; this commit does not advance the runtime boot checkpoint.

### Native SIS archive ownership and usage stop (2026-09-12)

The original HSD_SisLib_803A62A0 now loads SIS bytes and expands the root pointer
table into native storage, retaining encoded text and glyph data in the owned
DAT. dat_sis.c validates table extent against referenced payload boundaries.
sislib_data.c owns each font slot's DAT bytes/table and handles replacement,
explicit release and scene-heap reset. Removed the old SIS archive load/free
wrappers. Both interpreter passes resolve jump/call offsets against the font's
DAT; copied encoded strings retain those file-relative operands.

Both original menu archives import all 1604 native pointers with identity
checks against source offsets and capacity rejection without output writes.
The formatting test also exercises production SIS ownership, archive-relative
calls/returns, copied text and replacement/release using a synthetic DAT with
host file/allocation boundaries. Both builds and all 25 native tests pass.
Evidence: build/native/sis-data-build.log, sis-data-full-build.log,
sis-data-boot.log and CTest's LastTest.log.

The actual client passes SdMenu.usd loading and next asserts in the old archive
parser while gm_801BA8FC loads GmEvent.dat from mnMain_Scene_OnEnter. This is an
event-mode dependency reached unconditionally during menu initialization;
inspect whether it should be deferred to event-menu entry to preserve versus
scope when resuming. No menu has rendered and no local match has run. Other
borrowed SIS assignments in stages still need native ownership integration;
this is not an all-assets acceptance claim.

Work stopped when account usage reached 70 percent used / 30 percent remaining,
as requested by the user. Do not resume substantive work on automatic goal
continuations until the user authorizes resuming. The available goal tool has
no paused status; the project remains incomplete. No Dolphin process was
launched. The LLDB batch startup probe exited before this stop checkpoint.

### Native Versus menu presentation (2026-09-12)

The user explicitly authorized work to resume with GPT-5.6 Sol. Moved the
GmEvent.dat load from shared menu initialization to MENU_KIND_EVENT, its sole
consumer in this scene. This keeps deferred Event Match data out of the native
Versus startup path while retaining the original event-menu initialization.

Added a typed LbAd.dat decoder for the menu's core audio-load metadata. It
validates the exact four-language, 30-list-per-language schema, list alignment,
table placement and 0x83D60 termination, then expands the 120 pointers and 3360
big-endian sound IDs into scene-lifetime native storage. The original-ISO test
checks every native list pointer against its source offset and every list's
terminator, plus capacity rejection without destination writes.

Both builds and all 25 native tests pass. An LLDB panic probe then ran through
menu initialization for more than twenty seconds without reaching HSD_Panic.
A direct launch presented the animated, English-text Versus Mode menu with
Melee highlighted in the native 960x720 Aurora window; the user observed and
provided a screenshot of that exact run. This satisfies the pre-CSS menu
presentation checkpoint. Input navigation, CSS, stage select and an actual
local match remain unverified. GDI capture returned a black D3D12 surface and
is not evidence. No Dolphin process was launched, and both native diagnostic
processes were stopped after inspection. Evidence: build/native/defer-event-*.log,
build/native/audio-load-*.log, CTest's LastTest.log and
docs/evidence/native-versus-menu-2026-09-12.png.

### In-client framebuffer screenshots (2026-09-12)

Aurora and the native client already had asynchronous pre-present GPU readback,
but the only game-side trigger was the original Y+D-pad-up debug combination
behind the debug-ROM level. F12 key-down now requests the same capture from the
normal Aurora event loop in every build; key repeat is ignored. Captures are
written as `screenshots/melee-<time>-<sequence>.bmp`, and that generated folder
is ignored by Git.

A live D3D12 run focused the Freak Melee window and sent F12. Aurora logged a
successful save, and inspection of the resulting 960x720 BMP showed the full
Versus menu without window chrome or desktop content. This verifies the
screenshot command and gives future visual work a framebuffer-native evidence
path. Both native builds and all 25 tests pass. Evidence:
build/native/f12-screenshot-build.log,
build/native/f12-screenshot-full-build.log,
docs/evidence/native-versus-menu-f12-2026-09-12.png and CTest's LastTest.log.

### Native CSS, stage select, and match-transition assets (2026-09-12)

Character select and stage select now own immutable DAT bytes plus native graph
arenas instead of exposing relocated 32-bit archive pointers. Their complete
retail dat/usd graphs import successfully: the CSS expands 6342/6343 native
descriptors and stage select expands 2012/1963. Runtime navigation has reached
both screens at full speed after Aurora's first-use shader compilation.

The match transition now imports refraction parameters, rumble programs, every
effect archive referenced by `efAsync_DatEntries`, and `PdPm.dat`'s 0x184-byte
player-common table into native data. The 35 tested effect archives include all
fighter and Kirby-copy banks. Particle conversion handles nonzero command-ID
bases, absent banks, optional color palettes, spline descriptors, and native
pointer arrays without biased-pointer arithmetic. Rumble programs use fixed
persistent storage so a stage-select scene reset cannot invalidate them.

The client also exposes a local mailslot control channel for framebuffer
screenshots and optional virtual-pad commands. This permits task-owned testing
without sending desktop key events; live controller input remains authoritative
for user-driven acceptance. A real-ISO test covers every converted effect bank,
both regional CSS/SSS archives, refraction, rumble, and player-common truncation.

Live acceptance reached the selected stage's preload and stopped in
`grDatFiles_801C5FC0`, where the legacy archive parser attempts 32-bit in-place
relocation. Stage `map_head` conversion is the next startup boundary. No match
rendering or gameplay acceptance is claimed yet. No Dolphin process was
launched.

### Versus match initialization and asset inventory (2026-09-14)

The native client now imports many retail fighter common/character/costume and
animation-bank data, plus stage, item, effect, pause-overlay, and HUD assets.
The `IfAll.dat` HUD importer validates all 11 roots (ten model groups and one
scene), and `IfCoGet.dat` imports its model, animation, camera, and two lights.
`LbBf.dat` imports 16 flash entries as native command words. The common fighter
import now includes crowd audio configuration, the entry-platform model,
numeric/shake/respawn data, and both color-script tables. Tests use the supplied
original ISO; all 35 available native tests pass.

An LLDB integration run completed Versus match initialization and entered the
first simulation frame. It initially stopped because the entry-platform model
was unset; after that import, a follow-up run reached `ftCo_800BFFD0` and
stopped because the fighter color-overlay table was unset. That table has now
been imported and its entry-animation script tested, but no later live run has
established rendered gameplay. A
separate HUD issue was traced to `ifStatus_802F6194`
casting a joint to a game object to exploit matching GameCube field offsets;
it now walks joint child/next pointers directly.

The remaining startup and gameplay work is not a matter of only widening
archive pointers. In particular, `ftCharacterAssets_Import` currently copies
4-byte on-disc fighter action words into native memory without decoding their
PowerPC bitfields and pointer-bearing branches. `CmdUnion` is now four bytes
and common/color branch commands use script-relative operands, but the fighter
action and item importers still require conversion. This is the next systemic
porting milestone; neither static import tests nor the first-frame trace prove
that the match plays correctly. No Dolphin process was launched.

`docs/native_archive_inventory.md` records all 23 `PlCo.dat` roots, their
native conversion status and the repeatable ISO preflight command. Numeric,
shake, damage-shift, respawn and color data have since been imported and checked
against the retail ISO. All 15 flash scripts and fighter color entry 117 run
through the native color runner to termination in tests. An ISO preflight also
checks all 33 fighter archives, 9,994 action-script entries and 64,174 distinct
instruction starts. Their execution remains pending, as does the CPU graph.

### First rendered Versus frames and particle/item runtime fixes (2026-09-14)

A standalone native run reached Battlefield gameplay and advanced from the
two-minute Ready frame to at least 1:46, with HUD damage changing to 47%.
This is live match execution, beyond startup and first-frame tracing. The
fighter bodies were not visible in the inspected screenshot, and this is not
yet a visually correct or completed match.

The first match crashed because the generator list cursor was stored in a
32-bit integer and truncated its native pointer. A second crash came from
`psSetupTev` treating a particle as a two-word GameCube record: its write to
word 1 modified the high half of the particle's native `next` pointer. The
generator, particle and TEV interfaces now use typed native pointers. The
next match crash was a particle cleanup function treating separately linked
globals as one fixed GameCube memory layout; it now accesses the actual list,
joint array and allocator. The later 1:46 run reached an unset item-common
color table. An ISO preflight and importer now cover all seven entries and six
scripts in `ItCo.usd` root 5.

Native panics now log a stack of return addresses and the executable base for
offline symbol lookup without pausing the render worker in LLDB. The 40 native
tests include the new color-script corpus preflight. These fixes require a
follow-up Battlefield run; the current stage importer still accepts only
`GrNBa.dat`.

### Native magnifier ownership (2026-09-19)

The magnifying-glass HUD now addresses its six player records and five copied
image descriptors through typed members. The prior teardown rebuilt each GObj
pointer from a `u32` base address, truncating the native global address, while
initialization cleared only the original 0x74-byte GameCube prefix even though
native pointers enlarge every player record. Initialization now clears through
`offsetof(ifMagnify, image_descs)`, preserving the original boundary without
depending on the 32-bit layout.

All 42 native tests pass. A trace-free, one-stock Final Destination match with
the standard four-character test lineup reached the results screen, exercising
magnifier construction, match rendering, and scene teardown without a crash.

### Native common-item records and stress path (2026-09-19)

The common-item importer now converts every pointer-bearing special-attribute
record in `ItCo.usd` instead of copying GameCube words into native structures.
Food variants own 28 typed joint/heal/offset records, both mushrooms own their
two animation graphs, and Warp Star owns its seven animation entries. Starman's
special word is now an explicit 600-frame duration rather than a pointer-shaped
value. Any remaining common-item attribute block containing a relocation is
rejected until it has a typed decoder.

Item dynamics now preserve both parts of the original archive record: four
flexible-bone descriptors and one collision-bone descriptor. The collision
initializer consumes those native arrays directly. Item animation-table sizes
come from DAT object boundaries; the previous pointer-pattern heuristic
mistook Parasol's second state for an article descriptor and imported only one
of its eight states. The corrected import contains 262 states and 162 scripts,
including all eight Parasol states and the other previously truncated
five-state table. Runtime transitions assert that animation indices remain in
the imported range.

The container-drop table no longer depends on `it_804A0E50` following the
random-spawner global in GameCube linker order. It addresses the table directly
and uses named native count, kind, total-weight, and cumulative-weight fields.
A trace-free accelerated Final Destination run used Ganondorf plus CPU Zelda,
Ice Climbers, and Game & Watch, 99 stocks, and very-high items. It advanced for
more than two minutes, through repeated container drops and item state changes,
reaching 88/93/94/95 stocks before intentional termination. This is a broad
common-item crash smoke test, not behavior acceptance for every individual
item.

A clean relink of the fighter-archive test exposed that the exact state-table
boundary used by `ItCo.usd` cannot be applied to fighter archives. Fighter DATs
may contain relocations into a state array and padding before the following
object, so their article tables retain validated structural termination. The
common item bank uses its exact target boundaries, preserving all 262 states
and 162 scripts, while the 33 fighter banks retain their expected 87 articles
and 116 scripts. Both fresh retail-corpus tests pass.

Fighter wait-animation choices now use explicit native `animation_id` and
`weight` fields. The previous union overlaid two 32-bit values with a native
pointer, then truncated that pointer to choose an animation; on x86-64 the
weight occupied the pointer's upper half. The corpus test validates every
present wait table across all fighters, including termination, action bounds,
and a complete 100-point weight range. A subsequent accelerated Final
Destination run with the standard four-fighter lineup remained live from 99 to
92/95/95/96 stocks before intentional termination.

### Native fighter-item attribute graphs (2026-09-19)

A relocation scan now covers every special-attribute block used by the 87
fighter articles. It found all remaining pointer-bearing formats in one pass:
Link and Young Link's boomerang, hookshot, and arrow; Sheik's chain; Ness's
yo-yo; the Ice Climbers' rope; Samus's grapple; and Game & Watch's article
model references. This is the bounded conversion inventory for this archive
layer rather than a runtime-crash queue. The Ice Climbers rope was already a
typed import.

Sheik's chain now imports both joint graphs into native pointers and validates
its link count against the 15-entry input-history capacity. Chain construction
returns a native success value instead of truncating its tail joint into an
`int`; partial allocation failure frees links without reading an already-freed
node and removes the parent item cleanly. The retail fighter corpus test checks
the converted count and both joints. A trace-free direct match with P1
Ganondorf and P4 human Sheik visibly created, held, and retracted the chain on
Final Destination without a crash. The other inventoried formats remain to be
converted before their moves count as runtime-validated; the Link-family work
below is the next completed group.

Link and Young Link now import the boomerang's two models and animation
bundles, the hookshot's three models, and the arrow's two models as native
graphs. Their extended-attribute type also keeps the three scalar fields at
`0x94`, `0x9C`, and `0xA0` as 32-bit values. Treating those unknown scalar
fields as native pointers had widened the structure and moved the down-air
shield descriptor from its file-defined `0xC4` offset.

Fighter item-state scanning now stops only at descriptor addresses actually
listed by the fighter's article table. The previous shape heuristic truncated
Link's bow after one animation state because its second state happened to look
like an article descriptor; it also omitted three valid scripts elsewhere in
the fighter corpus. All 87 fighter articles now contain 119 converted scripts
and 306 item commands. The corpus test validates the complete Link and Young
Link state counts and their converted model pointers. Trace-free Final
Destination matches exercised arrow, boomerang, hookshot grab, and bomb for
both characters, with each match remaining live until intentional termination.

### Native Game & Watch article attributes (2026-09-20)

Game & Watch's ten fighter articles now import their shared model-visibility
descriptor instead of retaining its relocated 32-bit DAT address. Each
descriptor owns native pointers to the retail normal-model and color-overlay
joint-index lists used by the item's render pass. Chef additionally imports its
0.5 wall-bounce multiplier, 80-frame active lifetime, 30-frame inactive
lifetime, and all five food-specific velocity, gravity, terminal-velocity, and
rotation records. The runtime structures and call sites now name these fields
and use typed pointers rather than treating the special-attribute block as an
array of native pointers.

The retail fighter-corpus test verifies both visibility lists for all ten
articles and Chef's complete numeric table while still importing all 33 fighter
archives, 87 articles, 119 scripts, and 306 item commands. A trace-free direct
match showed the Chef pan in its active animation and remained live while the
control channel exercised neutral, side, up, and down specials plus grounded
and aerial attacks. A second accelerated one-stock run used P1 Ganondorf with
CPU Zelda, Ice Climbers, and Game & Watch and reached the results screen, with
Game & Watch surviving to second place. Both runs were intentionally terminated
after evidence capture.

### Native Ness yo-yo attributes (2026-09-20)

Ness's yo-yo article now imports its exact `0x5C` retail attribute block into a
typed native structure. The import decodes the 20-link chain configuration,
up-smash and down-smash charge lengths, link constraints, release and gravity
physics, spin rates, action-frame gates, two model joints, and the yo-yo
material animation. The obsolete trailing scalar previously implied by the
32-bit GameCube layout has been removed, and the runtime consumers now use
descriptive field names rather than encoded offsets.

The fighter-corpus test checks the retail scalar values and all three imported
graphs while continuing to load every fighter archive. A trace-free direct
match used P1 Ganondorf and P4 Ness on Final Destination; IPC input exercised
both yo-yo smashes, screenshots captured the native string and yo-yo during
each action, and the client remained live after both actions completed. The
test process was then intentionally force-stopped.

### Native item model visibility (2026-09-22)

Item construction initialized its `xDAA` flags by writing the original raw
GameCube byte value `1`. In the Windows bitfield layout that set `b0`, while
the retained item draw callback requires `b7` before it submits the model.
The initialization now clears the byte and sets `b7` by name. A live
Ganondorf-versus-CPU Zelda match on Final Destination with very-high items
visibly rendered a Poké Ball and barrel at the same time; the capture is
`build/native/item-visibility-fixed.bmp`. This verifies the common-item draw
gate, not every article's model or animation.

A second run split Ganondorf, Zelda, Ice Climbers, and Game & Watch across two
local UDP clients with very-high items. Both clients displayed the same item
models during play (`item-netplay-{host,guest}-visible.bmp`) and the same Ice
Climbers winner screen (`item-netplay-{host,guest}-result.bmp`). The clients
compared 2961/3725 confirmed peer checksums with zero mismatches, rejected
input packets, or rollback corrections. This covers one same-machine online
round with visible items; it does not establish every item or network route.

### Shared shader-cache contention (2026-09-22)

Two clients launched together previously logged `database is locked` while
opening Aurora's Dawn cache and writing its pipeline cache. Aurora now waits
briefly for SQLite's cross-process writer lock before cache setup or writes,
and compresses a Dawn cache value before taking its write transaction. Two
normal clients launched together without either error. A separate client also
started after a test process held both cache databases' write locks for three
seconds. This checks cache access under contention; it does not measure or
eliminate first-time shader compilation time.
