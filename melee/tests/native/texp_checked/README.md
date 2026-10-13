This private console fixture compares the complete native TEV compiler against
the pre-edit Git compiler on 128 healthy expression graphs. Both Abort and
Checked outputs must preserve stage descriptors and assigned constant registers.
It exercises cycles, the 512-expression transition scratch bound, the native
32-node DAG bound, actual register-packing failures and partial descriptor
allocation exhaustion, with unchanged descriptor output and complete scratch
cleanup. Checked mode mutates scratch expressions; it does not restore them.

The allocator is controlled calloc/free, and descriptor failures are injected
after construction. The existing HSD reservation fixture separately tests the
real native allocator and 341 actual material-builder shader combinations. This
fixture does not prove imported model construction, rendering or game behavior.

`material_reference.py` builds separate pre-edit and current native MObj/TObj/TEV
compilers and runs the existing HSD reservation fixture operations unchanged.
All 341 cases, including four actual imported Arrow roots and shader recompiles,
must match checks and a numerical fingerprint. The fingerprint includes every
assigned TEV configuration field, stage/flags/channel, texture resource ID and
constant register/index/ref metadata. It excludes process addresses, padding and
texture coord/map fields that are not assigned until rendering setup.

Checked preflight uses fixed visitation storage for at most 512 scratch nodes,
then the actual native optimizer and scheduler. Shared children are visited once
in preflight. Abort mode skips this extra transition admission pass; native stage
and register bounds are still enforced. Allocation-list pointers are trusted
native objects, not an arbitrary memory-safety parser. Expression constructors
retain their native assertion behavior and require prior allocation admission.
