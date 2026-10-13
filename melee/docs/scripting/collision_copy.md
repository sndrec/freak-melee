# Collision history copy

`collision.copy_from(id) -> bool` is a Fighter-only operation. The argument must
be an Int expression representable as native signed 32-bit Int. High-bit creation
IDs may appear negative; passing the Int preserves all 32 bits. Bool and Float
arguments require an explicit `int()` conversion.

The resolved target supplies the collision history and the caller receives it:
`mpCopyCollData(&target->coll_data, &caller->coll_data, 2)`. Self-copy succeeds
without changing bytes. Zero or an absent creation ID returns false and leaves
the caller unchanged. The target may be a stock or foreign Fighter; this operation
does not require family membership or select any fighter-specific behavior.

Mode 2 copies current, previous and last collision positions; selected `x34`
flags; collision facing, skip/ledge IDs and stick X; all five ECB records;
environment/contact flags and contact vector; and floor, both wall and ceiling
index/flags/normal records. The ECB records are copied unconditionally even
though the native body also contains an earlier mode-1 block.

The caller retains its collision-owner pointer, auxiliary vector and flags,
joint-only selector, `x50`, ledge-snap controls and ECB source geometry/joints.
No other Fighter state is copied. Position, velocity, facing and ground/air
classification outside collision history remain separate authored operations.
The kernel performs field copies only: it invokes no callback, allocates nothing,
and reads no stage geometry. It imposes no finite-float or stage-index admission
rule on the copied history.

The interpreter retains the current physical caller and its exact owned actor,
program, storage and motion layout. It bounds traversal of the native Fighter
registry and admits physical scene identity before reading a target. It retains
the target generation, userdata and current native definition or owned
program/storage, then checks both lifetimes before copying and before publishing
the Bool result. A malformed matched target faults the operation. A failed
post-call guard does not undo a completed collision copy.

Like the native registry and shared lifetime readers, this binding assumes a
live Fighter GObj's non-null userdata is a valid engine-owned Fighter allocation.
The production constructor supplies the native Fighter-pool actor through
`GObj_InitUserData`; removal invokes its destructor and clears the userdata.
Physical GObj admission alone does not establish the extent or provenance of
an arbitrary non-null userdata address. The controlled fixture supplies valid
Fighter storage and does not claim to contain corrupted userdata pointers.

The retained Popo/Nana sources use this operation before ground recruitment,
during collision following and before Belay launch. The binding supplies only
the generic history operation. Simultaneous-companion activation remains a
separate runtime requirement described in [Family queries](family_queries.md).

`tests/native/script_collision_copy` executes the unchanged native kernel and
legacy relation/registry oracle alongside the actual compiler, Prepare/Publish,
Fighter actors, VM and fixed GObj pool. Imported source assets are controlled.
It verifies copied and retained fields, full-width IDs, stock/owned targets,
self/missing targets and prior source/target lifetime failures. It launches no
game and does not claim stage solving or whole paired-character behavior.
