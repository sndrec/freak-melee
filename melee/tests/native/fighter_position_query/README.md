# Retained fighter position source query

Run `python tests/native/fighter_position_query/run.py --snapshot .local/baseline69
--output A:/FreakMelee-test-archives/fighter-position70-first` from the repository.
The output must be new. The immutable snapshot supplies only the verified clang
compiler; extracted actual current `nativeFighterTarget_Position` and
`ftLib_8008741C` bodies compile against live headers whose hashes are recorded.
The new function is not claimed to exist in SDK69.

The actual registry is a bounded live HSD fighter list in this console fixture.
Cases cover all32 ID bits, current position updates, zero/missing/stale IDs,
NULL registry/userdata/output, object/userdata address recycling, unlinked
PAGE_NOACCESS former objects, and byte-exact unchanged output on failure.
Camera and bone storage point at inaccessible pages throughout: this getter
must return `Fighter.cur_pos`, not camera or model geometry.

There are no mocked target lookups and no allocator/pose callbacks. Actor and
registry construction is controlled fixture setup. Generated source continuation
guards and typed vector output rules are separately tested in
`tests/native/script_queries`. This is console source evidence, not gameplay.

Scoped live-source run passes33 cases/66 checks at
`A:/FreakMelee-test-archives/fighter-position70-second/receipt.json`. The initial
runner stopped before compilation because its manifest lookup used a forward
slash key against Windows backslash keys; separator normalization fixes that
input check. No engine body needed correction. Both source bodies and relevant
live headers are hashed in the successful receipt; the verified SDK69 clang is
the only frozen executable used.
