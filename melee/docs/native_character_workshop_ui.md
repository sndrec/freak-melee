# Character workshop: approved UI baseline

Status: user-approved layout and interaction direction, 2026-09-24.
This is an implementation specification, not a claim of working engine tools.
The preserved interactive study is [workshop-modes.html](mockups/workshop-modes.html).
Its synthetic fighter, script and preview data are placeholders. The pose-copy
workflow below supplements that study; its controls are not drawn there yet.

The study also includes **Model mode**, endorsed 2026-09-26, with Meshes,
Materials, Skeleton and Skinning contexts. The user's revision replaces
Deformation preview with explicit Edit, Pose and Animation interaction modes.
These extend the previously approved three-mode baseline. See
[Model workspace design](native_model_workspace_design.md) for the compatible
rig boundary, import flow and explicit mockup limitations.

## Workspace and modes

The main sidebar opens a full-window Workshop draft browser, with cloning and
saved local projects. Collection also lists editable drafts separately from
installed packages and can open a draft directly. Once opened, the workshop
edits one fighter project; its header identifies that fighter, returns to the
entry point and provides mode switching and save/apply operations. Do not keep
the fighter collection or clone button in the working editor.

Use three full work modes, each with space allocated for its own task:

- **Action Editor:** searchable move/state tree, live 4:3 practice preview,
  contextual event/hitbox inspector, event timeline and ordered command list.
- **Scripting:** handler/file outline, large code editor, diagnostics and source
  links, API reference and smaller practice preview. Evaluate the concrete
  syntax and editing experience with actual Mario clone data and handlers.
- **Animation:** large rig viewport, bone selection and transform tools, local
  transform inspector, clip properties and shared animation curve graph.

Preserve the selected state, clip and frame across modes, and preserve each
mode's relevant selection and view position. Undo belongs to the document,
not a tab. Links to a handler, clip or source event switch to the appropriate
mode without losing context. Saving a draft, applying at a practice reset and
installing a built package remain distinct operations.

## Simultaneous action events

Expandable hitbox groups expose a separate aligned lane for each stable hitbox
ID. Three hitboxes created at the same frame remain individually selectable;
their markers share the same time coordinate rather than being spread across
fake frames. Show the frame's commands together in explicit execution order.
Changing that order does not change their timestamps. Preserve clear/create
ordering and the original subaction control flow; runtime-dependent branches
must not become an invented fixed timeline. The command list remains available
when a compact timeline cannot express an operation faithfully.

Drag an event to update its timing continuously. G starts a grab for the selected
event; move the mouse, then left click to confirm. Right click or Esc restores
the starting timing. A confirmed move creates one undo step. Moving a looped
event moves every repetition by the same amount, preserving the loop spacing.

The timeline generates waits from event times. Events at the same time share
a wait; separating them adds a wait and bringing them together removes the
redundant wait. Other events retain their times. Timers remain visible and
editable in the event list. Events currently stay within their containing loop
interval; animation-dependent timing uses the event list.

Hitboxes have a duration in animation frames. Drag the bar to move both ends;
drag its left or right edge to resize it. Each loop repetition follows the
same edit. The inspector also provides a Duration field. Hitlag pauses the
duration, and animation speed controls how quickly it advances. Intervals in
the same hitbox slot stay non-overlapping.

Refresh Hit Eligibility lets an adjacent hitbox in the same slot hit an
opponent again. With it off, the next interval retains the opponents hit by
the previous interval. A gap starts a fresh hitbox. Opening a draft converts
static create/clear pairs into these intervals. Save keeps the conversion.
Actions whose endings depend on runtime decisions retain their commands and
can be edited through the event list.

## Animation graph

Show the enabled channels of all selected bones on one shared value-over-time
graph, with channel colors, visibility toggles, animated-only filtering and
optional normalization for comparing channels with different units/ranges.
Provide box/shift selection, grab/rotate/scale of selected keys, selection or
cursor pivots, tangent editing, zoom/pan, frame-all and a larger graph view.
A dope sheet is a secondary overview, not a substitute for the shared graph.
Inspector values always retain their actual units, including when normalized.

Native scalar curves use constant, linear and Hermite interpolation with
endpoint slopes and linearly progressing time. Arbitrarily weighted Bezier
time handles are not part of the native curve contract. Graph transformations
are authoring operations on keys/slopes, subject to representable key times
and valid ordering. Explain invalid results rather than silently discarding
keys or changing interpolation. Validate edited curves through the native
evaluator; preserve untouched imported tracks. Unsupported Blender curves need
an explicit bake/error-tolerance choice as described in the animation design.

## Pose capture and endpoint reuse

Include this in the initial animation editor, independently of the deferred
whole-move copying feature:

1. Choose a source animation and frame, preview it, and **Copy pose**. Capture
   evaluated local bone transforms, not merely keys that happen to exist at
   that frame. Fractional sampling follows the native evaluator.
2. In the destination clip, **Paste pose** at the current frame, clip start or
   clip end. Choose all bones or the selected bones. Show source clip/frame and
   destination frame so endpoint replacement is explicit.
3. Preserve destination root translation by default, with an explicit option
   to include it. Leave unselected bones and unrelated keys/tracks unchanged.
4. Insert/update editable transform keys as one undoable operation. Do not
   change clip duration, copy move events or create a live link to the source.
   Later source edits do not alter the pasted pose.

Copying a pose does not imply copying its source tangent or forcing a hold.
Preserve existing destination interpolation/tangents where applicable; use the
editor's visible interpolation defaults for new keys. Authors can adjust the
approach/departure curves explicitly after pasting.

The first required path is between clips of the same fighter/skeleton.
Cross-skeleton imports require explicit mapping and unmapped-bone feedback;
they do not gate this initial workflow. A reusable named pose library can
follow later. Ledge-hang, standing-idle and airborne/fall poses are source
clip/frame choices, not assumptions about universal animation indices.

Acceptance includes a ledge getup starting from a captured hang pose and ending
at standing idle, plus grounded/airborne endpoint examples. Verify native
evaluated local transforms, preserved root placement, selected-bone isolation,
undo/redo and save/reopen. Endpoint keys alone do not prove an entire move's
transition/collision behavior: test the resulting action in practice as well.

## Implementation handoff

The UI baseline is agreed; no additional layout proposal is required before
implementation. Read `native_character_workshop_progress.md` and current code
before selecting the next task. Connect the approved modes to existing project,
command, script, animation and preview functionality, addressing real gaps as
found. Do not repeat historical backend proof steps already completed.

Review real Mario data, native preview interactions and screenshots with the
user as the interface becomes functional. Whole-move transfer, broad character
template coverage and later polish retain their existing deferred scope.
Goal execution and model switching remain controlled separately in the app.
