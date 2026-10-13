# Controller feedback and camera quakes

These operations request shared engine presentation effects. Scripts choose
their timing; the engine uses its normal controller preferences and camera
bookkeeping. They do not install a stock fighter's move handler.

## `controller.rumble(pattern, duration)`

Request a loaded rumble pattern for the current Fighter's controller. Both
arguments are integral values and use native 32-bit conversion. The pattern
index selects the loaded pattern commands and priority; it is not a strength.
Duration `0` uses the native one-shot convention (`-2` at the controller layer).
Other duration values are passed through with the native signed bit pattern.

```text
controller.rumble(12, 10)
```

The original eligibility rules apply: the player must have rumble enabled and
the Fighter must be eligible. Ineligible Fighters do nothing successfully,
including when pattern/controller values would be invalid if actually used.
Eligible requests reject unloaded pattern indices or invalid controller slots
before hardware access. This is a Fighter operation; an Article uses
`item.rumble(pattern, duration)` for its damage owner's separate native path.

The call retains the source's physical lifetime, program and storage identities
through the request. If a native callback retires or changes that source, the
script call fails without publishing its output. A request already sent to the
controller is not undone. Authoring checks and backend comparisons use silent
controller leaves; physical controller behavior requires runtime testing.

## `camera.quake(kind, x, y, z)`

Request one of the four defined native quake kinds at a world-space epicenter.
Available to Fighters and owned Articles. The position components are Float32;
the engine copies them into its existing quake bookkeeping. This request does
not move the Fighter or perform a stage collision query.

See [combat visuals](combat_visuals.md) for related effects and
[joint feedback](joint_feedback.md) for built-in effect argument layouts.
