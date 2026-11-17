# Canonical owned source resources

`native_source_resources.h/.c` is an allocation-free, engine-independent codec
for named PNG and opaque blob entries. It performs no file access and owns no
global registry. Package preparation must copy the encoded bundle into its final
immutable owner, then decode borrowed views from those copied bytes. The same
program, asset-table, reader and rollback-history lifetime leases that pin other
candidate assets must pin this input before publishing any resource pointer.
Releasing/replacing the blob invalidates every directory name and payload view.

## Actual consumer requirements

`examples/rook_workshop/moves/005_NeutralSpecial.fms` declares nine Workshop Item
recipes. Eight have exact `icon items/snack0.png` through `items/snack7.png`
names; the ninth has no icon. Those actual PNG files are 450â€“681 bytes each.
`native_script_item_program.c` requires an exact name match and stores the
prepared borrowed PNG bytes/length in a data-only `NativeItemDefinition`.
`billboard_size` is separate recipe metadata; it is not encoded in this bundle.
The codec therefore provides exact logical names, typed byte spans and derived
PNG dimensions without any Fighter-specific or Rook-specific policy.

The audited legacy `native_holdable_visual.c` uses Definition-owned recipes and
24 decoded visuals per kind; it must not be used to claim ordinary source
presentation. Its pixels are decoded at activation, then reused for drawing.
Actual `native_gx_image.cpp` uses WIC and supports dimensions up to 1024Ã—1024,
converting to GX RGBA8 tiles. Source presentation integration is a separate owner
and is not implemented by this codec slice.

## API and storage

The lightweight header is valid in C and C++. `Measure` determines canonical
size, `Write` writes into caller-reserved bytes, and `Read` fills a caller-owned
fixed directory. `Find` returns a typed exact-name entry. There are no tick
allocations, compression transforms, native callbacks or path-backed pointers.
Every declared entry has a nonempty payload. `Read(NULL,0)` is the optional absent
bundle and zeros the entire view. A serialized empty bundle is also valid.

Bounds are provisional explicit storage limits: 64 entries, names up to 127
bytes, 8 MiB total, 4 MiB per opaque blob, and 256 KiB per PNG. PNG dimensions are
bounded to 1024 each. These are codec capacity choices, not measured workload
requirements or a whole snapshot/resource footprint claim. The 4 MiB blob bound
admits a complete authored copy of actual retail `ItCo.dat` (2,949,760 bytes)
inside its 16-byte Item-blueprint envelope; the original 1 MiB provisional bound
did not. A shared named blueprint avoids repeating that bank for every recipe.

The unversioned canonical encoding is:

| Bytes | Value |
| --- | --- |
| 0â€“3 | `FMRS` magic |
| 4â€“7 | Little-endian entry count |
| 8â€“11 | Little-endian exact total size |
| Directory row: 0â€“127 | NUL-terminated logical name; remaining bytes zero |
| Directory row: 128â€“131 | Little-endian kind: 1 opaque blob, 2 PNG |
| Directory row: 132â€“135 | Little-endian payload offset |
| Directory row: 136â€“139 | Little-endian nonzero payload length |

Rows are strictly sorted by exact ASCII name. Payloads immediately follow the
directory, tightly concatenated in row order, with no padding, gaps, overlaps or
trailing bytes. `Write` sorts a fixed index array without modifying inputs.
Different input permutations produce identical bytes. No format-version field,
old-format bridge or per-entry native pointer is present.

Names permit ASCII letters/digits, underscores, hyphens, dots and forward-slash
segment separators. They reject absolute paths, empty/dot/parent segments,
backslashes, colons, spaces/control/non-ASCII bytes, trailing dots and reserved
Windows device stems. Lookup is case-sensitive; duplicate names and names
differing only in ASCII case cannot coexist. No case folding, path normalization
or basename substitution occurs at lookup.

Invalid reads preserve the previously accepted output view. Invalid measures
and writes preserve the output count/destination. Inputs, outputs and diagnostics
must have independent storage; overlapping output/input and diagnostic regions
are rejected before mutation. Views can be copied while their input remains
immutable and pinned; no view points into its own directory struct.

## PNG validation boundary

PNG validation checks the signature, exact framing and chunk CRCs, first unique
IHDR, legal color/bit-depth combinations, bounded dimensions, compression/filter/
interlace fields, palette/transparency rules, consecutive IDAT chunks, final IEND
and absence of unknown critical chunks or trailing bytes. Ancillary bytes and
opaque blobs remain exact. This does not inflate or validate compressed pixels.

Actual pixel preparation must still run the real decoder and validate the decoded
result before publication. The visual worker observed that WIC can report
successful CopyPixels with zero pixels for a CRC-correct `not-zlib` IDAT, so
decoder success alone is not a strict compressed-stream integrity proof either.
The codec deliberately admits structurally sound synthetic incomplete IDAT data
in one test to preserve this explicit boundary. No full PNG integrity, exact
rendering or gameplay acceptance follows from a structural receipt.

## Evidence

`A:/FreakMelee-test-archives/source-resource-codec-four-mib/receipt.json` passes
12,244 checks / 10 cases with the frozen compiler and `-Wall -Wextra -Werror`,
plus C++ header syntax. It covers all eight unchanged Rook PNGs, exact borrowed
spans, order-independent canonical output, every prefix truncation, hostile
directory/path/type/length/name-padding mutations, duplicate/case aliases,
CRC/header/chunk-order/palette/transparency failures, maximum directory and exact
8 MiB capacity, unchanged rejection outputs, overlap rejection and absent/empty
bundles. Child compilation/console processes use `CREATE_NO_WINDOW` and bounded
timeouts. That codec receipt does not execute package/project/runtime integration.

## Authoring commands

The structured resource operations share native authoring code with the Workshop.
See [Resource authoring](scripting/agent_resources.md) for input fields, examples,
limits, current format support and preview/build instructions.

- `resources.read` inspects names, hashes, images, article inventory and native limits.
- `resources.edit` imports or removes a batch through the draft lease, revision
  guard and recovery journal. The live `session.resources.edit` uses Workshop Undo.
- `resources.export` writes one exact owned resource to a new file.
- `resources.blueprint` creates a validated item blueprint from a supplied DAT and
  Article offset, or a named common-item choice from the player's ISO. Import the
  output as a blob. It copies the complete DAT and admits its model/command graph.
- `export-fighter-facts TEMPLATE OUTPUT.fma` exports the template's canonical
  numeric asset record for developer inspection.

The former direct-write resource commands were removed. Their callers now use
these structured operations. Stock-icon and HUD-emblem imports use the same
live resource edit path, retain Undo history and remain unsaved until Save.

## Historical command receipts

The receipts below describe the older direct-write command implementation.
Current resource acceptance is exercised by `tests/native/agent_resources_test.py`
and the real hidden/muted Workshop runner `agent_resources_live_test.py`.

`A:/FreakMelee-test-archives/source-resource-tool-cli-review/receipt.json` passes
92 checks / 10 cases with a private copy of the freshly rebuilt real tool/DLLs.
It clones actual Mario assets, inserts/replaces PNG/blob entries, preserves all
unrelated spans, rejects malformed/oversized/unsafe/case-alias inputs and corrupt
existing bundles, and exercises a real destination lock. Independent ISO/FST/
`itPublicData` parsing verifies the exact 2,949,760-byte original `ItCo.dat` and
Capsule descriptor offset against the stock-written envelope; the explicit DAT
route produces identical accepted bundle bytes. Full native blueprint graph,
decoded visual and source-recipe integration is separately tested by the parent
candidate/visual work. No game or user project/store is touched by this runner.

The updated full runner at
`A:/FreakMelee-test-archives/source-resource-tool-rook-facts-review/receipt.json`
passes 98 checks / 11 cases, repeating the complete resource transaction matrix
and two byte-identical actual Rook facts exports. Unknown-template failure leaves
the accepted record unchanged. Its verified `rook.fma` is retained alongside the
receipt for the parent private source-draft candidate proof; it is not a claim
that the complete Rook source/assets have activated in a game.
