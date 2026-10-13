# Actual owned-resource authoring CLI

```powershell
python tests/native/source_resource_tool/run.py --tool build/native/native_content_tool.exe --iso '../Super Smash Bros. Melee (USA) (v1.02).iso' --output A:/FreakMelee-test-archives/source-resource-tool-new
```

The rebuilt tool must include the new resource command unit. The runner copies
the real executable/DLLs privately, creates a retail Mario asset draft in a fresh
A: directory and retains every console log/command. All child processes are
hidden and bounded; no game, global build or user content store is used.

Cases exercise PNG/blob insertion and exact replacement, unrelated entry
preservation, path/case/device/type/input/size rejection, malformed existing
bundle preservation, and a real Windows destination lock to reject atomic
replacement without leaking the owned temporary. They also independently read
the actual ISO FST / `itPublicData` common table, compare stock kind0's full
original `ItCo.dat` and selected Article offset against the written FMIB resource,
and compare explicit DAT authoring to stock authoring byte-for-byte. Invalid
offsets/kinds leave the complete accepted bundle unchanged.

This is actual authoring/codec/envelope evidence. Full native Item graph import
and recipe/visual publication belong to the parent source-candidate tests.

Acceptance: 92 checks / 10 cases at
`A:/FreakMelee-test-archives/source-resource-tool-cli-review/receipt.json`.
The original copied `ItCo.dat` is 2,949,760 bytes; its 16-byte FMIB envelope fits
the explicit 4 MiB blob bound. The complete bundle remains bounded to 8 MiB.

The updated exporter matrix passes 98 checks / 11 cases at
`A:/FreakMelee-test-archives/source-resource-tool-rook-facts-review/receipt.json`.
It additionally executes `export-fighter-facts rook OUTPUT.fma` twice, verifies
byte-identical canonical FMAS output, and checks unknown-template rejection
preserves accepted output. The verified `rook.fma` is in that archive.
