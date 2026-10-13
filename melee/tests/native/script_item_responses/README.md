# Item response fixture

This connected console fixture runs `item.bounce()`,
`item.respond_stage_mask(mask)` and `item.attachment_owner_is(other)` through the
actual source frontend, compiler/catalog, published Item program, Item/Fighter
actors, Execution, VM, storage, receiver exports and fixed GObj pools.

Run from the repository root with a fresh output directory:

```powershell
python tests/native/script_item_responses/run.py --compiler .local/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-item-responses-final
```

The runner compiles and executes with `CREATE_NO_WINDOW`, enforces strict warnings
for the bindings and new test source, and checks every recorded input hash after
execution. `receipt.json` records commands, hashes and cohort counts;
`reference_source.json` pins the independent native response bodies to a Git
commit. No game or shared build runs.

The native comparisons execute the complete victim/map response bodies, vector
arithmetic, hitbox setter, owner-scale kernel and physical owner identity queries.
They cover 25 victim velocity combinations, 576 stage environment/previous-contact/
mask combinations, and two foreign/non-Fighter damage-owner cases. They compare
velocity, position, ECB state, hitbox damage and recorded side-effect order with
the native reference. Source callbacks also execute both responses and compare
attachment ownership against original creator and current damage owner, including
stock/foreign Fighters, null, a valid Item, revoked control and separate receiver
records for the same physical target.

Lifetime cases retain real pool generations and actor storage. Explicit test
observers after native kernels and inside the actual attachment getter provoke
retirement, fault, same-slot reuse, userdata/class/generation substitution,
model/parameter/common-data changes and damage-owner invalidation. They verify
failure leaves outputs unpublished and VM scratch balanced. A queued removal is
admitted while the owned view remains healthy; comparison rejects after the actual
Destroyed hook retires storage. These observers exercise continuation guards;
they do not claim the pure native kernels naturally invoke authored callbacks.
Compile/runtime negatives reject wrong contexts, argument types, malformed Object
tokens, noncanonical Bool masks and mask words wider than native 32 bits.

The fixture controls package/Article/model graphs, runtime package selection and
physical Fighter/Item userdata. Program preparation/publication, actor ownership,
receiver capture and VM execution remain actual production code. Geometry speed
and ECB setup, audio, effect creation and the stale-move factor are controlled,
recorded leaves. The hitbox damage setter and scale arithmetic are actual source
bodies, not successful placeholders. Unselected engine operations abort or reject.
This is not a DAT import/constructor, native callback-scheduler, rendering, gameplay
or complete match-snapshot acceptance test.
