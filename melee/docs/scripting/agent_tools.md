# Create fighters with an AI agent

Freak Melee includes `native_content_tool.exe` beside the game. It can inspect
your fighter drafts, check their scripts and read the installed scripting reference. The current
agent tools provide read-only inspection and validation; run `agent list` to see the operations
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
