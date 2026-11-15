# Workshop implementation handoff

Updated: 2026-10-13. The backend and four-workspace interface already exist.
Do not repeat the initial Mario bring-up or generated-C architecture work.
Read [current capabilities](native_character_workshop_progress.md) and the
[approved UI](native_character_workshop_ui.md).

## Authoring contract

A project owns its identity, facts, meshes/materials, skeleton, costumes,
animations/demo clips, original subactions, behavior, tuning, CPU data and
entity/resources. A move groups its existing states; do not invent a second
runtime state machine for editor convenience. Clone provenance must not become
a runtime dependency on the donor's editable definitions.

Keep the Workshop inside the game. Collection selects/creates a project;
Action, Scripting, Animation and Model are distinct full work modes. Preserve
state/clip/frame/selection context and undo across modes. Follow the approved
mockup; explain a required deviation before implementing it.

Subactions remain original command streams. Scripts own fighter-specific
decisions and may call generic engine primitives, never stock special handlers.
Typed scripts validate/lower to interpreted bytecode. Owned variables and named
parameters remain editable and rollback-tracked. Ordinary authors/recipients
need no C compiler or behavior DLL. Handwritten native extensions are a separate
explicit advanced facility, not an ordinary-script fallback.

Draft save, source validation, preview, Test Character and installation have
different side effects. Apply a coherent draft at a practice reset; never patch
half a skeleton/program into a live match. A failed candidate leaves the last
accepted source/live preview intact. Report the actual failure, not a success
label after partial publication. Do not claim power-loss transaction safety
unless it has been established independently.

## Immediate implementation order

1. Complete compatible source skin import, extra bones, rigid rest compensation,
   source-aware reimport and useful painting/storage coverage.
2. Review Workshop usability with the user using actual Rook/Mario data. Fix
   concrete issues across all modes without redesigning their accepted layout.
3. Verify source-only edit/install/gift workflows in isolated stores, including
   paired families, and the complete friend release payload.
4. Address remaining original-style text/layout polish collaboratively.

Optimization is deferred. Whole-move transfer and Kirby are also deferred;
neither blocks the current clone/edit workflow. Cross-network/physical-PC
acceptance is for the user's later testing, not a prerequisite for local work.

## Model and animation boundaries

Model is import/materials/rigging/skinning, not mesh modeling. Use the compatible
Melee rig with semantic bone roles; arbitrary foreign-rig conversion is deferred.
Keep Edit rest/bind data separate from temporary Pose offsets and animated clips.
Update every affected skin/animation/attachment/role reference together when
adding/changing bones. Respect engine-controlled helpers at runtime.

Shared graph curves use the native constant/linear/Hermite contract with linear
time. Arbitrarily weighted Bezier time handles are unsupported. Pose transfer
copies evaluated local transforms at source frames, not move behavior or live
links. Preserve destination channels/tangents unless explicitly edited. Use the
native evaluator to check integer/fractional frames and loop boundaries.

G/R/S uses the immutable starting pose, axis/plane restrictions, commit/cancel
and selected-bone pivots. Auto Key starts off; temporary animation edits are
keyed explicitly unless enabled. Weight painting is bind-space editing even
when the visible model is posed.

## Acceptance and evidence

Exercise clone -> edit -> test -> save/reopen -> install with independent ownership.
Include multi-hit/looped events, interruption/ground-air behavior, edited poses,
root motion, resources, undo/recovery and Blender interchange. Test relevant
gameplay consequences, not only that controls display a value.

Use targeted grouped comparisons against stock for changed engine/script
contracts. Existing 26-form preparation and roster comparison coverage is in
[roster status](SCRIPT_PORT_CHECKPOINT.md). A single passing branch does not
prove every interaction; an untested branch is not evidence that implementation
is missing. Respect the user's role in interactive usability testing.

Maintain generic APIs and linked author-facing docs. Organize source by
responsibility, without forwarding wrappers or compatibility paths. Keep fixed
runtime pools and allocation at explicit transitions. Preserve unrelated work.
The root private upstream backs up the source; do not push donor repositories
or unrelated future changes without authorization.
