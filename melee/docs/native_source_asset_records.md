# Owned source asset records

`native_source_asset_facts.h` defines data-only, owned asset facts; the separate
`native_source_assets.h` contains the imported engine graphs and arenas. This
keeps project/editor data headers usable in C++ without importing C-only engine
structure definitions. `Copy` is an
authoring operation when a draft begins; `Read`/`Import` consume the copied
record and its DATs without looking up donor behavior. The record contains
roots, counts, rig roles and insertions, audio dependencies, article declaration
names and slot mappings, pose graph paths and numeric motion data. It contains
no callback addresses, inherited behavior IDs or borrowed strings/arrays.

Projects persist these facts in `assets.fma`. The manifest contains a title and
an authoring template hint; runtime preparation does not resolve that hint to a
stock behavior definition. Model editing resolves the copied costume root from
the owned facts directly, and rejects a missing root instead of guessing another
joint graph. The ordinary Single fighter packet carries the same facts, copied
DATs, FMS, parameters and CPU profile. It carries no character DLL.

Motion data retains animation, flags and attack words. Camera selection is an
explicit None/standard-common-camera policy. Imported authored animation,
interrupt, physics and collision pointers are NULL; the program's code-ID maps
own those phases. Unsupported camera policies fail at authoring copy.

The `FMAS` record encodes fields individually in little-endian order with
bounded strings and arrays. It is not a native structure/pointer dump or an
old-format compatibility bridge. Validation rejects invalid counts, rig roles,
duplicate declaration names, pose paths, audio policies and motion records.
Separate declarations can share a physical asset slot. Physical kinds are
validated again after reading their optional decoded attribute field.

`nativeSourceAssets_Import` requires a vacant owner at its final stable address.
The character importer stores pointers into that owner's actual fields; moving
the struct would invalidate them. The owner copies its facts, reserves its
three import arenas and motion table at the load transition, and imports actual
fighter, animation and costume graphs. Semantic part maps, visibility, graph
targets and costume joint count validate before readiness. Native animation tags
for the owning rig are remapped to the assigned target slot; borrowed tags remain
unchanged. Failure frees only the candidate and restores it to vacant.

The caller pins the input DAT bytes until release. The import API itself does
not publish engine tables, infer behavioral inheritance or prove that a running
scene has stopped using an accepted owner.

The real-disc fixture clones Mario into a private directory and tests callback
independence, canonical round-trip, every truncated prefix, rejected writes with
unchanged outputs, source aliases, full import, self-reference integrity and
preservation of a separately accepted owner on failed replacement. Evidence:
`A:/FreakMelee-test-archives/source-assets-initial-root/receipt.json` (1499 checks,
15 cases). Its extended candidate proof uses the same actual DATs; see
`native_source_package_candidate.md`. No game/window/install is performed.
