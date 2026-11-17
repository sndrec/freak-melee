# Inspect and capture a Workshop preview

Open a draft with `session.open` and wait for `session.inspect` to show
`preview_ready: true`. The `preview_view` object describes the current camera,
playback, overlays, and the clip and frame applied to the frozen actor.
`loaded_preview_package_id` identifies the loaded package. The editor can apply
live pose and hitbox changes over that package, so a capture also records the
live draft revision and the loaded fighter's revision.

## Choose a view

- Select an action and frame with `session.actions.select`.
- Use `session.animations.select` to seek the Animation workspace, then
  `session.animations.preview` to read the actor's actual bone transforms.
- Use `session.models.select` for Model playback, selected bones, and
  editor-only mesh visibility.

`session.preview.configure` takes the instance and current revision. Set
`camera.target` as an XYZ vector in game units, `camera.yaw` and `camera.pitch`
in radians, and `camera.distance` in game units. You can pass only the camera
fields you want to change. Pitch ranges from −1.45 to 1.45 radians and distance
ranges from 8 to 220 units. Alternatively, set `reset_camera` to `default` to
restore default framing or `front` to straighten the current view.

Use `hitboxes`, `hurtboxes` and `bones` in Action, and `bones` in Animation.
Set `playing` in either workspace to start or pause ordinary preview playback.
The live revision reflects camera and overlay edits and paused frame changes.
The actor's applied frame is reported separately as `preview_view.pose.frame`.

## Save a PNG

Call `session.preview.capture` with the instance, the inspected revision,
`scope: "viewport"` or `scope: "window"`, and a new absolute `.png` output
path on a local drive. The viewport image crops to the game's presented
rectangle. The window image includes the full Workshop interface. Pause playback
before capture when you want to compare an exact frame against another image.

Poll `session.preview.capture_status` with the same instance and returned
`capture_id`. A `ready` result includes the PNG location, dimensions, byte
count, hash, presentation frame number, and the view metadata for that image.
MCP also returns the PNG as an image content block. The CLI returns its path
and metadata as JSON. Preserve the PNG until the MCP client has retrieved it.

One capture can be in progress per instance. Starting the next capture replaces
the previous receipt. Use a new output path for each capture. A `failed` result
includes the reason the requested frame or file could not be captured.
