# AI assistance for fighter creation

Accepted 2026-11-17. Implementation is in progress.

## Progress

- Shared fighter-state/move-file loading and composition now live in the native
  fighter authoring code; the Workshop and agent tools call it directly.
- Structured CLI and native stdio MCP support read-only environment, project,
  script-file, attribute, action and documentation inspection. Editable source
  provenance and its relationship to validated behavior are reported.
- The installation launcher has an independent MCP entry that follows the
  active version without launching the game or updating it. The packaged
  `START_HERE.md` explains setup and currently available tools.
- The first contract test covers a real Mario clone, unchanged draft files,
  split sources, malformed source detection, pagination, MCP resources/errors,
  bounded input and live request/reply through the launcher across two versions.
  It also passes against an extracted setup ZIP with a Windows-only PATH and
  the packaged launcher. The game/tool build, family-package regression and
  release-payload checks pass. This verifies the read-only foundation only.
- Complete domain editing coverage, visual/practice tools and full
  repository-free acceptance remain to be implemented. The remaining practical
  authoring guides are also open; completed slices are recorded below.
- `scripts.check` validates the editable source set through the same independent
  native candidate operation as the Workshop's validator. It collects errors
  and maps them to their original move files. `parameters.read` exposes typed
  overrides, and shared parameter import preserves explicit tuning and source
  line positions. Fresh/split Mario checks, multiple independent errors,
  parameter conflicts, repair and preservation of accepted source pass locally.
- The installed reference now contains 88 pages and 13 linked source examples.
  A shared build step resolves offline links and section anchors, labels optional
  online developer references, and replaces test-fixture links with practical
  authoring checks. Both MCP resources and the in-game reference can read the
  bundled examples. Package preparation rejects undeclared files and broken
  section links; native tool, family, parameter, reference and payload checks pass.
- `scripts.update` saves source/parameter batches through shared authoring code,
  with an inspected disk revision as its precondition. A native directory lease
  protects open Workshop families from offline writers, including legacy CLI
  commands. A recoverable journal rolls back failed/interrupted source batches;
  saved-source checks preserve raw external edits. Source validation remains
  isolated, and the live owner commits accepted behavior itself. Agent editing,
  read-only, parameter, family, pose, reference and release-payload checks pass.
  Read and write CLI/MCP checks also pass from an extracted setup ZIP with a
  Windows-only PATH. No public update was uploaded.
- Same-user asynchronous named pipes now connect the tool to an explicit live
  instance. Discovery returns the executable and source/build identity. A shared
  source/configuration fingerprint rejects mismatched binaries before requests
  reach the editor. Both endpoints use the same schemas and result types.
- Live source and parameter edits run at the editor boundary and share its undo
  history. Member/workspace/source selection, unsaved inspection, revision
  guards, gesture/gameplay refusal, recent operations and disconnect controls
  are implemented. The real hidden/muted client passes read/write/undo/redo,
  manual textarea input, stale and offline-writer conflicts, atomic failure,
  build mismatch and disconnect checks; draft files remain unchanged. Pipe
  framing/partial/large-message/cancellation checks also pass. Extracted-package
  checks verify these operations with a Windows-only PATH, including in-game
  access disable/re-enable and a separate run using only the shipped tools.
  Build/install, other asset domains and preview/testing operations remain.
- The Save button and `session.save` now capture the whole fighter family and
  commit assets, source files and parameters through one recovery journal.
  Saved-disk revisions guard raw external edits; successful saves retain undo.
  Parameters and HUD-image imports use the same save path. Native checks cover
  single and paired fighters, invalid candidates, a blocked mid-save write,
  journal recovery and disk conflicts. A real hidden/muted client verifies
  source/parameter save, unchanged accepted behavior and undo/redo after saving.
  The same live save checks and CLI/MCP/launcher checks pass from an extracted
  release using a Windows-only PATH.
  Build/install remain separate work.
- `session.scripts.check` and the Workshop's Check script button now validate
  the selected member's unsaved sources, parameters and assets through the same
  isolated native process. Results identify the checked live revision and source
  locations, collect independent errors, and require a matching validator build.
  Checks preserve disk files, accepted behavior, preview and undo history.
  Real Mario and Zelda/Sheik checks cover both members, multiple source errors,
  an invalid in-memory model and temporary-file cleanup. The hidden/muted live
  client verifies read-only checks; the actual UI button and problem links also
  pass without saving the unfinished source. The live checks and CLI/MCP/launcher
  checks also pass from an extracted setup ZIP with a Windows-only PATH; all
  nine current authoring/reference/payload regression checks pass.
- Live build and installation now use the same whole-family operations as the
  Workshop buttons. An isolated native process checks every member and caches
  the complete package; acceptance saves all members through one recovery
  journal. Failure preserves the previous accepted behavior and editor history.
  Unsaved preview refresh and practice start/stop preserve saved draft files.
  Inspection distinguishes effective fighter data, accepted source, cached build,
  the package loaded by the actual preview actor and the installed package.
  Exact package export preserves existing output files. Native checks cover
  paired builds, multiple errors in both members, interrupted acceptance,
  preserved asset pointers, stale packages and explicit installation. All ten
  authoring/reference/payload checks pass. The extracted setup also passes live
  build/preview/practice/install/export and CLI/MCP/launcher checks with a
  Windows-only PATH. No public build was uploaded. Domain editing, controlled
  tests/capture, agent feedback and full packaged acceptance remain open.
- Agent bug and feature reporting now uses the existing F2 capture code,
  binary uploader and administration queue. Prepare captures pixels and bounded
  gameplay diagnostics immediately; inspect exposes the original evidence and
  draft/preview identity. Submit/status reuse sealed files and a durable report
  ID across failures and process restarts. Local task/issue keys suppress repeat
  captures, while random report IDs avoid collisions between players. Reports
  persist confirmed screenshot completion and image hashes, so a partial file
  left by an interrupted capture cannot be mistaken for a completed screenshot.
  Reports
  carry agent/category labels, descriptions, repro steps and optional pins;
  source files and assets are excluded. The Workshop Agent tools panel lists
  their descriptions and status. The native owner tests cover F2 behavior,
  partial diagnostics, duplicate suppression, retry and receipt handling. A real
  hidden/muted client passes the F2 wire-format roundtrip against a private test
  endpoint, including failure, restart recovery, altered-attachment refusal and
  successful receipt. That test and CLI/MCP/launcher checks also pass from the
  extracted setup with a Windows-only PATH; all twelve related regression
  checks pass. No test report or build was sent to the public service. Remaining
  work includes the domain editing tools, structured clone/import, controlled
  preview/practice evidence and full packaged acceptance exercises.
- Saved and live action tools now expose actual action IDs, playback flags,
  reachable commands, unrolled loop occurrences and named property definitions.
  Hitbox fields include element, knockback, radius, attachment, hit eligibility
  and duration. Sound, effect, throw and other command operands share the native
  codec and the Workshop's names. New events are constructed directly, including
  the first script on an empty action; the UI Add event menu uses this same
  operation. Property groups commit atomically, shared commands are localized,
  timing edits preserve repeated patterns, and live edits share editor undo.
  Proven stock lifetimes can be converted into duration-owned hitboxes.
  The timeline reader now preserves actual execution order when an absolute
  wait targets a frame already passed, retaining its debt for relative waits.
  All thirteen core authoring/reference/payload checks pass, as do native hitbox,
  action duplication and fighter-event checks. The action CLI, MCP/launcher and
  hidden/muted live checks pass from an extracted setup with a Windows-only
  PATH, including the real UI Add hitbox button, undo, preview, practice,
  installation and export. Paired build/save fixtures now explicitly construct
  both Zelda and Sheik and verify two-member coverage; legacy clone-stock creates
  only one member. Remaining work includes other asset domains, structured clone/import, controlled test
  evidence and full packaged acceptance. No public build was uploaded.
- Action property updates now accept up to 256 distinct events as one atomic
  edit and one live undo step. Handles refer to the original inspected revision;
  shared-command localization preserves their targets. Replies include requested
  changes and actual quantized properties. Action labels and editor visibility
  are readable/editable through the same operation as the UI controls, persist
  through the journal and whole-family Save, and share the existing label undo.
  Native checks cover shared-script isolation, duplicate handles, a later invalid
  entry, no-op metadata, label rejection and paired-member metadata paths.
  All thirteen core checks pass. Extracted-package CLI/MCP/launcher and real
  hidden/muted Workshop checks pass with a Windows-only PATH, including grouped
  undo/redo, label visibility in the UI and labels saved through build/install.

- Saved/live animation tools now discover clips, model bones, native tracks,
  keys and evaluated poses. They create independent rest clips, duplicate clips,
  change duration, insert channels, copy endpoint poses and edit/delete/transform
  keys across selected bones. Key changes use the same operation as the graph
  gestures and inspector, report quantization and reject partial batches. Clip
  creation/duplication carries the correct rig binding while preserving target
  action events and playback settings. Foreign bindings expose raw tracks and
  samples; bone-local editing reports the binding requirement.
  Live changes share structural animation undo. A read-only preview operation
  reports the actual frozen actor's clip, frame and bone transforms. The real
  hidden/muted client verifies an edited pose, Undo/Redo, unchanged disk before
  Save, persisted keys, practice, installation and export. All fifteen core
  checks pass, including semantic retiming/curve evaluation checks replacing an
  old fixed-byte assertion. Extracted-package CLI/MCP/launcher and live tests pass
  with a Windows-only PATH. The bundled reference contains 89 pages and an
  animation authoring guide. Pose-copy capacity and skipped channels are reported;
  this uses the current editor's native pose buffer. No public upload occurred.

- Common attributes now have saved/live grouped edits by inspected names, with
  native type/range metadata and requested/applied float32 values. Both routes
  use one atomic native operation. Rejected groups preserve all original bytes;
  saved edits use the family lease, disk revision and recovery journal. Paired
  member edits preserve the other member. Live edits use the current workspace's
  history, including Model, and synchronize the ordinary attribute fields.
  Enter-to-confirm now uses the inspector's capture handler; the previous bubble
  handler did not receive Enter reliably. A hidden/muted client verifies manual
  entry, cancellation, grouped undo, Model undo/redo and unchanged disk before
  saving. All sixteen core checks and extracted-package CLI/MCP/launcher/live
  checks pass with a Windows-only PATH. The installed guide covers this tuning
  workflow. No public upload occurred.

- Structured stock discovery, installed-fighter discovery, stock/package cloning,
  package import and closed-draft rename are implemented. The hub and agent tools
  share complete native draft construction: paired families, owned templates and
  tuning, split move files, presentation images and a per-project agent guide.
  Construction publishes a new destination after preparation; existing folders
  and the source installation are preserved. Failed preparation stays outside
  the visible draft library. Rename uses the family lease, inspected revision
  and recovery journal, and reports its committed revision from inside the lease.
- Saved-draft installation now accepts an exact cached build through the same
  native acceptance and installation operations as the Workshop. It verifies
  every member's current sources/assets, saves accepted behavior and installs
  the package. A stale build preserves draft files and the prior installation.
  Native checks cover complete Mario, Zelda/Sheik and Popo/Nana creation,
  per-member source validation, import/clone independence, protected rename,
  incomplete-installation failure and stale-build rejection. All seventeen core
  checks pass. The actual hidden/muted Mario client now starts from the structured
  clone operation and verifies live ownership, editing, preview, practice and
  export. Extracted-package CLI/MCP/launcher and live checks pass with a
  Windows-only PATH. The installed reference contains 90 pages, including the
  creation guide. No public upload occurred. Model/resource/presentation tools,
  controlled preview/practice evidence and full acceptance remain open.

- Saved/live model tools now inspect meshes, stable skeleton identities, local
  rest transforms, actual bind-space vertices and weights, materials, texture
  sequences and source-aware reimport plans. They apply GLB/glTF import batches,
  matched reimport, compensated rest batches, weight strokes, material/render
  properties, assignments, image embedding, labels and extra-bone edits through
  the native authoring operations. Rejected candidates preserve the complete
  original assets and metadata. Saved edits use the family lease and journal;
  live edits reserve whole-asset Undo/Redo before publication and reload preview.
  Active weight strokes now prevent concurrent agent mutations.
  The UI and tools share material draw-pass/pixel-engine encoding. Material
  assignment also updates joint draw flags. A live test exposed textured
  geometry without required UVs reaching a fatal renderer path; the native
  assignment/texture/render operations now reject that combination with a
  diagnostic. Texture-image replacement can resolve inspected sequences beyond
  its former fixed 128-entry buffer.
  All eighteen core checks pass. Native and extracted-package checks cover
  imports, reordered reimport, retained painted weights and bind-space geometry,
  extra bones, late-batch failure, member isolation and no-op revisions. Hidden,
  muted live clients verify whole-asset/metadata Undo/Redo, preview reload,
  unchanged disk before Save, saved build and existing authoring/install/export
  behavior. Extracted CLI/MCP/launcher checks pass with a Windows-only PATH.
  The bundled reference contains 91 pages. Advanced texture pipeline fields,
  editor-only model selection/visibility, resources, presentation assets,
  controlled test evidence and final acceptance remain open. No public upload
  occurred.

- Texture tools now inspect and edit the Model inspector's mapping, transforms,
  contribution flags, wrapping, filtering, LOD and supported color-combiner
  inputs through a shared native writer. The UI uses that writer too. Mapping
  validation reads staged properties, so switching between UV and reflection
  checks the mapping actually being applied. Texture-sequence inspection now
  distinguishes animation IDs from physical layer indices.
  Active color/alpha expressions reject inputs the native expression evaluator
  cannot evaluate; those invalid choices were also removed from the inspector.
  Animated costumes retain texture IDs tied to render slots. Inspection reports
  that constraint and the inspector disables that ID field; image replacement
  remains supported without changing animation bindings.
- Material assignment now stages the costume and fighter together, retains
  matching material animations, remaps expression selectors and admits the
  complete result before publication. The UI and agent share the same Model
  operation and whole-asset history. Assignments removing required expression
  textures are rejected. The standalone regression caught an aliased serializer
  input/output lifetime bug in the new assignment path; separate staging storage
  fixes it. All eighteen core checks pass. Extracted-package CLI/MCP/launcher,
  model authoring and hidden/muted live checks pass with a Windows-only PATH.
  Live checks wait for the requested package identity on the actual preview
  actor, cover material/texture Undo/Redo, reject unsupported expressions and ID
  changes, preserve saved files before Save and successfully build saved edits.
  Editor-only model selection/visibility, resources, presentation, controlled
  practice/preview evidence, output schemas and final acceptance remain open.
  No public upload occurred.

- Model view operations now expose mesh/material/bone selection, editor-only
  mesh visibility, Edit/Pose/Animation modes, action-clip seeking and playback.
  Temporary local poses accept atomic multi-bone changes and rest resets without
  changing saved rig data or animation keys. The ordinary Model UI uses the same
  view and temporary-pose operations. Skeleton context retains visible bones;
  selection failures and stale requests preserve the previous view.
  Model preview now identifies the edited draft to the frozen-pose reader and
  evaluates its current animation tree. Live checks read actual actor transforms,
  play/pause and long-clip frames, and exercise the real mesh-eye, bone-selection,
  pose-field and playback buttons. Animation Undo restores the clip but retains
  its existing conservative unsaved marker; tests compare restored clip metadata
  and unchanged disk files directly. All eighteen core checks pass. Extracted
  CLI/MCP/launcher, offline model and hidden/muted live checks pass with a
  Windows-only PATH, including actual UI input, fractional end-frame clamping,
  long-clip actor readback and saved model build. No public upload occurred.
  Resources, presentation, controlled practice/capture, typed output
  schemas and final acceptance remain open.

- Resource tools now inspect typed named entries, hashes, dimensions, native
  capacities and article/sound/effect inventory. PNG/blob import and removal
  stage one atomic batch; offline changes use the family lease, revision guard
  and recovery journal. Live changes use the active editor or Model Undo history,
  keep disk unchanged and retain history across Save. The ordinary stock-icon
  and HUD-emblem importer now uses that operation and waits for explicit Save.
  Export preserves existing output files. Item-blueprint creation supports an
  explicit DAT/Article offset or a named common ISO item and admits its actual
  Article/model/command graph before publication. Blueprint imports use the same
  admission. Direct-write resource commands were removed and their production
  callers updated. Inspection and the installed guide explain current audio and
  particle import limits. All nineteen core checks pass. Extracted CLI/MCP/
  launcher, resource transactions and hidden/muted live checks pass with a
  Windows-only PATH, covering atomic rejection, exact exports, native limits,
  deferred Windows-lock recovery, independent ISO-byte comparison, script
  references, paired-member isolation, both history owners, ordinary HUD import,
  and the requested package loaded on the actual preview actor. The reference
  now has 92 pages and 13 examples. No public upload occurred. Presentation,
  controlled practice/capture, typed output schemas and full acceptance remain.

An older fighter-event package test also fails while preparing its source. Its
fixture omits current parameter/fighter declarations, and its runtime probe
assumes a compiled DLL. The same preparation failure reproduces with the
extracted feedback-stage reference tool. It needs an interpreter-based
replacement and is separate from the passing native fighter-event check.

A player should be able to give their coding agent the installed Freak Melee
folder and ask it to create or change a fighter. The installation should supply
the information and tools to inspect a draft, edit it, see the result, test it,
and produce an installable fighter. The player's Melee ISO supplies stock assets.
The repository, a C/C++ toolchain, Python, Node.js and hosted credentials should
be unnecessary for this workflow.

The first implementation covers the capabilities of the current Workshop.
Kirby's deferred behavior and arbitrary cross-fighter move transfer keep their
existing scope. New capabilities should serve any custom fighter.

## What ships today

The `2026-11-17-connections-1` installation contains:

- `native_content_tool.exe`, with cloning, project inspection, script checks,
  action/event editing, animation keys, model import, weights and packaging.
- 85 Markdown scripting reference pages and 26 FMS template files.
- The Workshop and its live preview, interpreter and project editing code.

The source also contains `melee_native_control.exe` and a Windows mailslot
control channel. The helper is excluded from the release. Requests mostly
simulate UI/controller input, and inspection results go to the game log.
It provides useful recording and test facilities, but lacks correlated replies
and a complete semantic Workshop interface. There is no packaged MCP service.

The audit found these obstacles:

| Area | Missing capability |
| --- | --- |
| Discovery | Identify installations, live Workshop sessions, user-data roots, drafts, members and supported operations without guessing paths. |
| Documentation | Follow the reference offline. A file-target audit found 61 distinct relative link targets absent from the package, including examples, guides and repository test fixtures. |
| Script files | Use the same fighter-state and move-file composition as the Workshop. The CLI currently checks a supplied combined script. |
| Structured results | Obtain consistent property schemas, typed values, diagnostics and operation results. Existing commands mix human-readable output with some JSON. |
| Editing coverage | Create and change every relevant editor property through named operations, including operations presently exposed only in the UI. |
| Live editing | Work with unsaved state, undo and preview without overwriting the player's edits or reopening the editor after each change. |
| Verification | Know which revision is previewed, tested, installed and selected; capture images and behavioral evidence directly. |

These findings come from the packaged manifest, `native_content_tool.c`,
`native_source_content_tool.c`, `native_workshop_authoring_tool.c`,
`dbnativecontrol.c`, and the Workshop script-file implementation.

## 1. Make the installation self-describing

Ship a short `START_HERE.md` beside the authoring tools, an agent guide, and a
small agent-readable workspace guide when a project is created. Explain where
the project lives, which files are editable, how to check changes, and how to
preview, test and install them. Keep the instructions independent of a specific
agent vendor. Include copyable terminal and MCP configuration examples.
Use the existing release launcher's active-version selection for a stable tool
entry point, so an auto-update does not break the agent's configuration.

Add structured discovery for:

- Build and source identity, supported authoring operations and actual limits.
- Configured ISO availability, user-data location and installed library.
- Running instances, open draft, selected paired-fighter member and dirty state.
- Draft identity, source files, resources, dependencies and installed package.

Use scoped responses and pagination for large action, bone, vertex and key lists.
Search should return a small useful result before an agent requests full data.
Report unsupported operations and absent assets explicitly.

Document and expose the difference between editing a draft, saving it,
validating its scripts, building a package, installing it and selecting it.
Include which draft revision produced each result. This should also help resolve
the existing confusion about whether Workshop changes are active in gameplay.

## 2. Complete the offline authoring reference

Audit the shipped reference's links, anchors and examples. Package useful guides
and examples, repair template paths, and replace links to internal test fixtures
with self-contained explanations. Label optional repository research links as
external material. Add a packaging check for broken local links.

Add practical guides for cloning a fighter, changing a normal attack, writing a
special, creating an article/item, editing an animation and importing a model.
Each guide should end with preview/test instructions and the install step.
Keep explanations in the existing Godot-style format: purpose, inputs, outputs,
units, examples, related operations and meaningful limits.

Expose searchable documentation and machine-readable reference data:

- Script symbols, signatures, callbacks, scope, parameters and return types.
- Action/event properties, enumerations, defaults, ranges and relationships.
- Fighter-specific action mappings, rig hierarchy and semantic bone roles.
- Coordinate systems, facing conventions, angles, frame timing and units.
- Curve interpolation/quantization limits, rig import requirements and resource
  constraints.

Use existing runtime/editor descriptors as the source where available. Where
coverage is missing, add shared descriptors consumed by the tools and reference.
Keep prose authored and readable; avoid maintaining a separate guessed API list.
Distinguish stable project identity, changing package hashes and temporary
selection handles.

## 3. Expose native authoring operations through CLI and MCP

Extend the shipped `native_content_tool.exe` with structured commands and a
`mcp` stdio mode. Both call the same native authoring operations directly.
Replace affected CLI dispatch and UI-specific edit paths as necessary; avoid
a second editing implementation or an MCP process that scrapes CLI text.
Keep operation implementations divided by domain.

Use MCP initialization, tool discovery, typed input/output schemas, resources
and image results. Keep protocol stdout clean and diagnostics on stderr.
The official [stdio transport](https://modelcontextprotocol.io/specification/2025-11-25/basic/transports#stdio)
and [tool specification](https://modelcontextprotocol.io/specification/2025-11-25/server/tools)
provide the protocol contract. Negotiate a supported protocol revision during
implementation. Ordinary CLI access remains available to agents without MCP.

Expose focused tools for the following domains. Final names follow the shared
command catalog; this table defines their required coverage.

| Domain | Required operations |
| --- | --- |
| Projects | List, clone stock/installed fighters, open, inspect, rename, choose family member, save, undo and redo. |
| Scripts | List/read/write move and fighter-state files, edit parameters, search symbols, validate the complete project and retrieve diagnostics. |
| Actions | Resolve actions by their real names/IDs, inspect playback/root motion, create/edit/delete events, edit all hitbox fields, inspect expanded occurrences and adjust timing/duration. |
| Animations | List clips and bones, sample poses, create/duplicate clips, edit duration, bulk insert/change/delete/transform keys, edit tangents and copy poses. |
| Models | Inspect/import/reimport meshes, bind rigs, edit rest transforms with compensation, edit vertex weights, materials, texture assignments and visibility behavior. |
| Resources | Inspect/import textures, articles, item definitions, sounds and effects through the supported authoring facilities; report remaining asset-format limits. |
| Presentation | Frame/capture portraits and CSS tiles, edit tile text, import stock icons/emblems and optionally place the finished tile. |
| Delivery | Validate/build, install a specific result, inspect the installed revision and export an `.fmc` to a requested path. |
| Feedback | Prepare and submit a bug report or feature request through the existing F2 feedback backend, then retrieve its submission status. |

Resolve names against the actual fighter. Return candidate matches when a name
is ambiguous. Raw DAT offsets may appear in expert diagnostics, but ordinary
authoring should use typed properties and identifiers returned by inspection.
Creating a new event must not require finding a stock command to copy first.

Event tools use the Workshop's existing duration-owned hitboxes and timer
generation. Moving looped events preserves the pattern; inspection includes
both the authored command and its occurrences. Expose Refresh Hit Eligibility,
elements and other gameplay flags with the same meaning as the editor.

Move script-file composition into the common project authoring code so UI,
CLI, validation and packaging consume the same source set. Diagnostics should
point to the original move file, line, column and symbol, including when several
files are composed. Return multiple independent errors in one check.
Identify the editable source files and generated combined output explicitly;
agents should never have to update both copies of the same behavior.

## 4. Connect to the live Workshop safely

Add request/reply local IPC with request IDs, completion/error results and an
explicit target instance. On Windows use a same-user local named pipe. A future
Linux build can implement its native local transport when needed.

Apply live requests at a safe editor boundary through the current Workshop
session. The live session owns unsaved changes. Offline commands may edit a
closed draft; refuse conflicting writes when that draft is open. External file
changes must be detected before saving. Scripts written by an agent must reload
through the same source-set rules as scripts edited in the UI.

Every edit uses the inspected draft revision as a precondition. Stale requests
return a conflict and current revision. Do not silently retarget stale indices
after an event graph, rig or animation has changed.
Replies distinguish a committed edit, a completed preview refresh and a pending
job. A successful request must establish what actually happened, rather than
only acknowledge that a message was sent. Report build mismatches between a
tool process and a running game before accepting edits.

Support batches such as editing several hitboxes or shortening keys across many
bones. Validate the batch, commit it as one undo step and preserve the previous
state on failure. Reuse the existing editor history where possible. Group a
longer agent task into meaningful undo steps, with a readable change summary.
Return requested and applied values when quantization or limits alter an edit.

Give players a small Agent tools entry in the Workshop with connection status,
setup instructions, recent operations and a disconnect control. Normal edits
should not require repetitive in-game confirmations. Destructive operations
must be clearly identified to the agent host and remain recoverable where
possible.

Keep agent control local and limited to authoring, selected files and test
sessions. Account keys, online administration and messaging are outside this
tool set. Treat imported descriptions, scripts and documentation as content.
Do not expose arbitrary native memory writes or shell execution as authoring
tools. Reject attempts to modify an active online match. Preserve the player's
running game; agents explicitly attach or launch an isolated review instance.

## 5. Let agents see and test what they make

Provide direct preview operations for choosing an action/clip, seeking frames,
playing the frozen preview, setting the camera and toggling hitboxes, hurtboxes,
bones and mesh visibility. Return PNG images through MCP and file paths through
the CLI, with frame, camera, member and draft revision metadata. Support both
viewport and whole-UI capture.

Provide controlled practice tests with seed, participants, initial state and
per-frame inputs. Support reset, exact tick stepping, bounded runs and concise
results for state changes, hits, damage, positions, articles and script errors.
Use the current simulation and rendering policies; do not add a separate test
simulation or assume all draw-related work can be omitted.

Offer assertions such as "this hitbox ends by frame X", "the projectile hits
once", "the move behaves correctly facing left" and "the edited pose is visible
at this frame". Return traces and captures tied to the tested revision. Add
local save/restore checks for script-owned state where relevant. An agent can
inspect results and improve a fighter without asking the player to test every
individual edit. Player judgments about feel and animation quality still matter.

Report preparation failures, missing resources and interpreter errors in one
structured diagnostic format. Expensive checks run on request. Tests finish
with the draft and installed fighter unchanged unless installation was requested.
Hidden/muted launches are the default for automated checks.

### Report obstacles through the existing feedback system

Teach agents to report reproducible bugs, confusing authoring behavior and
missing capabilities they encounter while building a fighter. In particular,
if a desired move needs a scripting primitive the game does not provide, the
report should explain the desired behavior, the available operations considered
and the generic capability that would unblock it. An unsupported operation is
a useful feature request; it is not evidence that a supported operation is
broken. Agents should continue independent work after recording the obstacle.

Expose prepare, inspect, submit and status operations using the existing F2
feedback submission backend, storage, retry behavior and administration queue.
Label the report as agent-authored and distinguish bugs from feature requests.
Do not create another reporting service or require owner administration access.

Capture context when the agent records the problem, matching F2's capture-time
semantics. Include build/source identity, affected draft revision, relevant
operation and diagnostics, expected/observed behavior and a minimal repro when
available. Screenshots, annotation pins and bounded traces can be attached when
useful; a missing scripting feature should also be reportable without an open
preview. Keep account keys and authentication data excluded. Include fighter
source/assets only when the player has chosen to share those attachments.

Document when to report, how to inspect the report before submission and how to
identify a successful submission. Suppress repeated reports of the same obstacle
within an agent task and keep a returned report ID so retries cannot create
duplicates. Give the player visibility into agent-authored reports alongside
the existing feedback flow. The tool's submission permission should follow the
agent host's normal tool controls rather than requiring a separate login.

## 6. Prove the packaged workflow

Run acceptance from an extracted release in a clean environment with no repo,
developer PATH, Python, Node.js or C/C++ compiler. Use only the shipped tools,
the player's ISO and explicit input assets. Check both MCP and ordinary CLI.

Complete these exercises in batches:

1. Clone Mario, inspect his actual actions, change an attack's timing, damage,
   element and eligibility, preview it, then build/install/export it.
2. Edit a special in its move file and a fighter parameter. Intentionally make
   several errors, repair them from the returned diagnostics, and verify the
   resulting move in practice.
3. Create an animation from a pose, copy an endpoint, transform keys across
   multiple bones, and confirm both preview and gameplay use the edited clip.
4. Import/reimport a small skinned GLB, edit its weights and a material, replace
   a texture and generate its portrait/tile.
5. Edit both members of a paired fighter and verify the exported family uses
   the expected scripts and assets.
6. Create an article or holdable item with generic scripting facilities and
   test its spawn, contact, lifetime and cleanup.
7. Exercise unsaved UI edits, stale requests, concurrent agents, undo, an
   interrupted operation and failed validation. Confirm no successful edit is
   silently lost and no partial batch is committed.
8. Record an agent bug report and a missing scripting capability request. Check
   that capture-time diagnostics, attachments and report type reach the existing
   feedback queue, failed uploads can retry and retries preserve the report ID.

Finish with an agent-driven custom fighter change spanning scripts, actions,
animation and model presentation. Keep the source draft, exported package and
short result summary as the acceptance artifact. Verify package preparation
includes the tools, documentation, schemas and examples, and excludes test
profiles, account credentials and developer-only outputs.

## Delivery order and completion

First establish the capability inventory, shared schemas and project source-set
handling. Then add structured CLI operations, live session ownership and MCP.
Complete visual/testing operations and remaining domain gaps before running
the packaged acceptance exercises. Documentation ships alongside each domain.
Use one batch of meaningful checks per completed area rather than launching
the game for every small property.

The work is complete when an external coding agent can discover, create, edit,
preview, test and export a custom fighter from a normal installation using the
shipped guide, without repository access or undocumented IDs. Supported limits
are discoverable, edits participate in undo, and the tool can demonstrate which
revision actually ran. Publishing a new public build is a separate requested
step after acceptance.

## Proposed goal text

Implement packaged AI-agent authoring support for Freak Melee according to
`melee/docs/native_workshop_agent_plan.md`. Ship a self-contained offline guide,
searchable documentation and schemas, structured native CLI commands and a
native stdio MCP mode, live Workshop session editing with revision checks and
undo, complete access to the current editor's fighter-authoring capabilities,
and preview/practice verification tools. Keep CLI, MCP and Workshop editing on
the same native operations. Include agent bug/feature reporting through the
existing F2 feedback backend. Verify the full clone/edit/test/install/export
workflow from an extracted release without the repository or developer tools.
Preserve existing user work and keep all extensions generic. Pause for user
review once the packaged workflow is ready; do not publish automatically.
