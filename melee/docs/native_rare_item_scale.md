# Rare held-item saved scale

Peach's native turnip constructor can also create a Bob-omb, Mr. Saturn or
Beam Sword. For non-sword items, it attaches the item and applies the owner's
scale before saving the scale used by later item callbacks.

The decompiled constructor wrote Bob-omb's saved scale through the turnip
union member. On GameCube those fields overlap at item offset `0xDE8`. On
64-bit PC, a preceding owner pointer expands the turnip member: its relative
offset becomes 24, while Bob-omb's actual scale remains at relative offset 20.
The constructor therefore changed the wrong field, and the ordinary Bob-omb
throw callback retained the old saved scale.

The constructor now writes `bombhei.xDE8` for Bob-omb and `dosei.xDE0` for
Mr. Saturn. Turnip uses its own member. Item selection, attachment, owner-scale
application and subsequent callback order are unchanged. This repairs the
native representation; it introduces no fighter-specific scripting operation.

`tests/native/peach_rare_scale_stock_compare.py` extracts the actual constructor
and ordinary Bob-omb/Mr. Saturn throw bodies. Its console-data-flow reference
adjusts only the two union field identities, justified by verified retail
PowerPC stores at `0x802BD5F4`, `0x802BD600` and `0x802BD60C`. It retains the old
native constructor as a negative control. The test does not present that
adjusted reference as an unmodified 64-bit decompilation.

The fixture passes 153 constructor/throw pairs and 6,456 checks across item and
owner scales, Bob-omb armed states, Mr. Saturn reentry, sword exclusion and
allocation failure. The old implementation still exhibits 22 of 25 Bob-omb
saved-scale mismatches. CTest registers this as `native_peach_rare_scale`.
Evidence is in `.local/peach-rare-scale-repair` and the generated build fixture.

Shared item attachment, scaling and presentation calls are controlled test
boundaries. This is a saved-scale and callback-order contract, not whole Peach
gameplay acceptance or an authored ordinary-item API.
