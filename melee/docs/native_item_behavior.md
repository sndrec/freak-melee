# Engine and authored item construction

The common item allocator now receives an explicit behavior selection separately
from its package author. Asset and callback selection run before model construction
and instance callbacks, in `src/melee/it/native_item_behavior.c`.

| Constructor | Selection | Result |
| --- | --- | --- |
| `Item_CreateEngine` | Engine | Loaded ordinary common-item assets and native callbacks; parent declarations cannot replace either. |
| `Item_80268B18`, `Item_80268B5C`, `Item_80268B9C` | Parent | Existing airborne/grounded constructor semantics; a matching parent package can supply assets and article logic. |
| `Item_CreateAuthored` | Author | Explicit package assets and article logic, independently of the current damage owner. |

The engine entry validates the common-item range, loaded article, common
attributes and model descriptor before modifying the request or allocating an
instance. It then performs the actual airborne/hold-kind initialization and
enters the common allocator directly. It does not forward through the
parent-selected constructor. Original parent pointers, positions, velocity,
facing, collision group and damage remain caller-owned inputs.

Ordinary native callbacks retain their private item storage. Engine selection
never queries the parent's package or clears that storage. Existing parent and
explicit-author selection bodies were moved directly into the new source file;
their behavior is unchanged. The render callback remains native for engine
items, and authored standard rendering still applies only when requested by an
authored definition.

## Acceptance and limits

`native_item_behavior` links the actual selector and extracts all five actual
constructor entry points. Its 125 checks cover 24 combinations of parent asset
and logic availability, render policy and parent category. The parent/author
results are compared byte-for-byte with the previous selector from local commit
`1f2df74d6`, including render callback identity. Additional checks cover engine
selection against conflicting parent declarations, complete spawn request
preservation, missing data rejection before writes and allocation failure.

Allocation, model construction and hold-kind classification are controlled
fixture boundaries. The full native build and 40 selected compiler/engine tests
pass; no gameplay launch was performed for this change.

This is constructor support, **not yet a scripted ordinary-item feature**.
Generic retained creator/handle ownership, attachment, held binding and remembered
throw-scale operations still need integration before Peach's rare-item pulls can
be promoted to authored behavior. No Host ABI or Item layout changed, and no
fighter package was reinstalled for this checkpoint.
