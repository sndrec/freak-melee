# Workshop feedback, round 2

Recording: `A:/videos/rec/2026-10-13/2026-10-13 12-54-12.mp4`.
Reviewed the video and microphone audio track 3. Implementation resumed at the user's request. Pause the goal after this feedback
pass is addressed and ready for the next user review.

## Implementation status

- [x] Add Event, selection and recorded crash.
- [x] Undo/redo and live animation/pose editing.
- [x] Per-channel keys, clip-length commits and consistent scrubbing.
- [x] Contextual inspectors, command meanings and bone names.
- [x] Graph grid, dropdowns, checkboxes and tooltips.
- [x] Solid gizmos and world/local space.
- [x] Model visibility and full material controls.
- [x] Readable common attributes and menu typography.
- [x] Focused verification and local commits, then pause.

The implementation is ready for the next user review. IK remains future work.
Unknown bone roles and attribute meanings retain explicit labels; the editor
does not infer their behavior from names alone.

## Fix order

1. Repair Add event, loss of selection, and the crash at the end of the recording.
2. Make editing reversible and live: undo/redo, scrubbing, curve transforms,
   pose values, individual channel keying, and clip-length commits.
3. Improve inspector context, event meanings, names, gizmos and controls.
4. Finish Model visibility and material coverage; revise the CSS heading.
5. Plan IK separately, as requested.

## CSS heading — 00:15–00:40

- Reduce the size of “CHOOSE A FIGHTER” to fit the surrounding stock menu.
- Replace Impact with lettering appropriate to this menu context. Inspect the
  original menu artwork before choosing or generating the face.
- Keep the lettering in the game renderer and attached to the menu scene.

## Action Editor — 01:14–05:54

- Prevent timeline labels from drawing outside their markers. Show the full
  event name on hover and in the inspector when selected.
- Show only properties relevant to the selection. Show Action Playback when
  an action is selected with no command/event selected; hide it while editing
  an event or hitbox.
- Give all meaningful opcodes names, explanations and timeline entries.
  Hide commands proven to have no effect from ordinary views.
- Make command-variable index and value prominent editable properties.
- Use contextual names for known roles. In common aerial attacks, command
  variable 0 controls the landing-lag window: 0 enables autocancel, nonzero
  uses the attack-specific landing action. The label must reflect this direction.
- Document common command-variable uses. Keep a generic index/value editor for
  moves that give those variables different meanings.

Source check: opcode 43 is a controller-rumble request, handled by
`ftAction_800728F8` and `ftCommon_8007EBAC` / `ftCommon_8007EC30`.
It should have a useful label rather than being discarded as a no-op.
The aerial landing interpretation is confirmed in `ftCo_LandingAir_EnterWithLag`.

## Animation interaction — 11:19–14:14

- Holding and dragging the playhead must update the selected frame continuously,
  matching Action Editor scrubbing.
- G/R/S must immediately begin moving/rotating/scaling the selected graph keys.
  Preview while moving the mouse; left-click confirms; right-click cancels and
  restores the original keys and tangents.
- Add meaningful bone names in the outliner. Use verified rig names/roles and
  allow authored names for joints that have no name metadata.

## Pose values and channel keys — 14:14–16:50

- Editing a pose value updates the preview without a separate Apply Pose step.
- Arrange position X/Y/Z, rotation X/Y/Z and scale X/Y/Z in one vertical column
  with enough room for precise numeric values.
- Give each channel an Insert Key control. I while hovering a property keys
  that property at the current playhead using its current pose value.
- Apply the end-frame change on Enter, rather than while typing or through a
  separate Apply Clip Length button.
- Preserve the established Auto Key option and its default-off behavior.

## Graph controls and grid — 17:08–20:46

- Use a pivot dropdown instead of a cycling button. Tooltips explain Selection
  (center of the selected keys' bounds) and Cursor (the graph's 2D cursor).
- A selects visible keys when none are selected, and deselects when a selection
  exists. Remove the redundant Select Visible button.
- Replace unexplained +/- and fitting controls with clear pan/zoom interactions
  and tooltips. A dedicated Frame All button is unnecessary for this pass.
- Anchor grid lines to frame/value coordinates. Panning moves the lines with
  their values; zooming adjusts interval density.
- Use a checkbox for Animated Only and an obvious on/off indication for other
  toggles. Start with normalized values off.
- Label every remaining control and provide a tooltip where its purpose is
  unclear.

## Undo, redo and gizmos — 21:00–22:26

- Ensure undo and redo actually work for editing operations, including curve
  transforms, pose changes and numeric edits. Restore the preview along with
  the document. Existing buttons alone do not satisfy this feedback.
- Use stable 3D gizmo geometry: arrows for translation, rings for rotation,
  lines with cubes for scaling.
- Pick each complete axis or ring, rather than individual ribbon fragments.
- Add a world/local transform-space control for the selected bone.

## Future IK — 22:26–23:30

- Plan authoring-time inverse kinematics for tasks such as moving a planted
  foot without manually rekeying its whole chain.
- Store the resulting animation in the supported joint-channel format.
- This is explicitly future work, not a prerequisite for the current fixes.

Use selected bone chains with an explicit end effector and optional pole
target. The solver should modify a temporary authoring pose; Insert Key and
Auto Key store the resulting joint channels. Constraints and chain selection
need a separate design review before implementation.

## Attributes — 23:34–24:16

- Give common attributes readable names and descriptions of their gameplay
  effect and units.
- Investigate currently unnamed values such as `x150_damageice_unk` in the
  original game code. Keep uncertainty explicit until its meaning is established.
- Tooltips must explain the setting rather than repeat its internal identifier.

## Model materials and visibility — 24:19–27:04

- Audit material properties and render flags against the actual model format
  and renderer, and expose the missing supported settings with clear controls.
- In Model mode, “all meshes visible” currently fails to reveal Ganondorf's
  sword. Trace fighter visibility groups, animated visibility and editor eye
  controls before fixing their interaction.
- Make accessories discoverable and editable. Provide a clear way to make the
  sword visible in an authored move, using general visibility operations.
- Confirmed: Model mode uses mesh eye controls as the sole visibility controls.
  Animation playback and Test Character use authored visibility behavior.

## Add event and crash — 27:05–28:06

- Add event produces no apparent result. Make it open a usable event chooser
  and insert the chosen event at the requested frame.
- Later, selection stops responding and the client exits.
- The visible terminal records exit code `-1073741819`, which is `0xC0000005`
  (access violation). The recording identifies the interaction sequence;
  the dump identifies a null timeline element in
  `ActionTimelineEvents::ProcessEvent`, caused by a stale element ID.

## Review artifacts

`transcript.txt` / `transcript.json` contain the speech transcription.
`contact-1.jpg` through `contact-4.jpg` cover the video at 30-second intervals.
`detail-1680.jpg` shows the final terminal and crash exit code.

Implementation evidence is under
`C:/FreakMelee-review/2026-10-13-workshop-feedback-2/implementation/`.
The build log is at
`C:/FreakMelee-review/2026-10-13-workshop/feedback-build.log`.

Focused verification used a fresh Ganondorf draft in isolated application data.
It covered Add Event with no command selected, timeline event dragging, live
pose editing and focused-field undo, per-channel key insertion, clip length
on Enter, graph scrubbing and move confirmation/cancellation, solid gizmos,
and Ganondorf's sword in Model Edit/Pose modes. A material edit was saved and
its custom blending/depth record verified in the resulting costume DAT.
Reopening and reapplying the unchanged material preserved the costume bytes.
Animation editing, pose transfer and action duplication roundtrips passed.
The independent insertion audit covered absolute/relative timers, insertion
after the final event, empty scripts, loops, and editing a variable when other
actions have empty scripts. Other actions and existing event timings were
preserved in those cases.

These checks establish the exercised interactions and saved data. The next
user pass evaluates the complete workflow, camera interaction and usability
with real projects.

### Source corrections

Follow-up: event dragging and G now preview continuously, with confirm/cancel
and one undo step per gesture. The timeline compiles event times into relative
waits, merging shared times and preserving other events and loop periods.
Timers stay in the event list and are hidden from the timeline frame stack.
Moving across a containing loop interval remains explicitly rejected.
Independent retiming checks cover simultaneous hitboxes, wait merging, repeated
loop events, frame-zero events, and atomic rejection at loop boundaries.
Focused client checks confirmed G updated the event from frame 20 to 22 before
confirmation, Esc restored frame 20, left click confirmed the move, direct
dragging committed it, and one undo restored the previous timing. Timeline
rebuilds occur outside input dispatch and restore hover/focus, preserving mouse
release and keyboard input throughout the gesture.

- The recorded crash dereferenced a removed timeline element ID during drag
  completion. Dragging now uses the current ruler and checks its presence.
- Empty action scripts are valid. They no longer stop the shared-command
  ownership scan from allowing edits to other actions.
- A fighter has four live hitbox slots. Command copying allocates an unused
  slot or reuses the copied command's slot; it never creates a fifth slot.
- The material's custom blending/depth pointer is at MObj offset 20. Offset
  16 is a separate field. Material edits now write the renderer's actual field.
- Numeric model and texture fields retain enough decimal precision to
  preserve an unchanged float when the material is applied.
