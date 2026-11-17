# Create fighters with an AI agent

Freak Melee includes `native_content_tool.exe` beside the game. It can inspect
your fighter drafts, edit their script files and parameters, check their scripts
and read the installed scripting reference. Run `agent list` to see the operations
available in your installed build.

You can give a coding agent this folder and ask it to read this guide. The
reference and stock script templates are available locally under `ui`.

## Use a terminal

From the folder containing `native_content_tool.exe`:

```powershell
.\native_content_tool.exe agent list
.\native_content_tool.exe agent call environment.describe
.\native_content_tool.exe agent call projects.list
```

`agent list` returns the names, descriptions and JSON argument schemas of the
available tools. `agent call` returns one JSON object. Successful calls contain
`ok: true` and `result`; unsuccessful calls contain `ok: false` and an `error`
with a code and message. Errors produce a nonzero exit code.
For large arguments, use `agent call TOOL --stdin` and send one JSON object on
standard input. Stdin and MCP accept up to 4 MiB of arguments per request;
inline arguments also have Windows' command-line length limit.

Use the absolute project path returned by `projects.list`. `project.inspect`
lists its members; specify a member key to inspect Zelda/Sheik or Popo/Nana
independently. `scripts.list` identifies the editable files and whether they
match the validated behavior. `scripts.read` returns their current contents.

Find symbols and explanations with `docs.search`, then use `docs.read` to read
a complete page. `attributes.list` describes common fighter settings.
`actions.list` resolves action names and IDs for the actual edited fighter.
Searches accept `query`, `offset` and `limit`; use `next_offset` to continue.

Use `parameters.read` to inspect typed parameter overrides and imported stock
values. Existing overrides take precedence over imported defaults. Invalid
source and parameter imports remain readable so the agent can repair them.

`scripts.check` validates the editable files against the fighter's owned assets.
Its `accepted` field reports whether the candidate is valid. A completed check
with script errors is a successful tool call with `accepted: false`; read its
`diagnostics` for the file, line, column and message. Independent errors are
collected together. Locations refer to the text returned by `scripts.read`.
The check leaves the draft and accepted behavior unchanged.

## Edit a saved draft

Inspect `scripts.list`, `scripts.read` and `parameters.read` before editing.
They return a `revision` identifying the saved project's files. Supply it to
`scripts.update` with a list of `{ "file": "...", "text": "..." }` objects and,
optionally, the complete `parameters` text. Use file paths from `scripts.list`.
An empty `files` list lets you change parameters alone.

One call saves the whole batch together. First save creates the template's
separate fighter-state and move files. Explicit parameter overrides retain
their values. Syntax errors in move code can be saved as a draft; use
`scripts.check` to find and repair them. Invalid parameter declarations are
rejected before saving.

`revision_conflict` means another edit changed the project. Read the current
files and revise your edit. `draft_busy` means a Workshop or another authoring
process owns the draft; close that draft before using offline writes. Saved-file
inspection remains available. Use the live session tools below to edit a draft
that is open in the Workshop.

Successful edits return `committed: true`, `status: "saved_draft"` and the new
`revision`. Checking, building and installation remain separate steps. Keep the
returned revision for your next edit. The tool keeps recovery backups during a
batch; the next exclusive open restores an interrupted edit before loading it.

## Work with the running Workshop

Use `instances.list` to find running clients and their executable/build identity.
Choose the instance you intend to edit, then call `session.inspect` with its
`instance` value. A client still loading a scene may temporarily report
`available: false`; wait for that same process rather than launching a duplicate.
Live editing requires the authoring tool built with that client. When
`build_matches` is false, use the `native_content_tool.exe` beside the returned
game executable.

`session.open` opens a saved draft in the chosen client. `session.member.select`
switches the edited member of a family, and `session.workspace.select` selects
Action, Scripting, Animation or Model. Each takes the current live `revision`.
Open refuses to discard existing unsaved edits. A real online match, preview
transition or active edit gesture prevents live mutations.

`session.scripts.list` and `session.scripts.read` return the selected member's
live source files, including unsaved text. `session.scripts.select` shows a file
in the Scripting pane. To change source contents, use
`session.scripts.update` with its live revision, a short `summary`, the same
`files` array used for offline edits, and optional `parameters` text. One batch
creates one Workshop undo step. `session.undo` and `session.redo` use the current
member/workspace's history. Source history preserves intervening manual text
edits by refusing to overwrite a different source set.

Live source edits change the working draft. `session.save` saves every member's
assets, source files and parameters together, while keeping the Workshop undo
history. It compares disk files with the revision observed when the draft opened
or last saved. A conflict preserves both the external files and your unsaved
edits. `saved_disk_revision` identifies the saved project; `revision` identifies
the current live editor state.
Installing a build updates its receipt separately from the draft's saved revision.

Behavior validation, preview refresh and installation are separate operations.
Saving editable source preserves the last accepted behavior until you build it.
`preview_scene` describes
the currently loaded scene; a source edit's `preview_status: "refresh_required"`
means that behavior has not been applied to the running preview.
`session.scripts.check` checks the selected member's current source, parameters
and assets, including unsaved edits. Supply the current live revision. It returns
errors with relative file paths, line numbers and columns, plus completeness
flags and source/parameter hashes. The same diagnostics appear under Check script
in the Workshop. Clicking a problem opens its source file and line.

Checks run in a separate native validator from the same installation, with a
15-second limit. The returned revision identifies the editor state checked.
Your saved draft, accepted behavior, running preview and undo history stay at
their current revisions. `scripts.check` checks the saved files instead.

Fighter → Agent tools shows the instance and recent operations. Its Disconnect
button, or `session.disconnect`, closes agent access while keeping the game and
draft open. Enable access there again when desired. This creates a new instance
identity, so discover it again before attaching.

## Build, install and try a fighter

For an open Workshop draft, use `session.build` with the current live revision.
It checks every family member, builds a complete package, and accepts and saves
the behavior together. `member_reports` contains each member's diagnostics.
An `accepted: false` result means the source needs attention; the previous
accepted behavior stays available. Building preserves editor undo history.

`session.install` builds and installs the current fighter. Existing CSS tiles
that used this draft's previous installed build move to the new package.
Both operations return the canonical `package_id` identifying the complete
fighter package. The installed copy and the running preview are separate states.

Use `session.preview.refresh` to load current edits into the frozen preview,
or `session.practice.start` to test the whole family with normal controls.
These operations cache a snapshot while keeping your saved draft as it is.
`session.practice.stop` returns that test to the frozen editor.

Scene loading finishes after the request returns. Poll `session.inspect` on the
same instance until `preview_ready` is true and `loaded_preview_package_id`
matches the operation's returned `package_id`. That loaded ID comes from the
actual actor. `fighter_revision` identifies the effective fighter data, separate
from view selection and undo state; portrait bytes are included in `package_id`.
`preview_fighter_revision` and `preview_matches_fighter` describe the loaded copy.
`accepted_behavior_matches_sources` reports whether the editable move files match
the accepted behavior. `installed_package_id` identifies the installed copy.

For a saved draft outside the Workshop, `project.build` takes its disk `revision`,
checks all members and caches an installable package while preserving draft files.
Its portrait comes from the draft or the installed default; an explicit `portrait`
PNG can override it. `packages.export` writes a cached `package_id` to a new
`output` .fmc file. The exported bytes hash to that ID. Both operations normally use
your installation's content cache; `content_root` selects another local cache.
The live inspector also reports that instance's `content_root` and `projects_root`.

`examples.list` and `examples.read` expose the installed FMS templates and
supporting examples. They are also available as MCP resources under
`freak-melee://examples/`. The installed reference links to local copies of
these files. Links marked **developer reference, online** provide optional
implementation notes on GitHub.

## Edit actions and events

Resolve each fighter's real actions with `actions.list` or
`session.actions.list`. Use the returned ID as `action_id`. The live tools read
unsaved changes in the open member; saved-draft tools read its files.

`actions.read` and `session.actions.read` offer these views:

| `view` | Contents |
| --- | --- |
| `metadata` | Playback flags, animation availability, command count and timeline limits. |
| `events` | Event handles, kinds, first frames and repetition counts. |
| `event` | One event's properties, field descriptions, ranges, choices and repeated frames. Supply `event_id`. |
| `timeline` | Events in execution order, including each loop repetition. |
| `types` | Available event kinds, their creation defaults and field definitions. |

Lists support `offset` and `limit`; event and type lists also accept `query`.
Timers stay hidden until `include_timers` is true. A `timeline_complete: false`
result identifies a trace stopped by a command such as an animation-end wait
or a tracing limit. Frames beyond that trace are unknown.
For duration-owned hitboxes, `duration_end_frame` is the start frame plus the
authored duration. An explicit clear or an all-slots expiration can end it
earlier. Use the preview or a practice check to verify its runtime lifetime.

Use `actions.edit` for a saved draft or `session.actions.edit` for the open
Workshop. Supply the inspected `revision`, `action_id` and `operation`. A live
edit also takes a short `summary` for Undo and selects the edited action in the
Action Editor. Related property changes commit together; a rejected edit keeps
the original archive.

| `operation` | Arguments and result |
| --- | --- |
| `metadata` | Set an action's editor `label` and/or `hidden` flag. An empty label restores its default display name. |
| `playback` | Set `loop`, `root_motion`, or the supported `animation_flags` bits. |
| `own_hitboxes` | Convert proven hitbox lifetimes from clear events into editable durations. `converted_hitboxes` reports the count. Unresolved patterns keep their original commands. |
| `create` | Supply `kind`, `frame` and optional `properties` from the types view. Creates a new event directly, including an action's first event script. |
| `update` | Supply `event_id` and a `properties` object. Update damage, element, radius, attachment, knockback, eligibility and other fields returned by the event view. |
| `timing` | Supply `event_id`, either `frame` or `delta`, and an optional hitbox `duration`. Moves all repetitions together while preserving the loop pattern and other event times. |
| `delete` | Supply `event_id` to remove that authored event and its repetitions. |

To edit several events together, use `operation: "update"` with `updates`:

```json
{
  "updates": [
    {"event_id": "<first inspected event>", "properties": {"damage": 10, "element": 2}},
    {"event_id": "<second inspected event>", "properties": {"damage": 8, "radius": 3.5}}
  ]
}
```

Include the usual project/instance, action ID, revision and live summary. Each
handle comes from that same revision. A batch accepts up to 256 distinct events
in one action. It commits as one edit; an invalid entry preserves the whole
draft. Use one entry per event, combining that event's properties there.
`updated_events` returns the previous and new handles, requested changes and
applied properties, including quantized values. For one event, `event_id` and
`properties` remain the shorter form.

Hitboxes created here own their duration. `refresh_hit_eligibility` controls
whether replacing a hitbox permits another hit on an opponent it already hit.
Angle 361 selects the Sakurai angle. Radius and hitbox offsets use game units;
the returned `quantum` gives their precision. Bone indices belong to this
fighter's rig; `use_common_bone_ids` selects the engine's named bone roles.
Some less common commands expose fields marked `expert`; their descriptions
and bit ranges describe the available operands.

An `event_id` is an opaque handle for the inspected action and revision. Use
the fresh handle returned by an edit, or inspect again after a structural
change. Finish with Save/Build as appropriate, then refresh the preview to run
the edited fighter. `session.actions.select` selects an action and optional
frame without changing its data.

## Report a bug or missing capability

Use the feedback tools when an authoring operation fails reproducibly or a
fighter needs a capability that the available scripting operations cannot
express. For a feature request, describe the behavior you want, the operations
you considered, and the generic capability that would help other fighters too.
Continue independent work after recording the obstacle.

1. Call `feedback.prepare` on the running instance. Supply a unique `task_id`
   for this authoring task and a short `issue_key` for this obstacle. Choose
   `bug` or `feature_request`, and describe the expected and observed behavior.
   Optional fields include `operation`, `reproduction`, `diagnostics`,
   `capability`, `alternatives` and screenshot annotation `pins`.
2. Inspect the returned `id` with `feedback.inspect`, using it as `report_id`.
   The local `screenshot`, `context` and `notes` paths contain the captured
   evidence. Wait for `attachments_ready: true` before submitting. The
   screenshot's completed write is recorded locally at this point; inspect it
   before closing the game so an interrupted capture stays distinguishable
   from a complete one. The
   `agent_report.captured_workshop` fields identify the draft and preview at
   capture time. Supplying `revision` to prepare requires that exact live
   revision; omit it when reporting a problem without an open draft.
3. Call `feedback.submit` with that `report_id`. This sends the original capture
   through the same F2 service and admin feedback queue used by players. The
   agent host controls permission for this network operation. Check
   `feedback.status` until `stored: true` confirms receipt. A `retry` status
   retains the files and error; call submit again with the same ID.

Preparation captures the displayed screen and gameplay diagnostics immediately.
Submission and retries keep those bytes, even after you change the fighter.
Reusing a task and issue key returns its original report instead of capturing
or sending another copy. Agents should keep that ID when an operation times out.
Use `feedback.list` to recover it after reconnecting or restarting the program.
If an upload's outcome is uncertain, resubmitting its sealed capture resolves it
without creating a second report.

Reports are marked as agent-authored in their notes and diagnostic context.
Players can read their categories, descriptions and status in Workshop → Fighter
→ Agent tools → Agent feedback. The local collection holds up to 16 reports,
with a 128 MiB storage limit. After inspection, `feedback.discard` removes a
local copy; a report already received by the server stays in the admin queue.
Discarding removes its local duplicate-detection record too. Keep submitted
report IDs in your task notes so you can refer to the original report later.

Automatic diagnostics exclude authentication data and account keys. Fighter
source files and assets are not attached by these operations. Include only the
smallest relevant diagnostic text in your description; omit credentials and
private content from text and screenshots. Reporting works from the hub even
without a draft or preview. Finish an open F2 annotation session before asking
the agent to capture another report.

## Connect through MCP

For a setup installation, configure your agent's MCP client to start:

```text
powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "C:\Games\Freak Melee\freak_launcher.ps1" -AgentMcp
```

Replace the example path with your installation. MCP clients usually accept
the executable and arguments separately:

```json
{
  "mcpServers": {
    "freak-melee": {
      "command": "powershell.exe",
      "args": ["-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass",
               "-File", "C:\\Games\\Freak Melee\\freak_launcher.ps1", "-AgentMcp"]
    }
  }
}
```

The launcher selects your active installed build. This entry remains the same
when that build is updated. It starts the authoring server without launching
the game or downloading updates. You can keep playing while an agent reads
the reference or inspects saved drafts.

For an unpacked build folder, configure the executable directly with the single
argument `mcp`. This path refers to that build rather than the installation's
active version.

## Understand which source is active

Editable move files describe your working draft. Validated behavior is the
source accepted for the fighter's playable package. Saving edits and building
a playable package are separate steps. Inspection reports their status so an
agent can tell which source it is reading.

Paired fighters have one project containing both members. Choose the member
before inspecting actions or sources. Resolve action IDs and bone roles from
that fighter's data instead of borrowing numbers from another character.

Account keys and online administration are separate from fighter authoring.
The authoring tools do not need those credentials.
