# Native item collision masks

Melee's original `src/MSL/stdbool.h` defines `bool` as `int`. Several item
collision routines store contact bitmasks in variables declared `bool`.
Native C's `_Bool` converts those masks to zero or one, removing wall and
ceiling bits before the routines test them or pass them to collision responses.
This is a porting mismatch, rather than an intentional change to stock physics.

The affected mask locals now use `s32`, named `collision_flags`. The private
`it_8026E_inline` helper also returns `s32`; public predicate signatures and
actual boolean landing decisions remain unchanged. The correction covers
`it_8026DFB0`, `it_8026E058`, the E15C/E248/E32C helper closure, E414/E4D0/E5A0/
E664/E71C/E7E0 and Peach turnip's third collision phase. It preserves original
query order, mask combinations, bounce arguments, position/line updates,
state changes and landing callback order.

[The source extractor](../tests/native/collision_mask_stock_compare.py) compares
21 actual bodies with the original commit `500911adf`, changing only the
original boolean type to its MSL integer meaning and renaming test functions.
[The fixture](../tests/native/collision_mask_stock_test.c) passes 45,080 original
versus candidate cases, plus explicit wall-contact continuation assertions.
Contacts and landing predicates are controlled inputs; all resulting engine
calls, response arguments and relevant state writes are compared. Twelve retail
function byte hashes, totaling 2,324 bytes, were also verified against the ISO.

The retained pre-fix run fails with ceiling mask 2 at E15C: native code skips
the original collision response. Evidence and source hashes are in
`.local/peach-closure-next-batch/collision-mask-fixture` and the rejected run in
`collision-mask-before-fixture`. The CMake test is `native_item_collision_masks`.
This establishes the corrected mask dataflow; it does not claim whole Peach
clone or rendered item acceptance.
