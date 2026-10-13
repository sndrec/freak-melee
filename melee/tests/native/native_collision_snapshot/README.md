# Connected collision snapshot fixture

The runner freezes production sources/headers and freshly built libraries, then
links actual collision, island, dynamic-attribute and HSD graph algorithms. It
opens no game window. Supply a fresh output on A: and run in the VS compiler
environment with `--compiler <clang.exe> --output <directory>`.

The synthetic map has two adjoining static floors and a dynamic platform. Real
JObj matrix evaluation moves the platform; actual floor/speed/island/attribute
queries observe it. Hidden/disabled/too-distant alternate connections exercise
the native record-stride correction. Source descriptor links/coordinates,
joint membership, attributes/timers, bounding flags, query state and model
bindings are mutated, including island reclassification and model destruction.

Actual collision, ordinary class, ObjAlloc and OS metadata components validate
and restore together. Exact re-save and repeated mutation agree; collision
observations and borrowed graph/attribute references return. Refusal cases test
malformed boundaries/identities/links/ranges/output aliasing and same-epoch map
reload. Nonfinite runtime position bits are deliberately preserved for debugging.
Expired non-head attributes can be outside both roots in the original engine;
the fixture exercises and preserves those records instead of requiring a full
active/free partition.

The map/model binding is explicit fixture setup. It is not a real stage loader,
every callback/hazard, complete match rollback, GPU pixel or timing proof.
Candidate peer-graph closure is required before the full aggregate can safely
resume callbacks after restoring opaque island/model/query references.

`compare_query_migration.py` separately compares four complete gameplay logs:
original, isolated unfixed query, isolated corrected control and current client.
Supply those paths with `--original`, `--unfixed`, `--control`, `--current`,
`--samples` and a fresh `--output` JSON path. It requires the original/unfixed
pair and corrected/current pair to match exactly; it reports every changed
family and excludes no trace fields. It establishes attribution, not query
correctness or complete rollback acceptance. Build provenance is separate.
