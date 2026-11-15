# Interpreted point-to-floor-island queries

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

`stage.compare_floor_islands` now runs in the ordinary typed interpreter for
Fighter and Article sources. It uses `mpIsland_8005AC14` for the first vertical
world-position probe and, only when requested and the first island exists, the
second probe. `mpIsland_8005ACE8` reads the first island's current endpoint bounds.
Identity comparison uses the actual island, not matching line IDs or coordinates.
Neither gameplay contacts, allocation nor RNG are changed.

Four distinct writable destinations publish atomically after source/program/
storage/scene and geometry guards: Bool existence, Bool identity and two Float
X bounds. A missing first island publishes false/false/zero/zero. Input values
and requested probe endpoints must be finite. Yoshi's four output locals now use
their proper Bool/Float types instead of inferred integer destinations.

The native island routines remain unchanged. The reference bodies are extracted
independently at `762ce905a`, with only entry/inter-function names renamed.
The connected cohort exercises actual compiler/metadata/actor/VM/catalog and
the native query sequence. Twenty-four new reference pairs distinguish absent,
identical and different islands, optional second probes and dynamic endpoint
values. Six compiled-helper cases check evaluation once and state destinations;
compiler failures cover output aliases, types and non-writable arguments.
Explicit observer changes exercise source, schema and geometry publication guards.

The complete stage cohort passes **7,591 checks / 944 cases / 776 native pairs /
94 VM calls / 83 negative cases**. All 1,750 input hashes were independently
rechecked unchanged. Segment intersections, endpoint coordinate leaves, selected
package publication and observer mutations remain controlled. This is not a
complete moving-stage solver, DAT import, rendered geometry or Yoshi CPU/gameplay
acceptance. Geometry replacement during a point probe supplies another valid
map to the native routine; the fixture does not claim recovery from destroyed
native geometry midway through a read-only call.

Five native targets build: client, content tool, source family runtime, source
package runtime and capture enclosure. The read-only nine-source cohort still
prepares Captain Falcon, Ganondorf, Marth, Roy and Luigi. Yoshi now resolves the
new operation but stops at its old `cpu.target.position.x` operand on line 1536,
column 69. Selected CPU target access, catch-pull-frame/throw-victim hooks, owned
article integration and full fighter acceptance remain open. Fox/Falco asset/
metadata and DK effect diagnostics remain separate gaps; no source body was
removed to bypass them.

Evidence:

- `A:/FreakMelee-test-archives/script-island-compare-oct10-final2/receipt.json`,
  SHA256 `1dad8e622638f04ec5532d2aa66172751d4abdfad54d18086beadcf81d2c6e01`.
- `A:/FreakMelee-test-archives/source-island-compare-cohort-oct10a/receipt.json`,
  SHA256 `effdfe5ad02076f240bb8b91a834229a1602cd18bef95c4fb7bb7c37baa5f5f1`.
- `.local/enclosure-build.log`: five-target build exited successfully.

No game launch or installed asset change occurred. Compact whole-match rollback
remains unfinished; the overall project goal remains active.
