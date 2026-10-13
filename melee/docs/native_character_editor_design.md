# Character authoring: design review

> Approved authoring design with historical implementation context. Use [current Workshop status](native_character_workshop_progress.md) and [current handoff](native_character_editor_handoff.md) for what exists and what remains; the early backend proof order is not a new task list.

Status: approved design, reviewed against the current source on 2026-09-23.
UI direction finalized 2026-09-24 in `native_character_workshop_ui.md`, including
three full work modes, shared animation graphs and first-pass pose copying.
This document defines future work, not implemented editor capabilities.
The 2026-10-07 decision in `native_interpreter_transition.md` supersedes the
generated-C backend: direct script validation, interpreted bytecode and explicit
native savestates are the production direction. The approved UI remains intact.
The parent `PROJECT_PLAN.md` supplies the wider port acceptance criteria.
`native_character_editor_handoff.md` records the implementation sequence,
source ownership, integration points and acceptance gates for this design.

## Author experience

Clone a fighter, give the clone a name, change a move, test it immediately,
and install the result as its own selectable character. Routine move authoring
must not require knowledge of C, pointers, binary offsets, or the engine's
numeric action identifiers. The user chose readable scripts first, with
visual tools for timing, hitboxes, and animation. C/C++ remains available for
advanced behavior.

Keep the complete workspace inside Freak Melee. Enter it from Collection;
replace the normal sidebars with authoring controls and shrink the live 4:3
practice viewport. Editing suspends that installation's room readiness. A room
may remain connected, but it cannot pull the author into a match or publish
unbuilt draft content. Chat is collapsible while editing.

The central workflow is:

1. Clone a supported stock or installed fighter into an editable local project.
2. Choose a named move and one of its states; see inherited behavior explicitly.
3. Edit a script, an event on its timeline, a hitbox, or an animation key.
4. Apply and restart the test from a reproducible setup. A compile error keeps
   the last working preview available and selects the offending source line.
5. Save the draft freely. Build and install only when the author wants a new
   playable package. Installation selects the new content hash for that local
   character entry; matches and replays retain their already-selected hashes.

Saving a draft, applying a preview, and installing a package are different
actions. Do not create a new selectable character for every autosave.
Undo/redo spans script and visual edits; autosave/crash recovery preserves the
draft outside replaceable game builds. Show changed-from-base values and allow
reverting an individual field, event, or move.

Packages contain the editable script/definition sources and their required
assets as well as validated runtime output. Installing a friend's authored
fighter should leave it cloneable without requiring their private project
folder. Keep package identity, local project identity, and transient runtime
fighter IDs distinct. Resolve dependencies by exact content identity for a
match. A selectable fighter may need companion forms or multiple articles;
do not assume four fighter slots imply only four internal definitions. Native
extensions may require their matching source/toolchain to edit; show that
requirement instead of presenting opaque machine code as editable behavior.

After the base fighter clone backend and collaborative interface design, add
copying of whole supported moves between fighters, including their states,
subactions, animations, behavior and assets. Move transfer is not a gate for
the initial clone/edit editor workflow.
Resolve the dependency graph and destination bone/attribute bindings explicitly;
copying a native callback pointer alone is not a portable move. Imports are
undoable and independent of later edits to the source project. Begin clone
coverage with Mario, then expand toward Captain Falcon, Fox, Jigglypuff and
other bases. Add readable stock behavior templates gradually; broad automatic
C-to-script conversion is not a prerequisite. See the handoff for acceptance.

## What the live source changes about the plan

| Source finding | Design consequence |
| --- | --- |
| `src/melee/ft/native_fighter_runtime.c` resolves only the compiled Rook definition and binds callback/data tables. | This is a bring-up implementation, not a complete general authoring or native-module loader. |
| `ftMario/ftmariospecialn.c` chooses Mario fireballs versus Dr. Mario pills by `fp->kind`. `ftCaptain/ftcaptainspecials.c` also branches on fighter kind. | Copying tables to a new kind does not preserve stock behavior. Clone support needs an explicit audit of family-specific behavior, attributes, articles, and callbacks. |
| `ftaction.c` executes existing subaction commands, including hitboxes and command variables. | Preserve those commands and their exact ordering. A new editor must not silently flatten their control flow into a different timeline. |
| Mario's neutral special consumes an animation flag, spawns at a hand bone, checks an interrupt flag, and handles ground/air transitions. | A timed-event editor is useful, but it also needs state transitions and named engine operations. |
| Fox's up special uses charge/launch/end states, delayed gravity, directional movement and model rotation. Captain's side special installs detection and damage/death cleanup callbacks. | Authoring needs conditions, input, event callbacks, explicit physics policies, and resource lifetime/interrupt cleanup. |
| Mewtwo's neutral special retains charge, owns a held article, releases it and applies recoil. Rook retains charges until landing/death. | Persistent fighter variables differ from state-local variables. Projectile ownership and lifetime need first-class support. |
| `fighter.c` schedules animation, interrupts, physics, collision, accessories, and hitlag through distinct phases. | An indiscriminate once-per-frame script hook would change behavior. Script events must execute at their documented engine phase. |

Stock behavior remains implemented by the existing decomp code. We do not
automatically translate arbitrary native callbacks into editable scripts. A
clone may initially retain a named native behavior with exposed parameters;
the editor must label that boundary instead of pretending the displayed
timeline describes its whole implementation. Replacing that behavior with an
authored move is explicit.

For the first **fully cloneable Mario** template, that boundary is stricter:
the draft opened in Scripting must reproduce Fireball, Cape, Super Jump Punch,
and Tornado in editable script. `stock.enter()` and copied Mario motion-state
callbacks are explicit untranslated placeholders, not part of that completed
template. General engine operations (state changes, ordinary fighter physics,
collision queries, article creation, effects) remain callable from script; the
script decides when to call them and owns move-specific state, transitions, and
cleanup. Keep the original Mario callbacks as comparison oracles while
developing and validating the scripts. Do not ship a misleading four-move
template while even one move still delegates its behavior.

The compiler now generates authored entry, animation, interrupt, physics,
collision and cue callbacks for all four Mario specials in
`examples/mario_full.fms`. The source calls general engine operations and
article constructors; it contains no `stock.enter()` or original Mario
special-handler calls. The earlier Fireball-only trial exposed a projectile
origin error: imported animation flags still identified Mario as the clip
owner, incorrectly sending an identical skeleton through cross-fighter
retargeting. Rebasing only self-owned animation tags to the clone's runtime
kind restored the stock joint pose and projectile origin. Grounded Cape,
aerial Cape, Up-special steering and grounded Tornado recordings now each
match all 3,669 traced stock P2 fighter-state lines, and two independent
installed Mario clones have completed a match together. These recordings do
not cover every move interruption or projectile interaction. Avoid one-off
functions that are merely renamed Mario special handlers.

The compiler binds authored ground/air entry and eight state phases to any
special slot. Cape's script now owns its reflector, attached article,
ground/air physics, boost, effects and item-to-owner reset/removal callbacks.
Direct module tests cover its article lifecycle. Live Fireball reflection and
an early Cape interruption now match stock Mario's tracked fighter state;
later interruptions after the article appears remain to be exercised.

Super Jump Punch's authored air entry clears vertical speed and scales
horizontal speed by its editable DAT attribute. Its script also handles
input steering, reverse cue, physics, collision and freefall landing. A
targeted Up-special replay matched stock fighter state at every sampled frame;
other trajectories and interruption timings still need coverage.

| Mario move | Script-owned behavior required beyond entry | Engine operations and dependencies to expose |
| --- | --- | --- |
| Fireball (`ftmariospecialn.c`) | Animation end/IASA, ground/air physics and transitions, consume throw flag once | Query hand bone, create the fighter's fireball article, spawn effect; preserve the same animation frame on ground/air transition. |
| Cape (`ftmariospecials.c`) | Reflect-window command flags, air boost, article lifetime, damage/death/exit and hitlag cleanup | Create attached cape article, maintain its owner handle, enable/disable reflector, pause/resume/destroy article, stage effects and ground/air physics. The item dispatches reset/removal checks to the scripted owner's module. |
| Super Jump Punch (`ftmariospecialhi.c`) | Stick-angle and turnaround input, launch and gravity phases, collision landing/freefall | Read move attributes and input, turn facing/part rotation, use common air/ground physics, freefall and landing-lag transitions. |
| Tornado (`ftmariospeciallw.c`) | Charge persistence, button-tap ascent, command-driven acceleration, collision/rotation, hitlag/damage cleanup | Tracked persistent and move-local variables, common physics and special collision box, bone rotation, effect ownership, ground/air transitions. |

All mutable script values and article handles belong to rollback-tracked
fighter/item state. Validate each move's entry, interruption, ground/air
transition, hitlag, and completion against the original, including simultaneous
clones and forced rollback. A successful compile or one visible move does not
establish gameplay parity.

Separate a package's selectable identity from the behavior family that an
inherited stock move actually needs. Audit and adjust the relevant call sites;
do not temporarily spoof `fp->kind` or add a universal compatibility wrapper.
Stock ancestry is retained only as authoring provenance after a clone is built.
The package owns its editable data, states, callbacks and compiled behavior;
shared immutable engine helpers are permitted, but runtime dispatch must not
rely on a permanent `mario_clone` identity or silently follow stock Mario's
editable definitions. The initial compiled-in Mario binding is a temporary
runtime proof, not the intended independent-character package design.
Do not mark every stock fighter cloneable until its untouched clone has passed
runtime acceptance. Transforming fighters, partners, copy abilities and unusual
articles need their own coverage.

## Small move language

Adopt a small, readable, typed language scoped to Melee behavior. Syntax is
provisional; the first prototype should implement a few real moves before
fixing it. Familiar indentation, assignments, expressions, conditions and
named events are sufficient. We are not embedding GDScript or recreating a
general-purpose language.

Existing subaction scripts and native move callbacks are distinct execution
domains. HSDRawViewer already edits the former; the primary new language need
is the latter. Retain an Actions/Subactions view for original command streams
and a Behavior view for authored state callbacks, with links between named
command flags/cues and their consumers. Ordinary timing/hitbox edits must work
before a stock move is rewritten in the new language. Where authored timed
operations can lower faithfully to existing subaction commands, use the
existing interpreter; do not create a second hitbox or timer implementation
with subtly different semantics. Assign each event a single execution owner
and engine phase. The language prototype must validate that division.

First-class concepts:

- Named moves/states, clips, bones, hitboxes, articles, effects and sounds.
- Numbers, booleans, vectors, named choices, bounded timers/counters, and
  engine-managed object handles. No author-visible addresses or pointers.
- State-local values and fighter-lifetime values, with explicit reset rules
  for transition, landing, death and match restart.
- Entry, animation cues, permitted simulation updates, input/cancel checks,
  hit-confirm, land/leave-ground, damage interruption, exit and cleanup events.
- Velocity/acceleration, facing-relative and world coordinates, ground/air
  physics policies, transitions, article operations and bone-local transforms.
- Access to the engine RNG through named operations, never wall-clock time or
  untracked random state.

The planned compilation path is readable script -> typed, validated behavior
representation -> interpreted bytecode. The language replaces
handwritten special-behavior C; it does not replace existing subaction commands.
Resolve interpreted callback references for the existing engine phases and
provide explicit fixed-layout state in rollback-tracked fighter/item storage.
Parse and bind at load/apply transitions. The interpreter executes fixed code
references with fixed storage during simulation; general object allocation,
name lookup and coroutine stack capture do not belong in the tick. Handwritten
C/C++ extensions remain separate explicit advanced sources.

Ordinary authors need no C development environment. Applying a behavior edit
validates and loads only the affected interpreted program at practice reset;
data-only edits need not rebuild behavior. Preserve source mappings for compile
errors and interpreted runtime diagnostics. Validate types and references before
loading a program so routine mistakes receive useful script-level explanations.
Measure apply/restart latency as well as representative four-fighter execution
and rollback catch-up. Define and test the client's numeric/execution contract;
interpreted execution alone does not establish determinism.

Bound execution and object counts for predictable frame time. Prefer named
states and event handlers over suspended scripts with `wait` and hidden stacks.
Errors should identify the move, state and source line. Runtime diagnostics
must stop a broken test coherently; online peers cannot silently skip different
operations.

C/C++ extensions can implement unusual operations or full moves. Generated and
handwritten code use the same exact-build character module loading and
packaging contract. This is a dependency of the first script-authoring slice,
not only an eventual advanced feature: authors need to apply compiled moves
without rebuilding the client, and share them without requiring recipients to
compile. The current compiled-in Rook registry does not meet that requirement.
Design module load/unload, engine access, registered state, and callback
lifetimes explicitly; do not introduce an ABI compatibility promise. A
module stays loaded for the lifetime of all matches, snapshots and preview
objects that reference it. Its mutable gameplay state belongs in registered
snapshot storage, not hidden DLL globals.

## One source of truth, several useful views

Scripts own control flow. Typed definitions own hitbox, attribute, article and
animation data. The timeline and inspectors edit those same documents.

An editable event such as a hitbox window has a stable source identity.
Dragging its start frame updates the corresponding literal source value;
editing its radius updates its definition. A computed time or dynamically
chosen branch is shown from the current test trace with a link to its source,
not converted into an editable fixed frame behind the author's back. Preserve
comments and formatting outside the edited source span. Never maintain two
independent versions of a move in script and visual form.

The workspace uses full Action Editor, Scripting and Animation modes with
preserved document context, as specified in `native_character_workshop_ui.md`.
Collection owns fighter selection and cloning outside this workspace. Each mode
allocates space to its primary task and keeps its inspector contextual. The
practice preview is resizable and 4:3; animation supports a larger rig viewport
and shared graph for all enabled channels of the selected bones.

Completion and an Insert command menu offer the actual available bones,
states, articles and engine actions. Selecting an error or an observed event
goes to its source. Small ready-to-edit examples cover a dash, projectile,
charge move, hit-confirm follow-up, limited-use move and landing reset. A state
diagram can be a navigation aid; a mandatory state-machine node editor is not
the default. This is separate from the primary animation curve graph.

## Timing, physics and interruption contracts

Show animation frame separately from simulation tick. Hitlag, animation speed,
loops and ground/air transitions make them different. Define the semantics of
frame-zero events, crossed frame thresholds, loop repeats, animation restarts,
and preserved-animation transitions before implementing the timeline. An event
must not disappear when playback skips past its timestamp, or fire repeatedly
because hitlag holds the animation there. Retail-script import retains retail
semantics, even when the author-friendly event vocabulary differs.

Cancel/IASA controls need allowed actions, conditions and priority. A single
"IASA frame" does not describe every move. Ground/air handoff must explicitly
preserve or reset animation progress, event progress, hitboxes and velocity.

Velocity controls identify self velocity versus knockback, grounded speed
versus airborne components, and the coordinate space. Position changes need
defined stage-collision handling. Bone controls identify local versus world
space and whether an offset is applied after animation. Do not make raw field
writes the default author API.

Hitbox tools show bone attachment, offsets, radius, damage, angle, knockback,
ground/air eligibility and hit group. Preserve Melee's hit-memory/multihit
semantics; recreating a hitbox is not automatically a new permitted hit.
Cleanup includes effects, loops, temporary transforms and owned articles when
damaged, grabbed, KO'd or otherwise interrupted. Articles can outlive a move
when declared; arbitrary bone poses and temporary flags should not leak into
the next state.

## Animation and Blender

The requested endpoint includes both in-game bone posing/keyframe editing and
a Blender import/export workflow. In-game tools cover selecting bones,
transform gizmos, local transforms, key insertion/removal, timing/trim/loop,
shared multi-bone/channel curve editing and clip-to-clip pose copying at the
current frame or either endpoint. The approved UI specification defines graph
operations, native interpolation constraints and pose-copy acceptance. Blender
handles substantial animation, rigging,
constraints, and mesh/skinning work. A full DCC suite is not required in-game.

Existing groundwork:

- `src/melee/lb/lbanim_dat.c`, `ft/ftanimation_bank.c` and
  `tests/native/figatree_test.c` decode the native port's stock figatree data.
- The supplied HSDLib includes `HSD_FigaTree`, FOBJ key readers/writers,
  `FigatreeContextMenu.cs` key export and `Tools/Animation/AnimationBakery.cs`.
- Melee already maps cross-fighter animation joints through shared body-part
  identities in `ftPartsRemap` (`ftparts_table.c`); `ftanim_tracks.c` uses that
  mapping when animation source and fighter kinds differ. Reuse this as the
  starting mapping for move imports, retaining its per-bone evaluation rules.
  Extra bones, attachment offsets and different body proportions still require
  validation; this is not proof that arbitrary full moves transfer correctly.
- The [community melee-blender pipeline](https://github.com/visgotti/melee-blender)
  documents extracting models/animations through HSDLib and importing them
  into Blender. It is evidence of useful prior work, not proof of a faithful
  editable round trip back to this client.

Use a shared explicit animation contract: stable joint identity/order, bind
pose, local transform basis, root-motion channels, clip range/rate/loop data,
track types, curves/tangents and any supported non-transform tracks. Raw DAT
import/export remains available. The live runtime can keep using its existing
FigaTree/FOBJ evaluation; editable projects do not need to keep rewriting whole
GameCube archives after every key edit.

For untouched imports, preserve original encoded tracks and metadata. For
edited curves, re-encode and validate against the native evaluator. Do not
assume Blender Bezier tangents, Euler winding, bone rest matrices or stepped
curves map directly onto FOBJ interpolation. Preserve what can be expressed
exactly; explicitly bake unsupported constraints into the game skeleton and
report the selected sampling/error tolerance. No silent "lossless" promise.
Both the in-game editor and Blender tooling target this contract; packaging
resolves it into the native game's existing animation structures.

Blender acceptance compares joint transforms, root motion and hitbox attachment
at integer and fractional frames after import/export/reimport. Cover loop
boundaries, negative/long rotations, nonuniform scale and special joint flags.
An animation that merely looks plausible in Blender is insufficient. Measure
errors in engine coordinates and show a comparison in the live practice room.
Competing edits from Blender and the in-game draft require an explicit reload
or keep-local choice, never last-save-wins data loss.

## HSDLib and HSDRawViewer reuse

A deeper source review establishes that these projects are primary groundwork
for authoring, beyond their figatree decoder. The supplied checkout has:

| Existing implementation | Evidence in `HSDLib/` | Intended use |
| --- | --- | --- |
| DAT graph editing and serialization, including relocated references and symbol tables | `HSDRaw/HSDRawFile.cs`, especially `Save(Stream, ...)` | Use the library for offline conversion/verification where practical; validate generated DATs against native imports. Preserve graph sharing and references, not merely field values. |
| Subaction opcode/parameter names, enums, signedness, bit widths and descriptions | `HSDRawViewer/Scripts/command_fighter.yml`, `Tools/SubactionManager.cs` | Seed the native inspector metadata and command vocabulary from this knowledge, checked against the actual decomp handlers. Keep runtime lookup tables fixed and precompiled. |
| Action lists, shared subroutines, reference navigation, event insertion/removal/reordering, property edits, copy/paste and undo/redo | `GUI/Plugins/ScriptEditor/ScriptEditorActionList.cs`, `ScriptEditorSubactionEditor.cs` | Treat this as a concrete baseline for editing original actions, not a new feature to invent from scratch. |
| Text compilation/decompilation of existing subaction commands and reference linking | `HSDRaw/Tools/Melee/ActionCompiler.cs`, `ActionDecompiler.cs` | Reuse the command-level format knowledge and round-trip fixtures. This compiler does not replace native special-move C callbacks. |
| Viewport picking of a hitbox selects its source event; translation widgets update its stored offsets | `GUI/Plugins/ScriptEditor/ScriptEditor.cs`, `SubactionProcessor.cs` | Preserve the direct connection between what the author touches and the command being edited. |
| Timeline markers for active hitboxes, interrupt flags, command-variable writes and frame-speed changes | `ScriptEditor.cs::UpdateFrameTips` | Baseline for useful timeline feedback, with actual engine traces supplying gameplay truth in our preview. |
| Joint/curve editing, compression, trimming, retiming and figatree conversion | `GUI/Extra/PopoutJointAnimationEditor.cs`, `Rendering/Animation/JointAnimManager.cs`, `Tools/Animation/AnimationBakery.cs` | Reuse proven format algorithms and workflow concepts when implementing in-game tools and Blender interchange. |
| Saving subaction changes into their owning structures and rebuilding animation banks/offsets | `ScriptEditor.cs`, `HSDRaw/Tools/Melee/FighterAJManager.cs` | Test the complete edit/save/reopen path, including changes spanning both fighter DAT and animation bank. |

HSDRaw is a C# library; HSDRawViewer is a Windows Forms/OpenTK application.
The native in-game editor uses the client's existing UI/rendering system.
Direct reuse of offline library functionality and format metadata is different
from embedding that desktop GUI or importing its rendering simulation into
the native game's frame loop. Keep the HSDLib checkout intact as a reference;
make specific reuse/porting decisions per subsystem with source provenance.

`SubactionProcessor` reconstructs preview state from selected commands at a
requested frame. It is not the complete Melee fighter simulation. Its own
frame markers and hitbox transforms are useful reference behavior, but the
decomp remains authoritative for hitlag, collisions, IASA callback decisions,
bone transforms and gameplay. An `Allow Interrupt` command does not describe
every native interrupt callback. Validate supported command edits against both
the source encoding and the actual native engine before expanding coverage.

These capabilities were verified by reading the supplied source, not by a
hands-on acceptance run of HSDRawViewer. Before implementing each equivalent
interaction, inspect that workflow on representative local data and retain
edit/save/reopen fixtures. This review does not establish that every HSDRaw
conversion or preview path is lossless or complete.

## Live preview and debugging

The test runs real Melee simulation with the edited definition. Animation-only
scrubbing is separately labeled and does not claim to simulate collisions,
IASA or projectiles. For a gameplay scrub, restore a known test state and run
forward with recorded inputs; keep the current full-draw simulation behavior
required for parity. Do not resurrect the deferred drawing-disabled fast path.

Expose play/pause, single-tick step, restart, speed, hit/hurtbox and bone
overlays, and a controllable opponent or Sandbag. A controller belongs to the
preview only when the author explicitly enters test mode. Typing in the script
must never make the fighter jump or trigger gameplay shortcuts.

Show the state, animation frame, self velocity, relevant variables and recent
events when requested. Explain failed transitions/cancels by showing the
tested condition. Authors can replay the same input sequence after edits and
compare behavior. Include interruption tests, not just an empty-room move
animation. Hitlag/SDI/DI behavior comes from the real engine.

Preview apply is transactional: compile and validate the candidate, destroy
old test objects/snapshots at a safe boundary, replace the definition, restart
the fixture. Invalid candidates leave the last successful build available.
All mutable script state, event cursors, handles and timers participate in
rollback and deterministic diagnostics. No snapshot retains freed code or data.

## Delivery sequence and evidence

1. **Clone correctness and project lifecycle.** Audit one straightforward stock
   base, preserve its untouched behavior, save/reopen a draft, build/install a
   distinct fighter, and prove its original remains unchanged. Record which
   behavior is inherited and which is authored.
2. **One real vertical slice.** Author a small new move with the proposed
   language, validate bytecode and load its interpreted program; establish safe
   replacement and rollback state ownership. Expose hitbox/timing edits,
   completion, useful errors, undo, and
   apply/restart. Save, reopen and play it in a normal local match. This is the
   point to evaluate syntax with the user before expanding it.
   Use HSDRawViewer's existing action/subaction editing flow as the baseline:
   a stock hitbox/timing edit should already round-trip through original
   commands without requiring conversion of the whole move to authored logic.
3. **Behavior breadth.** Exercise projectile spawn, directional movement,
   charging, hit-confirm branching, interrupt cleanup, persistent charge/reset
   and bone transforms. Start with audited existing article definitions; new
   article behavior needs the same authoring/state/lifetime discipline.
4. **Animation.** Build in-game pose/key tools and the Blender round-trip path
   against the shared animation contract. Validate fractional poses and root
   motion through the game before broad asset support claims.
5. **Authoring and sharing acceptance.** Build a fighter, transfer its package
   to an isolated installation, select it and force rollback correction in
   two local clients. Test four authored fighters and interruption/cleanup.
   Cover interpreted behavior through ordinary packages, and handwritten native
   extensions through their explicit advanced packaging path.

Broaden clone support and templates incrementally. Rook remains a useful
custom-behavior case, but its incomplete neutral special and unusual skeleton
make it a poor sole test of stock clone fidelity. CPU move choices, Kirby copy,
throws/capture remapping, transformations and costume dependencies remain
explicit character features; the editor must show their support status.

Physical multi-PC and cross-network acceptance remains deferred from the
current goal phase. All checks above can be developed on this PC. Successful
local authoring does not by itself establish final release readiness.
