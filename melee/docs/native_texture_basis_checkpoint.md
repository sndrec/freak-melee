# Texture normal basis: backend checkpoint

2026-10-19. Work is checkpointed while development returns to the hub UI at the
user's request. This record does not mark the Model milestone complete.

The skin reader/writer now retains independent texture normal bases when
canonicalizing legacy rigid shortcuts. Fixed texture attachments carry a 3x3
post-transform. An identity texture can retain its original normal input without
skeleton or camera motion, using an empty envelope and an explicit constant bit.

The native separate-table descriptor is 24 bytes: position and texture table
pointers, ten follower markers, a 16-bit constant mask, and an optional pointer
to ten 3x3 normal bases. The typed importer and compactor trace this data. The
runtime retains its immutable basis pointer and applies the correction during
matrix setup. No tick allocation or pool-capacity increase was added.

Verified backend evidence:

- 70 rigid cases / 576,147 checks; maximum position error 1.43051e-6 and normal
  error 2.38419e-7. New cases cover independent normal spaces with shear and
  nonuniform scale, identity matrices, repeated painting, reopen and compaction.
- 450 morph cases / 5,837,906 checks pass.
- All 130 retail costumes / 8,149 meshes / 227 brushes / 123,896,453 checks pass.

Receipts: `.local/native-texture-bases-tests-2.log`,
`.local/native-texture-bases-focused-results.log`,
`.local/native-texture-bases-retail-1/receipt.json`.
The first focused run stopped on a fixture's brush-count assumption: two source
vertices in different rigid domains occupy different canonical positions.
The corrected fixture expects one affected vertex for that case.

Remaining verification includes the native reflected-texture visual run with
`review_native_texture_skin_live.py --distinct-basis`, and rest-edit compensation
of these texture bases. The latter path currently changes vertex normals through
per-position adjustments and needs its own texture-basis handling. Unusual
helper model-node domains and the existing matrix limits remain open.
