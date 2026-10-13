# Ordinary source authoring and transport

Ordinary fighters use one unversioned `FMSC` packet schema. It carries Single
families or two independent members in AlternateForm / SimultaneousCompanion
families. Each member contains three DATs, an `FMAS` asset-facts record, nonempty FMS source, optional
typed parameter text and CPU profile data. The stable member key is an authoring /
cache label. There is no behavior code key, DLL chunk, generated C, or old packet
bridge. Runtime registration accepts Single and AlternateForm; both alternate
members must prepare their own `form_activated` event before atomic publication.
SimultaneousCompanion still rejects before member preparation. Immutable member
lookup, constructor history selection and safe companion retirement remain open;
two prepared full sources do not establish actual paired gameplay. Advanced handwritten
`module-probe` remains a separate loose-DLL diagnostic; public `script-c`,
`bundle-fighter`, `bundle-project` and obsolete paired composition are removed.
Historical C generation lives only in the explicit test reference exporter.

The packet header is 80 bytes with two fixed 60-byte member-directory rows; the
unused row must be zero. Each row contains the member key and seven lengths.
The source/facts chunks are required, records decode through the actual facts
codec, and text rejects embedded NUL. Parsing checks complete bounds/canonical
padding before publishing either output; rejected packets leave caller outputs
unchanged. DAT graph/FMS validation is the candidate layer's responsibility.

A draft owns `assets.fma`, copied DATs, `behavior.fms`, parameter/CPU text and a
small `project.txt` containing only title and an initial authoring template hint.
New stock asset drafts Copy/Write the donor's numeric facts once. Subsequent
ReadInfo/Open derives roots, actions, demo count, layout and rig from the owned
record, never from the template hint or a stock behavior definition. Importing a
source packet validates its actual candidate before creating a draft. Existing
paired draft directories are preserved. The family builder prepares each member
as a separate stable source owner and transports all three family modes;
building a companion packet does not enable its runtime activation. See the
[family authoring checkpoint](native_source_family_authoring_checkpoint.md) and
[alternate runtime checkpoint](native_source_form_runtime_checkpoint.md).

`source-check PROJECT SCRIPT` prepares a real candidate without editing files.
`script-build PROJECT SCRIPT` performs the same validation, then writes the
validated copied source through a flushed private temporary file and atomic
replacement of `behavior.fms`. A failed build leaves existing source and assets
unchanged. Offline preparation uses the actual custom validation slot; runtime
prepares again for its assigned slot. Diagnostics identify source line/column,
and success reports body/import/instruction counts. Neither command invokes a
native compiler or writes a module. `package-project` validates the full draft
before building/caching its source packet; `verify-package ID` validates copied
packet assets/code. Cache alone remains non-selectable.

Asset saves compare the owned profile with the saved record. Costume saves run
the real import against that profile before publication, rejecting changed rig
counts/roles or missing resources rather than silently recopying donor metadata.
An explicit profile-edit transaction is required for incompatible rig changes.
Compatible normal asset copying preserves FMS/state and uses matching decoded
profile/rig plus the existing archive, command-pointer and exclusive-clip checks;
it does not require scripts to be absent or transfer authored behavior.

Bounded codec evidence is
`A:/FreakMelee-test-archives/source-packet-second/receipt.json`:
9 cases / 1,267 checks, including every prefix truncation, unchanged failure
outputs, malformed fields, required facts/source, old format/DLL rejection and
the then-unsupported family rejection. That earlier transport receipt predates
the two-member authoring expansion. Its DATs are deliberately opaque transport bytes;
the actual-disc candidate and authoring transaction fixtures establish graph /
source preparation separately. No gameplay/rendering or paired-family parity
claim follows from packet validation.

The real CLI authoring receipt is
`A:/FreakMelee-test-archives/source-authoring-cli-oct07-root-second/receipt.json`:
6 cases / 15 checks. A freshly rebuilt tool clones actual retail Mario assets,
validates 77 bodies and 35 imports, preserves every draft file on rejected source,
rejects unsupported Workshop resources, and validates an unknown authoring hint
through the actual owned facts. Successful source save, source packet creation
and packet verification pass without a DLL or generated C. All console stages
run hidden; no game or real installation is changed. The initial archive remains
preserved: its manifest mutation accidentally introduced CRLF; the rerun changes
only the intended hint bytes using the canonical LF manifest.

The active installed-clone roundtrip now uses complete Mario source and the
source-only packet. It validates/cache-installs/exports in private A: stores,
removes its private author draft, then re-clones only the installed packet. Owned
facts, DATs, FMS, optional parameter text, CPU profile and portrait survive exactly.
The recipient edits DAT/CPU values and independently checks/builds/caches/installs
a second packet while the exported and original cached packets stay unchanged.
The actual hidden-tool receipt is
`A:/FreakMelee-test-archives/source-installed-clone-roundtrip-review/receipt.json`
(42 checks). No ISO argument is supplied to source verification, acceptance,
installed re-cloning or recipient publication.

The action-duplicate roundtrip retains its actual command insertion/deletion,
branch relocation, failed-edit preservation and unchanged neighboring-action
checks. It publishes complete validated Mario FMS before editing and verifies
both resulting source packets through `package-project` / `verify-package ID`.
Its receipt is
`A:/FreakMelee-test-archives/source-action-duplicate-roundtrip-review/receipt.json`
(47 hidden CLI commands, two source publications). Both scripts copy the real
tool/runtime DLLs privately, retain fresh A: logs/receipts and bound child runs;
their only installation markers belong to private fixture caches.

`fighter_script_bundle_roundtrip.py` and
`fighter_parameter_import_roundtrip.py` are historical DLL-era tool entrypoints,
unregistered from the current production-source CTest suite. Their helper files
remain because frozen historical/reference runners import them. Those entrypoints
require their historical tool/package protocol and are not acceptance commands
for the current source-only tool. Useful generated-C reference exporter/oracle
targets remain separate and unchanged.
