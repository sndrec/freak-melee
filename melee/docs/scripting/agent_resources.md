# Resource authoring

Resources are named files owned by a fighter package. Use them for item
blueprints, billboard images, stock icons, HUD emblems and other binary data.
Each family member has its own resource collection.

## Inspect resources

Call `resources.read` with the draft's `project` and optional `member`.
For the open Workshop member, use `session.resources.read` with its `instance`.
The response contains:

| Field | Meaning |
| --- | --- |
| `resources` | Exact logical names, `png` or `blob` kind, byte count and SHA-256. PNG rows include width and height. |
| `bundle_entries`, `bundle_bytes` | The size of this member's complete resource collection. |
| `limits` | Maximum entry count, logical-name length, bundle size and individual file limits. |
| `articles` | Article declarations and slots present in the owned fighter data. |
| `sound_banks`, `effect_bank` | Bank IDs used by this fighter's assets. |
| `common_items` | Named stock item choices and their `stock_kind` values for blueprint creation. |
| `asset_formats` | Supported authoring paths for each asset category. |

`query` searches names without case sensitivity. `offset` and `limit` page the
resource rows; `next_offset` is null on the last page. Use the returned exact
name for later edits and script references.

## Import or remove files

`resources.edit` takes the inspected disk `revision` and a `changes` array:

```json
{
  "project": "C:/Fighters/My fighter",
  "revision": "<revision from resources.read>",
  "changes": [
    {"operation": "import", "name": "items/token.png", "kind": "png", "path": "C:/Art/token.png"},
    {"operation": "import", "name": "data/settings.bin", "kind": "blob", "path": "C:/Art/settings.bin"},
    {"operation": "remove", "name": "items/old.png"}
  ]
}
```

An import replaces an exact existing name or adds a new entry. A removal takes
only its operation and name; that name must exist. Supply each name once in a
batch. The whole batch succeeds or leaves the collection unchanged.

Logical names use letters, digits, underscores, hyphens, dots and `/` between
segments, such as `items/token.png`. Lookup is case-sensitive. Names must be
relative, contain ordinary path segments and avoid reserved device names.
Two names that differ only by letter case cannot share a collection.

PNG import checks the file structure, dimensions and decoded pixels. Item
blueprint blobs also pass native model and command-graph admission. Other blobs
retain their exact bytes; their consuming script or asset loader determines
their meaning. A resource name by itself does not activate an asset.

Each member can own up to 64 entries. Names can use up to 127 bytes. The complete
collection, including its directory, must fit in 8 MiB after the edit. A blob
can use up to 4 MiB. A PNG can use up to 256 KiB and measure up to 1024 pixels
on each side. `resources.read` reports these limits.

For live editing, call `session.resources.edit` with `instance`, `revision`,
`summary` and `changes`. It uses the current workspace's Undo history and leaves
saved files unchanged. Stock-icon and emblem import use this operation too.
Use Save to keep those changes. Offline edits save through the draft transaction
and refuse a draft currently owned by a live Workshop.

## Import fighter sounds

Import a DSP-ADPCM SSM file as a `blob` named `sounds.ssm`. Each fighter
family member can carry its own bank. The bank starts at sample ID 0 and
holds up to 256 mono or stereo clips at sample rates from 8 to 48 kHz.
The maximum bank size is 4 MiB. Build the fighter to validate the bank.

Use [`sound.clip`](sound_channels.md#soundclipchannel-index-volume-pan) in
a fighter script to play a clip by its index in the bank. The fighter
package carries the audio when installed or shared.

## Create an item blueprint

A blueprint supplies an item's model, attributes and animation commands.
The item declaration and callbacks in your editable scripts supply its behavior.
Use `resources.blueprint` to create a new blueprint file.

For a stock item, choose a value from `resources.read.common_items` and supply
your ISO:

```json
{
  "iso": "C:/Games/Melee.iso",
  "stock_kind": 0,
  "output": "C:/Art/capsule.item"
}
```

For an authored DAT, supply its file path and the Article descriptor's
data-section offset:

```json
{
  "dat": "C:/Art/item.dat",
  "article_offset": 256,
  "output": "C:/Art/custom.item"
}
```

Choose one source form. The operation validates the actual Article, model and
commands, then writes the complete DAT and descriptor offset into the blueprint.
It preserves existing output files. The result reports the selected offset,
DAT size, output hash and `graph_validated` status.

Import the resulting file as a `blob`, for example `items/capsule.item`. Then add
an item declaration in an editable source file:

```text
item MyCapsule damage 3 angle 45 lifetime 120:
    blueprint items/capsule.item
    icon items/token.png
    billboard_size 6
```

The optional icon gives the item a billboard appearance. A move can pull a
declared item with `items.pull(index)`. Read the [Item API](item.md),
[article declarations](article.md) and [held articles](held_articles.md) for
creation and contact behavior. [Fighter parameters](language.md#fighter-attributes)
can hold adjustable damage, speed and cooldown values.

## Other asset types

Use [model authoring](agent_models.md) for costume textures and materials.
Article geometry and animations currently come from the owned fighter DAT or
an item blueprint; [article models](article_models.md) explains their script
controls. Use [action events](agent_tools.md#edit-actions-and-events) and the
[presentation API](presentation.md) to select existing sounds and effects.

Custom audio-bank and particle-bank import have no authoring operation yet.
Adding an arbitrary sound or particle file as a blob stores it but does not
create a playable bank. Report a needed import capability through the
[agent feedback tools](agent_tools.md#report-a-bug-or-missing-capability).

## Export and test

`resources.export` and `session.resources.export` take a logical `name` and
an `output` path. They write the resource's exact bytes to a new file and return
its hash. They preserve existing output files and editor history.

After changing resources or their script references, run `scripts.check` for a
saved draft, or `session.scripts.check` for the open member. These checks identify
missing resources, wrong types and malformed item definitions. Resource editing
can leave an unfinished reference for you to repair before building.

Use `session.preview.refresh` or `session.practice.start` to load the current
resource collection. Wait for the requested package identity on the actual
preview actor, then test the item or HUD image. [Build and install](agent_tools.md#build-install-and-try-a-fighter)
when ready to use the result in matches, and export the resulting `.fmc` if you
want to share it.
