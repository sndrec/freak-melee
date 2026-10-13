# Owned DAT pointer-field copying

`nativeFighterDatCopy_PointerField` is a construction-time operation for importing
one explicitly supplied DAT's complete local data into another owned DAT. It has
no fighter identity, archive discovery, gameplay donor lookup or public-symbol
linker. Both arguments identify serialized pointer slots relative to `DAT.data`.
Both slots must already resolve LOCAL through `HSD_DatResolve`; a relocated word
whose target is zero is LOCAL and is preserved as such.

Call with `output == NULL` to obtain validated `required_size`, allocate distinct
caller-owned storage, then call with that storage. Both inputs remain unchanged.
The operation builds and validates temporary output before publishing; any failure
leaves the output buffer unchanged. `required_size` and `stats` are independent
writable records and must not alias the byte buffers. Capacity failures after sizing
retain the required byte count. Results describe malformed inputs, ambiguous
references, nonlocal slots, capacity, overlapping storage, allocation or final
validation failures. Each input and output is capped at the content package limit
of 64 MiB. Construction uses temporary allocation; gameplay does not call this API.

The copier appends the whole source data at a 32-byte boundary because relocation
metadata cannot establish typeless allocation extents. It rebases every source
relocation slot and local target, and replaces the selected destination target.
All destination roots, reserved header bytes, existing data other than that word,
commands, scalar flags and blend fields remain unchanged. Source public roots are
not published. Repeating the operation with the same source appends another copy;
callers should group pointer ownership requirements deliberately during construction.

External symbols retain their original placeholder semantics. Source chains are
rebased and same-name chains are merged by linking the copied tail to the original
head; no original destination chain word changes. New names are added to the symbol
string table. Duplicate relocation slots, duplicate external names, overlapping
chains, relocation/external overlap and malformed/cyclic chains are rejected.
No external is automatically bound to another archive. The real importer retains
its existing NULL-at-import treatment of those external placeholders.

The caller must separately validate typed required resources with the actual
importer. A valid serialized DAT is not proof that an optional external supplies a
required joint. For the retail secondary Ice Climbers data, replacing the fighter
root's item-table field at+0x48 with the primary's corresponding local field copies
the complete three-article table. Its String endpoint graphs become local, while
the primary String model placeholder remains intentionally NULL. This operation
does not fill missing animation clips or establish behavior independence.

Console acceptance lives in `tests/native/fighter_dat_copy`. It compares complete
production imports and exact serialized relocation/external closure; it never
changes the importer or launches a game.
