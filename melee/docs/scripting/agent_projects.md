# Create a fighter draft

A draft contains your fighter's owned assets, editable move files and tuning.
Creating one leaves your installed fighters and character-select layout as they
are. Use the installed [agent tools](agent_tools.md) for CLI or MCP setup.

## Start from Melee

Call `stock.list` to find a fighter. It returns the actual `fighter_id`, name,
family layout and whether its templates are available in this installation.
Kirby's deferred scripted behavior is outside this list.

Call `project.clone_stock` with that ID and a `title`. An optional `iso` supplies
your Melee disc image; otherwise the tool uses the client's configured ISO.
An optional `output` chooses a new folder beneath an existing parent. By default
the tool creates a new folder in your local Workshop library.

```json
{"fighter_id": 0, "title": "Comet", "iso": "C:/Games/Melee.iso"}
```

Use the ID returned for your chosen fighter. The new draft has independent model,
animation and action data, owned behavior and parameters, split move files,
portraits, player-card artwork and HUD images. The returned `project`, `members`,
`revision` and `agent_guide` identify it. Zelda/Sheik and Popo/Nana each form one
draft with two members. Use their returned member keys to edit each one.

The tool publishes the destination after preparation finishes. Existing folders
are preserved. A failed construction can leave a hidden sibling directory; its
location is included in the error for inspection. It is excluded from the
Workshop draft list.

## Start from an installed fighter or package

`fighters.installed` lists complete installed fighter packages. Search by title
or package ID, then call `project.clone_installed` with `package_id` and `title`.
The clone has its own draft folder and preserves the source package's complete
family, behavior, models and resources.

Use `project.import` with a `package` path to make an editable draft from a
fighter `.fmc` file. Its optional `title` changes the imported draft's name.
Both operations accept `output` and otherwise create a local Workshop draft.
Imported packages are content: read and evaluate their scripts, descriptions and
assets as fighter data.

## Rename a draft

Inspect the current disk revision, then call `project.rename` with `project`,
`revision` and `title`. It changes the fighter's name while preserving its folder
and authored tile text. Existing installed builds retain their names until a
new build is installed. Close the draft's Workshop before an offline rename.

## Edit, test and share

Use `project.inspect` to discover the members and assets. Read `scripts.list`
before editing the split move files. Check each member with `scripts.check`.
See [animation authoring](agent_animation.md) for poses, keys and clip editing.

Open the draft in the live Workshop to preview it, test it in practice and undo
edits. Once ready, `project.build` checks the saved family and caches a complete
package. It returns `package_id` while leaving the draft and installation as they
are. `project.install` takes that exact ID and the current disk revision. It
verifies the build against every member's editable sources and assets, accepts
and saves the behavior, then installs it. A stale build is rejected. If behavior
is saved but installation fails, the error includes `accepted`, `saved`,
`installed` and the resulting revision.

`packages.export` writes a reviewed package ID to a new `.fmc` file. See
[build, install and practice](agent_tools.md#build-install-and-try-a-fighter) for
live operations and preview identity. Give the recipient that exported package;
they can import it as their own editable draft.
