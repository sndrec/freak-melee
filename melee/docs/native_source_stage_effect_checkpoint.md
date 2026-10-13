# Interpreted stage queries and shared effect contracts

This batch connects generic engine operations needed by the existing fighter
translations. Packages still execute interpreted bytecode. It adds no stock
fighter delegates, fighter-specific methods or compatibility aliases.

## Runtime changes

`effects.spawn_between(effect, first_part, second_part)` admits the four audited
native two-joint signatures. Both raw rig slots and the creator's scale root are
retained around actual synchronous `efSync_Spawn`. Missing raw slots are healthy
no-ops. Native construction and initial animation evaluation preserve their
order; a missing second allocation leaves the first effect intact. Completed
native effects are not undone if a later callback retires the caller.
See [two-joint effects](scripting/effect_between.md).

`stage.sweep_box(...)` returns raw signed environment flags from an independent
native ECB probe. `stage.segment_blocked(...)` returns the remapped segment test
Boolean for Fighter or Article sources. Geometry must be finite; sweep admission
checks native subdivision arithmetic and enabled debug bounds. Native callbacks
can change the stage; results are published only while the selected source,
program, storage and physical identity remain current. Existing global collision
scratch remains serialized. Low-nibble categories 1/2/3 require real actors in
stock stage callbacks and reject on an independent probe; no actor is fabricated
and no stock stage callback is modified. See [stage queries](scripting/stage_queries.md).

Effect pause, resume, destroy and clear operations now retain physical identity
and the exact owned schema when an interpreted actor is attached. Installing
the shared hitlag callbacks clears both saved authored callback IDs. Authored
pre/post callbacks can replace them through the existing actor API. Popo and
Nana directly clear those two slots instead of using the obsolete clear_hitlag
spelling. See [effect hitlag controls](scripting/effect_hitlag.md).

These operation paths use bounded stack state and allocate no script-side tick
memory. Native effect construction keeps its existing allocation behavior.

## Verification

The actual client, content tool and source-assets build passes. Eleven allocator,
Item/compiler/metadata/move-entry CTests pass, as does the existing combat CTest.
All console processes were hidden; no game, GPU or audio process was launched.

The two-joint cohort passes 66,433 checks / 76 cases, including 24 comparisons
with unchanged native bodies and 14 retirement mutations. Actual frontend,
statement compiler, VM, binding, attachment bodies and LIFO animation drain are
connected. Selected source publication, effect allocation and animation
evaluation leaves are controlled. All 1,604 pinned inputs were independently
verified. Receipt: `A:/FreakMelee-test-archives/script-effect-between-oct10-final2/receipt.json`;
SHA256 `1b7e02d4238cdec548ef38b51565c32d9c731b5b0781a817a327cf178f3a779b`.

The stage-query cohort passes 2,062 checks / 128 cases, including 86 native
comparisons, ten VM calls and 38 negative cases. Actual frontend, preparation,
Fighter/Item actors, VM and fixed pools execute the mechanically extracted native
initializer, ECB, subdivision, bounding, callback and segment-wrapper bodies.
Collision-line resolution, stage callback registration and selected package data
are controlled. Twelve signed/full-width aliases for actor-contact categories
reject before native work. The current registered callback sources were audited
and pinned; this does not establish full collision-solver or gameplay acceptance.
All 1,717 inputs were independently verified. Receipt:
`A:/FreakMelee-test-archives/script-stage-queries-oct10-final4/receipt.json`;
SHA256 `346f4b3658cee3b194cc6b3fabe3388b7da3f69f5ff382b7f847681c7295c407`.

The effect-hitlag cohort passes 380 checks / 35 cases, with 16 native comparisons
and 32 VM helper calls. It uses actual actors, saved callback IDs, recipient
dispatch and native effect loops on both lists. Selected package data, effect
records, scheduling and model-effect cancellation leaves are controlled. All
1,689 inputs were verified. Receipt:
`A:/FreakMelee-test-archives/script-effect-hitlag-oct09-final/receipt.json`;
SHA256 `d621d40058dc5b99632e6cf9000750f3f0a4157f6b83e4a75bc328edf2531bb3`.

Existing combat passes 37,048 checks, including 12,240 RNG comparisons and 30
native-body comparisons. Its physical Fighter now uses an actual fixed MatchPool;
unattached hitlag callback installation rejects. All 1,662 inputs were verified.
Receipt: `A:/FreakMelee-test-archives/script-combat-effect-pool-oct09-final/receipt.json`;
SHA256 `0af125da00a9b6d2b529e00ff8c40a38b7574b90fd9cc3f00a512ab5d2cd259f`.

The complete 65,536-ID signature audit passes 262,347 checks. Existing raw-part
and form-feedback regressions pass 247 and 67,412 checks respectively. Their
controlled boundaries remain unchanged; unexercised branches reject or abort.
Receipts: `script-position-twojoint-abi-oct10-final2`,
`script-effect-part-pair-regression-oct10-final4`, and
`script-form-pair-regression-oct10-final4` under `A:/FreakMelee-test-archives`.
Their 1,579 / 1,605 / 1,587 inputs were independently verified.

## Preparation boundaries and remaining work

The rebuilt content tool checks ten retained translation sources against private
copied drafts. Mario, Dr. Mario, Peach, Game & Watch and Zelda prepare.
Correction from the next source audit: the exact first diagnostics were
Link/Young Link's obsolete `cue.consume(0)` spelling and Sheik's root parameter
`attrs.needle_held_part` referenced from an article callback. The adjacent
`bones.world_position` and `item.change_state_flags` calls already had bindings.
Popo and Nana stopped at `family.member_id`. Unsupported bodies remain present.
Receipt: `A:/FreakMelee-test-archives/source-stage-pair-census-oct10-final2/receipt.json`;
SHA256 `144d8717b98cdc229e9323f3827595fb482b4e0359bf00e20a6f05b40b3264ee`.
Preparation does not establish complete fighter behavior.

Remaining work includes subsequent missing shared bindings, public resource
animation application, whole-fighter behavioral acceptance and compact
whole-match save/load. Regions/Pages still owns broad snapshots. The full
project goal remains active.
