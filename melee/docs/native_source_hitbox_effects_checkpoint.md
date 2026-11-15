# Fighter hitbox controls and raw-part effects

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

This interpreter batch connects three reusable engine operations. It introduces
no fighter-specific API or callback delegate.

`hitbox.set_enabled(index, enabled)` changes the native capsule state without
clearing its victim history. Enabling also latches the normal Fighter transition
cleanup marker; disabling preserves it. This keeps the authored correction for
Ness's lingering yo-yo hitbox glitch. That marker correction intentionally differs
from the buggy stock delayed-rehit branch. `hitbox.clear_victims(index)` clears
both victim lists and their counters with `lbColl_80008440`, preserving each
entry's auxiliary word, capsule state, coordinates and all other fields.

Both methods retain integral index width. An out-of-range integral index is a
healthy no-op after source validation, matching the existing authored guarded
mutation contract. Wrong types, noncanonical Bool payloads, out-of-width Int/UInt
payloads and unavailable source ownership reject before mutation. The existing
state, damage and coordinate methods retain their stricter index contract.

`effects.spawn_on_part(effect, raw_part)` selects an exact raw slot in the owned
rig and runs the native synchronous `efSync_Spawn` call. It does not remap a
semantic bone role or use the deferred queue. Negative, out-of-range or absent
raw slots are healthy no-ops after live source/rig admission. Effect IDs retain
their integral width until exact UInt16 and one-joint native ABI validation.
The operation retains the selected source/program/storage/rig and native JObj
class around dispatch. Completed native side effects remain completed if the
source retires before result publication.

This is the canonical spelling of the previous raw one-joint operation. Direct
catalog, routing, examples and tests were changed; its old Feedback branch was
removed rather than forwarded. Semantic-bone and scalar-argument operations
remain distinct. Historical Feedback tests retain those distinct contracts;
the new Effect cohort owns canonical raw-part acceptance.

## Verification and remaining boundaries

The client, content tool and source-assets build passes. Eleven selected
allocator, compiler, Item and metadata CTests pass.

The connected hitbox cohort passes 281 checks / 112 cases with 42 native
comparisons, 54 VM helper calls and 55 negative cases. It executes the actual
frontend, preparation, Fighter actor, VM, catalog, backend and fixed GObj pool.
References include extracted capsule helpers, unchanged authored Sheik/Ness
helpers and the exact hitbox-cleanup slice of `Fighter_ChangeMotionState`.
Selected package/data and unrelated native leaves are controlled. It does not
claim execution of the entire motion transition, collision solver, renderer or
whole-match history. The unchanged state/damage regression passes 11,344 checks
/ 3,250 cases with 3,207 native comparisons. Root independently verified all
1,696 pinned inputs against the final receipt:
`A:/FreakMelee-test-archives/script-fighter-hitbox-controls-oct10-final3/receipt.json`,
SHA256 `35e7432eeccf2f5775a625c9ed55f83d7431912021846fb7a586d8ae8e91a852`.

The connected raw-part effect cohort passes 247 checks / 51 cases with fourteen
retirement mutations. It runs the actual source/statement/bytecode/VM path,
catalog resolution, source-layout guards, native pool and Effect backend.
Comparisons retain the unchanged raw-part reference, native `efSync` case
`0x4EC`, generator attachment/refcount body and class ancestry. Generator
allocation and selected package/program/actor views are controlled. It does
not establish the complete actor scheduler, paired package activation,
particle commands, drawing or gameplay. Root independently verified all 1,603
pinned inputs against the final receipt:
`A:/FreakMelee-test-archives/script-effect-on-part-oct09-final/receipt.json`,
SHA256 `7faddc9592b5dc7b13404349ae5c30e876be072775c32c310e87332187760eef`.

Existing Feedback/catalog fixtures were migrated directly and passed syntax
or AST checks; their full historical runners were not rerun. Existing hitbox
extractors gained actual native helper dependencies without changing their test
sources. The unchanged state/damage executable above uses the current closure.

The complete ten-source preparation census preserves all declarations. Mario,
Dr. Mario, Peach, Game & Watch and Zelda still prepare. Link and Young Link
remain stopped at mutable sword-trail callback data. Sheik now reaches
`hitbox.clear_all`; Popo and Nana reach `hitlag.active`. These are preparation
boundaries, not whole-fighter or gameplay acceptance.

Root verified all ten source hashes and the rebuilt tool hash against:
`A:/FreakMelee-test-archives/source-hitbox-effects-census-oct10-final/receipt.json`,
SHA256 `3b08e4fc356ceae9452472ea0d6a17f1b0aef9d6e5014fec95998aecf90e0d16`.
Next connect generic callback output records for sword trails, Fighter-wide
hitbox cleanup and the remaining linked-hitlag observations/operations. Public
resource animation application also remains unfinished.

No game window, renderer or audio was launched. The complete compact match
snapshot remains unfinished; Regions/Pages is still the current snapshot owner.
The full project goal stays active.
