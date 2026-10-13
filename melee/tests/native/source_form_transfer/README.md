# Direct source form transfer

The fixture executes the actual production `ftformtransfer.c` unit against the
immutable frozen66 shared transfer reference. Actual current Player role
lookup/swap, common destination reset, velocity reset and pickup notification
bodies are extracted without translating authored source behavior. Registered
fighter/item lifetime checks use actual `native_callback_lifetime.c`.

The source package, selected-bank/view/controller, activation-event endpoint,
presentation, camera, status and attachment operations are controlled leaves.
The source finish endpoint is an explicit `nativeScriptFighter_Run` boundary
with its typed hook/arguments checked; no native source finish pointer is passed
to the transfer function. Stock-only cases retain the actual stock finish
argument. This suite does not execute authored VM finish behavior, construct
actual actors/packages, publish a family, restore rollback or run a game.

The 1024 healthy comparisons cover every 512-way combination of transfer
statuses/items/ground-air for both stock and source branches. Common fields and
ordered effects match the frozen reference, including generation advance and
Player role permutation. Source cases additionally verify preserved separate
form-state bytes, destination view generation and one code-ID event dispatch.

The 625 lifetime cases inject registry removal with `PAGE_NOACCESS` userdata,
generation/kind/data/userdata replacement, program/bank/schema changes, logical
retirement, callback fault and recursive transfer at actual continuation points.
Transient dispatch/reentry state is checked by a fresh successful call after
each rejected continuation. Preflight cases reject mixed ownership, missing
activation hook, wrong family, busy destination, stale/faulted views and invalid
role/registration before any mutation. Typed source-item cases test their exact
Spec/view/status identity through healthy owner reassignment and error/deferred
retirement/program changes during transfer.

```powershell
python tests/native/source_form_transfer/run.py --reference A:/FreakMelee-test-archives/baseline66 --output A:/FreakMelee-test-archives/source-form-transfer-review
```

Use a fresh A: directory. Every subprocess uses `CREATE_NO_WINDOW`; source hashes,
commands and results are archived. The actual current reset includes the direct
source NewLife rebind, while the controller/view response is deliberately
controlled. The test verifies transfer continuity and ordering, not a complete
engine reset or paired gameplay acceptance.
