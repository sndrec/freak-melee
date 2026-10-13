# Retained Player construction and expected-object role clearing

This hidden fixture executes the complete current `Player_80031AD0` and
`Player_80031FB0` bodies and their constructor helpers, extracted unchanged.
It uses actual GObj and Fighter pools, actual source compilation/preparation/
program publication, actor attachment, storage/VM, immutable family lookup,
Player role getters/swapping, and the complete native CPU initializer.

`Fighter_Create` is an explicit controlled factory leaf. It allocates from the
real pools, attaches a real interpreted actor and executes native CPU prepare.
The primary must already be published when the secondary factory runs; the
secondary's actual typed hook and all 30 history records verify that ordering.
Native Fighter creation's graph/resource/load orchestration and full unload
are proved separately, rather than replaced by a successful constructor stub
and claimed here.

Single, alternate and simultaneous metadata are controlled transition inputs.
Simultaneous preparation/activation remains closed in production. The cohort
checks healthy construction, early primary failure, secondary failure, role
swapping, source faults/detachment, same-allocation NewLife, source schema changes,
character/mapping changes, userdata owner replacement, physical retirement/reuse
and callbacks publishing replacements. Failed pairs unwind secondary then
primary without entering death or Sleep policy. Cleanup uses physical pool
ownership even after semantic invalidation, while success requires the captured
actor/program/storage identity. Expected-object clearing follows the original
across role swaps and preserves replacements and transformed mapping; a surviving
actor in either physical role keeps the slot active, including single-member slots.

Factory, post-construction and destructor callbacks are explicit controlled
boundaries. Unselected native callbacks are unavailable. There is no claim of
actual simultaneous package activation, whole companion gameplay, eight-actor
scheduling, rendering or rollback acceptance.

```powershell
python tests/native/script_pair_constructor/run.py --compiler "C:/Program Files/LLVM/bin/clang.exe" --output A:/FreakMelee-test-archives/script-pair-constructor-reproduction
```

Use a fresh A: directory. The runner records exact extracted source, compiler,
header and fixture pins, hidden commands and a receipt, and rejects input drift.
The production constructor bodies receive a strict syntax check; only their
existing unused native stack-pad variables are excluded from that check.

The final receipt is recorded at
`A:/FreakMelee-test-archives/script-pair-constructor-oct10-final/receipt.json`:
5,805 checks across 66 cases, with 110 VM calls and 58 controlled callbacks.
Every history record is compared byte-for-byte against an independently prepared
expected record, including preserved untouched fields and native fallback pose.
