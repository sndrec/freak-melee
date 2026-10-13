# Character workshop: implementation handoff

Status: approved M3 scope; UI baseline finalized with the user, 2026-09-24.
Execution direction revised 2026-10-07: interpreted behavior and explicit native
savestates, as specified in `native_interpreter_transition.md`. Historical
generated-C comparisons remain evidence; ordinary C/DLL authoring is superseded.
This records the accepted plan, not implemented capabilities. Goal execution
and model selection are controlled separately in the app.
Read `native_character_editor_design.md` for the agreed author experience,
source audit, animation requirements and runtime constraints. This document
defines the implementation order and the decisions to prove before broadening
the editor. It does not claim these systems already exist.
`native_character_script_examples.md` provides provisional author-facing
syntax sketches for projectile release and persistent charge/ownership.
`native_character_workshop_ui.md` records the approved three-mode mockup and
pose-copy workflow. Consult `native_character_workshop_progress.md` and current
code before using the historical backend proof sequence below as a task list.

The 2026-09-26 Model workspace design is in
`native_model_workspace_design.md` and the extended interactive study. The user
endorsed the layout with explicit Edit/Pose/Animation interaction modes. Retain
the cloned fighter's compatible rig; arbitrary imported armature conversion
is deferred. This addition does not reopen the layout of the other work modes.

## Agreed boundaries

- In-client authoring with a smaller real practice viewport; readable behavior
  scripts, original subaction editing, and in-game pose/key editing. Blender
  interchange is part of the intended animation workflow.
- Scripts replace handwritten special-behavior C. Validate script directly and
  execute its typed bytecode in the native interpreter. Existing subactions retain their engine
  semantics and interpreter. Do not duplicate their timing/hitbox machinery.
- Ordinary packages share editable scripts and assets, with validated bytecode
  built locally by the client. Handwritten native extensions are an explicit
  advanced path; ordinary authors and recipients need no C compiler or DLL.
- Keep stock native behavior until explicitly replaced. Never imply arbitrary
  C has been converted into editable script.
- Draft save, preview apply, and package installation are separate operations.
  All simulation edits apply at a practice reset; no mid-match hot replacement.
- Use local acceptance, including isolated stores and two local clients.
  Physical multi-PC and cross-network testing remain outside this goal phase.
- Keep complete move copying on the roadmap, after the base fighter clone
  backend and collaborative interface design. It is not a gate for beginning
  the editor UI or shipping its initial clone/edit workflow. Start clone
  coverage with Mario and expand script templates and supported bases gradually
  toward Captain Falcon, Fox, Jigglypuff and others.

## Authoring model to establish first

A fighter project owns identity/presentation, attributes, skeleton and costumes,
move definitions, animations, original command streams, authored behavior,
articles, effects/sounds and declared dependencies. A move is an author-facing
group of states, not an extra runtime state machine layered over Melee.

For example, a charge special groups ground/air charge, release and recovery
states. Each state identifies its clip, subactions, callback behavior and
transitions. Repeated links refer to one resource, rather than making a hidden
copy for every view. Named stable authoring IDs resolve to native indices at
build/load; authors do not maintain numeric state IDs or offsets.

Use full Action Editor, Scripting and Animation work modes, as specified in
`native_character_workshop_ui.md`. Collection owns fighter selection/cloning;
the workshop edits one project. Preserve state/clip/frame context and document
undo across modes. Fighter-wide attributes, costumes, bones and articles are
separate resources. Raw IDs/encoding belong in an advanced inspector.

The three editable sources have explicit ownership:

| Source | Editable facts | Views that can change it |
| --- | --- | --- |
| Original command stream | Hitbox commands, timing, flags, subroutines | Subactions list, supported timeline handles and hitbox inspector |
| Behavior script | Conditions, physics, transitions, article ownership and callback logic | Script editor; source-linked parameter controls only when unambiguous |
| Animation tracks | Bone channels, keys, curves and clip properties | Pose/curve tools and Blender interchange |

A timeline is a view across those sources. It must label where each event
comes from. Subroutine control flow and runtime-dependent events cannot be
silently flattened into draggable fixed timestamps. Explain disabled edits
and navigate to their actual owner.

Shared resources show their users. Offer an explicit make-local copy when an
author wants to alter only one move; remap the relevant references rather than
accidentally changing every consumer. Preserve untouched binary tracks and
unknown supported archive fields, and report unsupported structures without
discarding them on save.

Clones use a pinned base, not a live link to another editable project. Editing
or installing a newer parent must not silently change the clone. Immutable
assets may be deduplicated internally; the installed package must resolve its
full dependency graph without relying on the author's private project folder.
The base is editor-time provenance, not a runtime fighter class or behavior
selector. A built fighter owns its selectable identity, attributes, state and
move definitions, asset references, callback bindings and interpreted behavior.
It may call immutable shared engine functions; editing or replacing the stock
fighter must not change the built fighter. The current `mario_clone` code key
and `source_kind` callback inheritance are bring-up scaffolding, not the final
package contract or acceptance of independent cloning. Replace that binding
with package-owned definitions as the interpreted-program pipeline lands.
Content hashes identify built artifacts, not a promised format migration or
ABI compatibility scheme. Updates to an installed fighter retain its local
collection identity while active matches/replays pin their original build.

## Copying moves and mapping skeletons

Copy a move and its referenced states, subactions/subroutines, animations,
authored behavior, attributes, articles, effects and sounds as one operation.
Show a dependency summary, resolve naming/ID conflicts and map references before
committing the import. Do not overwrite another move implicitly. The operation
is undoable and produces editable local resources; immutable payloads may still
be deduplicated. Copying between drafts must not create a live link between them.

The game already supplies useful cross-character animation machinery:
`ftPartsRemap` in `src/melee/ft/ftparts_table.c` maps a source joint through
`joint_to_part` and the destination's `part_to_joint`. The differing-kind path
in `src/melee/ft/ftanim_tracks.c` applies animation tracks through that mapping,
with per-bone flags selecting normal or filtered track attachment. This is
source-verified machinery, not yet an acceptance test for arbitrary move import.

Seed mappings from those shared part identities. Preserve the existing
cross-character evaluation rules when applicable; inspect their track filtering
before baking a remapped clip. Show unmapped extra bones, mismatched hierarchies
and attachment points for explicit correction. Preview the destination pose,
root motion and hitboxes, including fractional frames and animation loops.
Do not automatically scale hitbox radii, movement or knockback to body size;
those are explicit author choices. Bone mapping does not prove that a move's
native callback attributes, state IDs, models or owned articles are portable.

Authored scripts declare their dependencies. A stock native behavior requires
an audited reusable definition and explicit parameter/state bindings before
cross-fighter use; never copy a callback pointer and reinterpret another
fighter's attribute/state layout. Unsupported behavior remains clearly marked
and cannot be installed as a supposedly complete working move. Provide partial
asset import only as an explicit choice. Begin acceptance with two Mario-based
projects, then a second supported skeleton to exercise the actual remapping.

## Engine and interpreter proof

Inspect and extend the actual integration points:

- `src/melee/ft/native_fighter_runtime.{c,h}` registers owned fighter definitions
  and currently loads generated modules. Migrate that registry and its callers
  to interpreted code references without creating a parallel fighter registry.
- `src/melee/gm/native_content_store.{c,h}` owns installed fighter content.
  Migrate ordinary behavior packages to authored sources, interpreted programs
  and resolved assets;
  update callers directly rather than keeping obsolete formats alive.
- `src/melee/ui/native_shell_collection.{cpp,h}` is the entry point for the
  new workspace. Author RmlUi markup/styles in dedicated asset files using
  the existing UI system; keep document, compiler, preview and UI code separate.

Prove these contracts with a complete interpreted fighter:

1. **Code lifetime.** Validate/load, instantiate, destroy test objects and rollback
   history, replace the program, and run again. Release every code reference
   before retirement. A failed compile keeps the working preview. A failed runtime
   initialization must support returning to that prior build, not leave a
   partially rebound fighter table.
2. **State ownership.** Generate fixed-layout per-instance state with declared
   resets. Two instances of the same fighter must not share variables. Track
   fighter, move/state and article lifetimes separately. Do not place mutable
   gameplay state in interpreter globals or overlap existing common fighter scratch
   storage. Include state, handles, allocator bookkeeping and registrations
   needed to reproduce article creation/destruction in rollback.
3. **Engine scheduling.** Trace a representative native move's animation,
   subactions, IASA, physics, collision and accessory callbacks. Specify cue
   consumption, transitions, ground/air handoff, hitlag and cleanup at these
   phases. Do not replace the scheduler with a generic script tick.
4. **Validation and distribution.** Define numeric and execution semantics and
   typed generic engine bindings explicitly. Cache validated bytecode by source,
   dependencies and binding identity. Share editable source and assets; clients
   build/load programs locally and agree on execution/content before playing.
   Measure cold and repeated edit/apply times without a C toolchain.
5. **Diagnostics.** Map errors to scripts and named states. Type-check units,
   bone/article references and callback availability before loading bytecode.
   Map interpreted instructions back to their editable source.
   Define bounded loops/operations and numeric failure diagnostics; do not
   silently change behavior by clamping arbitrary values to hide errors.

Write the language examples before implementing a large grammar. Cover a
ground/air projectile, a directional launch, persistent charge with reset,
and damage interruption that releases an owned object. Define expression
types, coordinate spaces, transition semantics, variable lifetime and cue
consumption from those examples. Syntax remains provisional until the first
playable example is evaluated with the user.

Readable stock templates are expanded gradually. A mechanical C-to-script
converter is optional research, not an acceptance dependency: arbitrary C has
pointer aliasing, unions, raw callbacks and implicit state lifetimes that the
author language intentionally does not expose. If conversion is pursued, start
with a recognized subset and report unsupported constructs; do not conceal
untranslated C behind an apparently editable script. Compare each converted
template against the original callback behavior using identical test inputs.

Custom articles eventually need their own authored behavior, resources and
ownership. The first projectile may reuse an audited stock article, but that
must be labeled reuse; it does not establish arbitrary projectile authoring.

## First complete workflow and acceptance

Use Mario as the user-selected initial base because his neutral special exposes
the key interactions clearly. This is an implementation fixture, not a limit
on the roster. Audit the Mario/Dr. Mario kind branches before enabling a clone.
Do not begin with a transforming fighter or use Rook as the only fidelity case.

| Step | Evidence required before expanding scope |
| --- | --- |
| Untouched clone | Original and clone retain the audited behavior and independent data; clearly state which moves/features have actually been checked. |
| Simple data edit | Change a normal's hitbox and timing, save/reopen, install and play. Only the clone changes; existing subactions still execute. |
| Scripted special | Replace one special with interpreted callbacks, consume a subaction cue, spawn an article, handle ground/air transitions and clean up interruption. |
| Edit/test loop | Apply a valid change, try an invalid script, recover, undo and restart a reproducible preview without losing the working draft. |
| Local distribution | Open the installed package in an isolated store without the source project's files; play its interpreted behavior and clone its included source. |
| Later move copying | After the base clone workflow and interface are working, copy a complete supported move between projects, resolve its dependencies and prove source independence; exercise a second supported skeleton before claiming cross-body transfer. |
| Rollback and lifetime | Correct inputs across state transitions/article creation and destruction; compare state and event effects. Repeat with two instances of the authored fighter, then four fighters. |
| Animation edit | Modify a pose/key, reopen and compare integer/fractional transforms through the native evaluator; preserve untouched tracks. |
| Pose reuse | Copy an evaluated clip/frame pose into another clip's start/end/current frame; verify selected-bone isolation, explicit root inclusion, undo and save/reopen. |

The test setup records character builds, stage, initial seed, positions, damage,
opponent settings and controller input sequence. Provide explicit live-control
and recorded-test modes so typing never drives the fighter. Test hitlag,
landing/leaving ground, damage, grabs, KO and restart where relevant. A scrubbed
pose is not evidence of correct gameplay; gameplay scrubbing restores and
resimulates the recorded fixture using the established rendering path.

Each stock-to-script translation must be checked against the original callback
source and replayed beside the unmodified stock fighter with the same seed and
inputs. Compare both actors' motion, physics, poses, active hitboxes and
hurtboxes, damage and action transitions at each sampled frame; include hit,
miss, ground, air, landing and interruption paths that apply to the move.
Inspect article state, ownership and visible material/texture selection for
article-producing moves. A trace match
on one path is evidence for that path only, not blanket 1:1 acceptance. Record
uncovered branches and the first differing frame when a comparison fails.
For an article-producing replay, pass `--expect-item-kind` to
`tools/compare_fighter_parity.py` so a path that never spawned the article
cannot pass silently. The comparator requires current RNG and item-visual
traces and reports the earliest differing sample across trace categories.

One passing neutral-special example does not mean Mario is fully cloneable.
Before using that label, cover his complete move set, normal/common behavior,
grabs and throws, articles, costumes, CPU use, death/respawn and result paths,
and the declared interactions with other fighters. Keep that acceptance matrix
visible and extend it for each new base. Stock template coverage is a separate
measure from the ability to clone inherited native behavior faithfully.

## Approved workspace interaction baseline

The user approved the three-mode mockup in `native_character_workshop_ui.md`.
Implement and validate these tasks using real Mario data: find a move, select
its visible hitbox and edit damage, identify the
subaction that created it, follow a cue to its behavior handler, fix a script
error, enlarge the viewport to pose a bone, copy a pose between clip endpoints,
and apply/restart the test. Exercise simultaneous hitboxes and shared curves
for multiple selected bones, not only single-event/single-channel examples.

Keep the practice viewport resizable, the selected-object inspector contextual,
and play/step/reset available without crowding the script. Show current state,
animation frame and simulation tick distinctly. Preserve selection and undo
across modes. The study is the approved UI direction, not runtime proof.

## Remaining design work during the first slice

Product and layout direction are settled. Review actual Mario scripts and
functional interactions with the user during implementation. Resolve any
remaining engine symbol exposure, module state registration, source maps,
archive preservation and timing semantics through the proof steps above.
Do not ask the user to choose internal field layouts or compiler plumbing.

## Handoff boundaries

The user approved incorporating this work into the goal and handing
implementation back to Sol. When execution resumes, reconcile existing backend
evidence in `native_character_workshop_progress.md` with current code, then
connect the agreed UI to those capabilities and complete outstanding gaps.
Do not restart the historical Mario/backend bring-up sequence. Whole-move transfer follows the base
clone workflow and interface design. No further architecture approval is
required. Finish one acceptance step at a time, commit coherent work locally,
and record tests and unresolved fidelity limits.
No upstream push. Keep engine/compiler code, project editing, animation,
preview, packaging and UI separated by responsibility; avoid one giant editor
file or forwarding compatibility layers.

Defer final syntax polish, broad stock-template coverage and a full DCC feature
set until the complete clone/edit/test/save/install path works. Do not defer
state ownership, source ownership or interpreted-program lifetime: those determine
whether later work can be correct.
