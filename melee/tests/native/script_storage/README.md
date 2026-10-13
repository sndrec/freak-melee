# Caller-owned typed declaration storage

`native_script_storage.h/.c` consumes the actual `NativeScriptProgramIndex`.
Layout construction uses declaration indices, exact source owners and the
explicit array flag, including arrays of length one. State fields use indexed
script byte offsets, with one byte for Bool and four bytes for Int32/Float32.
Alignment, overlap, range, type and owner errors fail while loading. Runtime
access never resolves names or reads DAT import offsets.

Every indexed Article, Item and model-effect declaration receives its own owner
row even with no state, parameters, helper or callback. Empty owners have zero
state bytes and parameter slots; NULL-buffer instances/snapshots remain checked
by exact owner and lifetime. Exhausting the caller's owner capacity fails loading
with no published layout identity or partial owner/field counts.

The caller supplies layout arrays, parameter slots, per-instance state bytes,
snapshot bytes and VM register/frame pools. Construction happens only while
loading/applying content. Completed program/layout/parameter tables remain
immutable while instances execute. Read/write/create/save/restore allocate no
memory.

Parameters use typed declaration defaults or explicitly supplied typed values.
Imported `attribute byte` metadata has a U32 logical parameter type; immutable
slots preserve all 32 bits. This does not introduce U32 mutable state fields.
Supplies identify the declaration semantic ID and exact owner; duplicates,
wrong types, wrong owners and invalid Int32 widths fail before changing output
slots. Defaults remain resolved, while imported attributes remain explicitly
unresolved until provided. Missing imports cannot silently read as zero.
`ParametersReady` is the gate a future content loader must pass before applying
an owner; storage construction itself can represent a still-unresolved table.
There is no runtime parameter-write operation.

Instances bind an immutable layout, parameter table, owner, identity and full
generation to caller-provided bytes. Creation initializes only the active owner
footprint, including canonical padding; unused caller capacity remains untouched.
Scalar reads use no selectors. Arrays require one integral selector; Bool maps
to 0/1, signed negatives and all out-of-range indices fail, and unsigned high
bits are retained during bounds checks. Float selectors fail. Writes use checked
language conversions; Bool storage normalizes truth and Int32 retains all bits.

An ephemeral snapshot consists of the active state bytes and owner/instance/
generation/layout/immutable-parameter identities. Immutable parameters and code
are shared rather than copied. Restore rejects a different owner, reused
instance generation, changed layout/configuration or malformed byte extent before
touching state. This utility does not recreate native fighters/articles or restore
their lifetimes. Future aggregate rollback must restore/rebind native identities
coherently before restoring the matching live storage instances. The snapshot has
no format upgrade machinery or per-scalar version tags.

The console fixture parses and indexes `probe.fms`, then executes its real typed
state/parameter semantic IDs through the production bytecode VM. It exercises
Bool/Int32/Float32 declarations, all-32-bit handles, arrays, array[1], exact owner
rejection, imported attribute failure/provision, readonly parameter storage,
create/retire/reuse and snapshot checks. A restored instance reproduces the exact
owned-byte endpoint after 64 further VM ticks, including Float arithmetic before
Double literal promotion and Float storage. Native field/module integration and
whole-game rollback remain outside this fixture.

The empty-owner regression also parses/indexes three metadata-only entities,
prepares zero-slot immutable parameters and checks zero-byte snapshot restoration
and cross-owner rejection. The strict suite passes754 checks and128 VM ticks.

```powershell
python tests/native/script_storage/run.py `
  --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe `
  --output A:/FreakMelee-test-archives/script-storage-example
```

All outputs remain on `A:` and subprocesses run with `CREATE_NO_WINDOW`.
