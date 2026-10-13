# Link / Young Link held tether source comparison

This unregistered candidate covers construction, held article state 0, the
fighter accessory2 and accessory3 updates, and teardown. It does not replace
the shipped Link examples or claim complete Catch/AirCatch behavior.

The fixture normally clones Link and Young Link from the retail ISO, imports
their parameters and article assets with the frozen content tool, compiles the
owned scripts into actual DLLs, binds their imported FMP parameters, and invokes
their generated callbacks. It extracts 33 unchanged original bodies from the
same frozen SDK. No stock Link solver or fighter decision handler is used by
the authored path.

## Compared source phases

| Original source in `src/melee/it/kinds/itlinkhookshot.c` | Authored responsibility |
| --- | --- |
| `it_link_attr_math`, `it_802A2568`, `it_802A2BA4` | Derive scaled tuning and integer count; construct nodes and alternating models; bind endpoint; pickup and scale order |
| `itLinkhookshot_UnkMotion0_Phys`, `fn_802A2E4C`, `it_802A6944`, `it_802A7AF0` | State-0 update selection, held attachment matrix, motion exit cleanup |
| `it_802A6A78`, `it_802A49B0`, `it_802A40D0`, `it_802A7B34` | Launch timing tests, reverse constraints, forward constraints and floor/island probes during hitlag |
| `it_802A6F80`, `it_802A7168` | Authored normalization, basis/cross products and model matrices |
| `it_802A2B10`, `it_802A7AAC` | Article removal, fighter handle and callback teardown |

The held fixture installs callback slots directly through the generic DSL:
`accessory_before` is accessory2, `accessory_hitlag` is accessory3. Existing
accessory4 remains distinct. Article-local typed helpers contain the reusable
solver computations; their calls are direct generated functions.

## Counts and limits

The actual `x2C` field is signed integer storage. Retail instruction evidence
is retained in `.local/link-tether-candidate/retail_count_receipt.json` and
`retail_count_instructions.txt`. Link's normal-scale air computation truncates
`15 * 1.5` to **22**, with endpoint index **21**. Young Link uses **15**, endpoint
**14**. The previous inferred 23-node claim was retracted; no float-count layout
repair or extra node is authored.

The candidate declares capacity 90 and reserved part 139. The generic resource
collection owns every constructed node and frees them by bounded index. The
tested scales are 0.5, 1, 1.25, 1.75 and 2.5, in both facings and motions 212,
214, 360 and 361. This is not a promise that arbitrary custom tuning fits the
90-node pool. Invalid count, exhausted pool or partial construction fails
explicitly and removes the incomplete root article.

The fighter launch timing fields at offsets 136, 152 and 168 are imported as
**int**, matching `ftLk_DatAttrs.x88/x98/xA8`. Timing comparisons include just
below, equal to, and just above the threshold.

## Scoped acceptance

Frozen27 normal imports and actual generated callbacks pass:

| Fighter | Assertions | Original source phase cases | Authored safety cases | Maximum compared float difference |
| --- | ---: | ---: | ---: | ---: |
| Link | 997,525 | 1,927 | 72 | 0 |
| Young Link | 712,807 | 1,927 | 51 | 0 |

Each fighter has 640 healthy constructor/normal/hitlag scenarios, followed by
teardown. Additional source cases cover invalid entry motions, held motion
exits and root-item allocation failure. Safety cases cover every GObj,
ItemLink and model allocation failure prefix; repeated destruction; newer
callback/part/hitbox bindings; removed or recycled creator generations;
changed damage owner; and zero vectors with no deployed nodes.

The linked production bodies are resource ownership/geometry operations,
generic joint channels, callback liveness, the parameter parser, DAT importer
and article model-root importer. Controlled boundaries are:

- Creator lookup and active Definition lookup.
- HSD GObj/ItemLink allocation, model construction and free operations.
- Matrix/vector math, including `lbVector_Normalize`.
- ECB initialization, sweep/floor responses and island lookup inputs.
- Root item creation, pickup/scale, removal and signal dispatch.

The harness compares constructor count and tuning, external endpoint binding,
state changes, node positions and collision history, deployment/floor state,
probe counts, model matrices/flags, render eligibility and complete freeing.
This establishes scoped agreement under those boundaries, rather than full
constructor-engine or gameplay acceptance.

Stock allocation-failure traversal and the model search with no deployed
nodes can access invalid state. Those branches are tested only on the authored
path. Authored teardown clears exact owned external-part borrowers and keeps
newer callback/binding replacements. The stock path can leave stale borrowers
or clear replacement callbacks; those safety changes are disclosed instead of
claiming raw-pointer parity.

## Reproduction and outstanding scope

From the repository root, with the retail ISO at the runner's default path:

```powershell
python -B tests/native/link_tether_held_stock_compare.py --snapshot .local/baseline27 --output .local/link-tether-held27-tracked
```

The runner supports explicit `--link`, `--young`, and `--iso` inputs. The
default tracked candidates are `tests/native/link_tether_held/link.fixture.fms`
and `young_link.fixture.fms`. They are exact copies of the accepted scratch
candidates and remain unregistered. Their exact
hashes, frozen manifest hash, imported FMP/DAT/owned scripts/generated C,
source inputs and commands are recorded in the output `acceptance.json`.
`--prepared` is a debugging option and is identified in the receipt as a
reused import.

Remaining work includes article states 1 through 8 and their transitions,
fighter Catch/AirCatch entry/reel/collision/capture/landing decisions, complete
damage/death cleanup with other articles, and gameplay/activation acceptance.
No game was launched for this batch. Raw pending proposals are preserved. The
tracked candidates retain their original preparation comments; the referenced
stage-island integration is available in the tested frozen SDK. The standalone
driver owns its extraction inventory and balanced extractor directly and does
not depend on the older untracked preparation driver.

## Frozen31 deployed retraction source slice

The new unregistered namespace `tests/native/link_tether_deployed/` contains two full readable draft sources, a self-contained extractor/import/compile runner and its console comparison fixture. It preserves the accepted held namespace unchanged. The remaining article states and AirCatch fighter phases now normal-import owned parameters/assets and compile as generated DLLs; successful compilation is not their behavioral acceptance.

The accepted slice executes the actual stock state4 callback `fn_802A33A0`/`it_802A5E28` and state5 callback `it_802A3500`/`it_802A678C`, with `it_802A44CC` and the original normal model update. It invokes original and generated physics first, then accessory2, preserving the deferred solver selection. It compares positions, velocities, collision history/environment, model matrices/flags, draw behavior, state/scale-call order and all-node removal. State4 covers incomplete retraction, completion returning to held state0, and invalid-motion removal. State5 covers incomplete retraction, completion without the final model update, and removal on the following update. Capture-retraction entry verifies the hitbox-clear call sequence; capsules start inactive, so this is not a new hitbox damage acceptance.

Each fighter uses 576 initial configurations: three scales (0.5, 1, 1.75), two facings, three first-deployed locations, eight controlled stage/island responses, valid/invalid creator motions and the two retraction modes. Up to four deferred updates run per configuration. At least one node is deployed; undefined original searches are not exercised.

Fresh frozen31 reproduction with tracked defaults:

```powershell
python -B tests/native/link_tether_deployed/source_compare.py --snapshot .local/baseline31 --output .local/link-tether-deployed31-tracked
```

The final receipt is `.local/link-tether-deployed31-tracked/acceptance.json`: Link 1,097,329 assertions / 1,264 compared source phases; Young Link 754,225 / 1,232; combined 1,851,554 assertions / 2,496 phases, maximum float difference 0. The runner verifies the frozen SDK/compiler/tool inputs before and after, hashes source and actual imported/generated inputs, and requires matching candidate/tool/snapshot/project hashes for `--prepared`. Optional `--link`/`--young` overrides remain available. No CMake build or game was launched.

Production resource, joint-channel, callback-lifetime and DAT model-root import bodies are linked. Creator/active-definition lookup, HSD graph allocation/model construction, matrix/vector math including normalization, stage/ECB responses, root creation/pickup/scale/removal/signals and the hitbox-clear boundary remain controlled explicitly. Required Host operations outside this slice trap if invoked. These are scoped source-phase comparisons, not complete constructor engine, capture or gameplay traces.

Scratch corrections required for successful code generation were ordinary DSL spelling (`facing.direction` and owned `attrs` sounds), moving declarations before action blocks, and splitting deeply nested air-launch logic into a readable fighter helper. Spool scale uses ordinary Float arithmetic because this language has no explicit double conversion; it is compiled but not executed in this slice and any future measured difference must be reported. No native tether solver or stock fighter delegate was added.

Remaining acceptance includes article states1/2/3/6/7/8 and spool animation, complete AirCatch360/361 input/entry/animation/physics/collision/reel decisions, standing/dash Catch integration, CatchPull/CatchWait notifications and whole fighter damage/death cleanup with the other authored articles. The full drafts remain unregistered, and this evidence cannot promote independent Link/Young Link defaults.

## Frozen32 AirCatch fighter phases

The separate `tests/native/link_aircatch/` namespace compares generated fighter
callbacks with extracted original AirCatch360/361 entry, input, animation,
physics and collision bodies, including the common AirCatch hit collision.
Fresh root reproduction passed 487,476 assertions over 8,730 source phases
across Link and Young Link, with maximum measured float difference zero.
Coverage includes 256 input configurations and 16 ordered timelines per
fighter, both facings, three scales, failed creation, obstruction, animation
completion, retraction, landing and hit bounce.

The candidates enter at original frame zero and retain the original
constructor-failure continuation into the animation-end check. The accepted
held and deployed fixtures remain unchanged. Article creation, signals,
solver execution, ECB responses and state transitions are controlled
boundaries; this does not accept full tether construction or capture gameplay.
The runner links the production callback-lifetime implementation and compiles
ordinary generated DLLs after fresh owned-parameter imports.

```powershell
python -B tests/native/link_aircatch/source_compare.py --snapshot .local/baseline32 --output .local/link-aircatch32-root
```

Exact inputs and hashes are in that output's `acceptance.json`. Remaining work
includes article states1/2/3/6/7/8, spool animation, standing/dash Catch,
CatchPull/CatchWait, composed cleanup and whole-fighter activation.

## Frozen36 remaining article states and spool

Fresh root reproduction of `tests/native/link_tether_remaining/` passes
244,184 graph comparisons and 113,372 spool comparisons across both fighters.
The matrix executes states1/2/3/6/7/8 with actual original bodies and generated
DLLs, in physics-selector then accessory2-solver order. It covers deployment,
surface contacts, obstruction rays, moving lines, reel/ledge decisions,
attack/ground exits and hang timers. The generic Bool-storage repair restores
the original A-button transitions. State2 correctly retains its stored surface
line instead of copying state1's wall-line update.

Maximum measured float difference is 1.91e-6. One reconstructed stack-spill
sqrt helper contains native C undefined behavior and is substituted explicitly
with deterministic sqrtf. Generic spool writes mark joints dirty, unlike the
original direct assignments; the fixture records those flag differences.
The README specifies both exceptions and all controlled engine boundaries.
This accepts the bounded article source slice, not console instruction
rounding, rendered behavior or complete fighter activation.

```powershell
python -B tests/native/link_tether_remaining/source_compare.py --snapshot .local/baseline36 --output .local/link-tether-remaining36-root
```

Ground standing/dash Catch, CatchPull/CatchWait notifications, composed
damage/death cleanup and whole-fighter gameplay/activation remain open.
