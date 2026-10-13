# Workshop source validation and preview path

Workshop keeps its existing layout and routes Validate through the existing
controller's actual draft save: edited DAT/animation/model, owned asset facts,
parameters and separate FMS files are saved before checking the composed source.
The editor stages that FMS and invokes `native_content_tool source-check` in a
hidden process. The tool prepares the real source candidate from the project's
`assets.fma`, three copied DATs and parameter sidecar; checking publishes no
behavior file, cache entry or live program. No generated C or DLL source hash is
used by this path.

Build uses `script-build` with the same candidate validation and commits
`behavior.fms` only after success. Workshop reopens the complete family and
updates its in-memory source and matching asset facts only after all member
builds/reopen succeed, retaining existing DAT allocations and editor undo state.
Diagnostics use the existing status/problem controls and composed-file line
mapping. A failed build leaves the accepted in-memory source and current live
preview selected. Project/rig save failures identify owned asset facts rather
than silently copying donor metadata over an edited rig.

Apply/Test creates a separate candidate view and supports Single source
projects explicitly. It calls `nativeFighterProject_Build` directly, then
canonical packet Parse, non-selectable Cache and runtime cached-candidate
validation. Only a successful result reaches `nativeHome_RequestPreview`.
That request queues the scene restart; compilation/checking never replaces an
active actor's pinned program. Unsupported families and candidate failures keep
the current preview and show the existing status UX.

Installed CSS listing checks the canonical Single packet, nonempty FMS and
actual asset-facts decoder. It no longer treats a native Definition/code key as
ordinary source behavior, and performs no heavy source compilation per frame.
Complete package preparation remains the scene/loading gate.

Verification boundaries are explicit. The existing actual Rml clips/session
integration passes 53 checks with the data-only project header; affected
Workshop/controller/model/CSS units compile with the real client flags. The
source content store fixture uses the actual facts codec and extracted exact
pure validators, retaining cache/install/identity and canonical payload checks.
Its DAT payloads are opaque format-test bytes, not gameplay assets. The separate
`workshop_source_script` fixture passes 14 checks through the exact production
process orchestration against the rebuilt source tool and a private copy of a
real Mario draft. Read-only and rejected checks preserve the accepted behavior;
successful publication changes only `behavior.fms`, and all original project,
source and tool hashes stay unchanged. Its receipt is
`A:/FreakMelee-test-archives/workshop-source-script-oct07-final/receipt.json`.
Actual hidden practice preview acceptance is recorded separately by the root
worker; these UI/format fixtures do not establish that outcome.

Behavioral comparison fixtures retain their C oracle through the separate
test-only `native_script_reference_exporter` target. It calls the reference
`GenerateC` API directly and accepts an explicit asset directory and parameter
sidecar; the inline-tuning fixture supplies both. Owned-state oracle requests
also supply `--construction ROOT FIRST COUNT`; ordinary FMS does not contain a
base hint or resolve a donor Definition to infer those bounds. No reference-generator source
or compiler/source-root configuration remains in the client or ordinary
content tool. Its focused four-case receipt covers a real facing-speed export,
inline Link/Clink tuning and failed-source/missing-sidecar output preservation
at `A:/FreakMelee-test-archives/test-reference-exporter-oct07/receipt.json`.

The source guard/dispatch fixture checkpoint is recorded at
`A:/FreakMelee-test-archives/source-owner-binding-fixtures-oct07-final/receipt.json`.
It covers twenty ownership failures, six owned-rig failures, actual transition
and receiver VM execution, and top-level encoded factory-domain routing. The
factory leaf remains explicitly unavailable in the routing test: failure must
preserve arguments and output. Controlled metadata and stock-only primitives
are distinguished from the root worker's real source-package/live acceptance.
