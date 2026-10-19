# Costume import capacity

Custom costumes allocate their decoded model storage while loading. The loader
starts with 512 KiB and 2,048 descriptor records. If either fills, it discards the
incomplete graph, increases the exhausted capacity and imports again. Descriptor
capacity and byte capacity grow independently. The accepted graph stays in one
stable allocation until its owner releases it.

The maximum workspace is 64 MiB per costume member. This bounds admission of
malformed or excessively large models. The workspace contains decoded model
descriptors and writable envelope display lists; image and ordinary vertex
payloads continue to refer to the owned DAT. Retail scene imports retain their
existing caller-owned storage. Their largest decoded graph fits that storage.

Scripted-fighter construction, outfit validation/loading, painting, generated
model validation and costume inspection use the growing loader. Costume
compaction grows its tracing workspace by the same rules. A failed tracing
attempt discards the ranges it collected before retrying, so the final range
list describes one complete import.

The costume assets own this allocation directly. Source-package and outfit
owners release it when retiring the assets. A rejected load keeps its diagnostic
and clears all graph pointers into the discarded allocation. The original DAT
stays unchanged throughout loading and compaction failure.

All growth takes place during asset construction or an authoring transaction.
Fighter and item pools keep their existing capacities. The subsequent
[large-model work](native_large_gltf_checkpoint.md) lets the separate HSD scene
arena commit more pages during construction. Snapshots copy its initialized
prefix. A model with more runtime objects therefore adds real snapshot state;
the unused address reservation is excluded.

## Verification

The capacity cases live in `tests/native/costume_compact_test.cpp`. They exercise
both descriptor exhaustion and writable display-list exhaustion, then compact
and reload the accepted graphs. They also check bounded rejection, offset-zero
joint references, source immutability and invalid-input diagnostics.

`tests/native/fighter_costume_assets_test.c` compares the owned loader against
the existing caller-supplied import for all 133 retail costume archives.

Current receipts:

- `.local/native-costume-capacity-ctest-2.log`: both suites pass.
- `.local/native-costume-capacity-unit-2.log`: **3,108 checks** pass. The
  1,536-section fixture imports **3,076 records** using **438,448 bytes** in the
  initial 512 KiB workspace after expanding its record table. The long display
  fixture uses **2,163,000 bytes** in a 4 MiB workspace. A 33-section fixture
  whose writable displays exceed 64 MiB rejects cleanly and leaves compaction's
  input unchanged.
- The retail comparison covers **133 costume archives**. The largest retail
  graph uses **1,578 records** and **218,744 bytes**.
- `.local/native-costume-capacity-source-1/run.log`: **2,175 checks** across
  **26 cases** pass for owned scripted-fighter assets, package preparation and
  failure cleanup.
- `.local/native-gltf-capacity-1/receipt.json`: **409 native checks** and the
  saved-project import/reimport batch pass. The source draft remains unchanged.

`.local/native-costume-capacity-receipt.json` collects the result counts and
SHA-256 hashes of the reviewed game, content tool and test executables. The
game and all five verification targets build successfully. These are backend
checks; this change adds no new Workshop controls or live visual acceptance.

## Remaining model limits

This removes the fixed import-workspace rejection. Both the native weight
writer and [GLB/glTF importer](native_large_gltf_checkpoint.md) now use
[draw-section partitioning and stable vertex identities](native_large_skin_checkpoint.md).
Morph painting and the unusual binding domains listed in the Model plan remain
open.
