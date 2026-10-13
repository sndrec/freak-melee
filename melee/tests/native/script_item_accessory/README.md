This hidden console fixture prepares two Article programs using the real source
frontend and immutable Item metadata/program preparation. It attaches real Item
actors to the fixed GObj pool, executes typed source signals/helpers through the
actual VM/catalog/Item binding, and runs the actual accessory tick and state
reset/removal bodies extracted from Item source.

Declaring `on accessory` leaves the slot inactive. Tracking installs its owned
code ID and clears the native pointer; clearing removes both. Tests cover tick
execution, hitlag suppression, state reset/retracking, shared native assignment
superseding source tracking, packed snapshot restoration, and missing/mismatched
hooks without slot mutation. Source/program/storage/generation/class/epoch and
same-slot replacement negatives reject with output unchanged. Callback fault
and removal stop the real native tick tail.

The Article descriptor/common attributes, published prepared-program lookup,
graphics/effect/audio/physics leaves and PositionX fault/removal injection are
controlled. No DAT animation import, complete game constructor, rendered scene
or gameplay parity is claimed. Other self-operation leaves abort if selected.

Run `python tests/native/script_item_accessory/run.py --compiler
.local/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/NEW`.
The directory must be fresh; child consoles use CREATE_NO_WINDOW and bounded
120-second timeouts. Commands and input fingerprints accompany the receipt.

Final receipt:
`A:/FreakMelee-test-archives/script-item-accessory-final/receipt.json`.
It records 137 checks, 33 cases, 12 source VM signal invocations and two
actually prepared entity programs. The three accessory tail calls are counted
separately from legitimate nested destruction work when a callback removes its
Item. Primitive-only Item regression fixtures explicitly reject these actor
operations; their empty pool does not fabricate an accepted callback.
