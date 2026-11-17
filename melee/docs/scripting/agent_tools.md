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

`examples.list` and `examples.read` expose the installed FMS templates and
supporting examples. They are also available as MCP resources under
`freak-melee://examples/`. The installed reference links to local copies of
these files. Links marked **developer reference, online** provide optional
implementation notes on GitHub.

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
