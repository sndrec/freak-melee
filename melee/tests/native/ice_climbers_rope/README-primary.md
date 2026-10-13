# Primary Belay, constructor and ordered cleanup source proof

This extends the historical scalar/renderer checkpoint in README.md. It is an unregistered source candidate, not full fighter or paired-package activation. Accepted Ice/Blizzard and Squall/secondary Belay inputs are read unchanged. All comparisons use frozen SDK68 for the real generated callbacks and immutable SDK66 for mechanically extracted original complete bodies. There are no stock fighter-specific delegates in the candidate.

The final candidate includes two reviewed corrections to primary.fms: the launch explicitly selects air motion354 after forcing air, matching the source, and a captured creation handle is checked with the existing article reference query before binding. Resource-allocation failure can retire an authored article inside its created callback while returning its captured ID; an expired result is published as -1, the auxiliary slot is cleared, and the original Wait failure path is taken. Prior receipts are preserved as historical inputs, including the original349 mismatch and failed intermediate fixture runs.

## Final receipts

Outputs are under A:/FreakMelee-test-archives. Each receipt contains exact commands, source hashes, old complete-body provenance, generated C/DLL or executable, retail DAT/FMP and logs.

| Gate | Pairs | Checks | Receipt relative to archive | Receipt SHA256 |
| --- | ---: | ---: | --- | --- |
| Primary phases | 6,432 | 483,189 | `ice-primary68-final/result.json` | `addb173c35687e0f1e1015f9bcedd7036f94e4c119f2ee0838a53a65549e5b84` |
| Constructor/lifecycle | 1,500 | 2,930,969 | `ice-constructor68-final2/result.json` | `08181bf5753d05ea36c3e2ebd1333425b66e474776fd195f2214ccec52f37ada` |
| Scalar solver rerun | 11,360 | 5,069,197 | `ice-rope68-solver-final2/result.json` | `9c4f5e9e954e245d06ccf81ed886cf64a86114c5f865334b44ee5b5dd7abeb4b` |
| Renderer rerun | 4,544 | 2,869,249 | `ice-rope68-render-final/result.json` | `c66bd5679da035c9f92f1d0a293db15cb17935edc1ebb2738a064de1186838ea` |

Combined: 23,836 original/generated pairs and 11,352,604 checks. Primary additionally checks two source removal/reuse cases; constructor/lifecycle checks 124 authored safety cases. Solver and renderer maximum Float difference is0. The unchanged solver and renderer runners were rerun against the final candidate; their receipt scope strings retain their original standalone gate wording.

## Coverage and controls

primary_run.py compares both primary entries and all four phases for347/352,348/353 and349/354. It covers recruitment success/failure, companion absent/present, commands, animation completion, ground loss/landing/cliff and ceiling/wall priorities, endpoint publication, velocity and callback order. The counter is deliberately below creation for the phase matrix; constructor_run.py exercises the actual creation and full scheduler separately. An additional256 launch vectors vary distances, facing and airborne state; the original double-refined my_sqrtf and scripted sqrt produce exact compared velocities for this bounded matrix.

constructor_run.py compares64 constructor/Item allocation-failure pairs,600 scheduler pairs (9..83),44 owner/expiry pairs,768 accessory solver/render pairs,8 interactions across all four states and16 complete cleanup pairs. The cleanup pairs invoke the actual original Init_8011F060 and generated installed damage callback, covering Ice removal -> Blizzard effect cleanup -> rope removal -> root rotation/hitlag release/unlink. Ice/Blizzard presence and linked/active hitlag vary independently. All defined link positions/velocities, active flags, model matrices/translation/flags and freed-node identities are checked. Extend compares the defined tail collision current/previous history; original uninitialized non-tail collision bytes are not interpreted as behavior.

The executable links complete native_article_resource, native_article_refs, native_callback_lifetime and native_fighter_relations units. It extracts complete nativeArticle_Create/Creator, signal dispatcher, original rope constructor/resource setup/AB54, article states/accessories/cleanup, shared solver/render helpers and original fighter cleanup. Controlled boundaries are Item allocation, HSD/link allocation and model materialization, affine operations, stage probes, common motion/animation publication, effect destruction and Item removal. AB54 itself is actual; its immediate it_80273168/it_802742F4/Item_8026B074 dependencies are observed controls, so this does not claim full held-item geometry/bookkeeping/debug semantics. Item removal invokes the actual original or generated destroyed callback through a controlled reentrancy/registry boundary; complete Item removal engine behavior is not re-proved here. Interactions control it_8026B894's internal result to false while comparing the complete caller's saved-creator decision.

The120 node/link/model prefix-failure checks are an explicit authored safety policy, separate from healthy retail pairs. The original allocator uses uninitialized data on first failure and stale links after freeing on prefix failure, so those undefined paths are not executed as a parity oracle. Four further callback retirement/reuse checks validate item or creator invalidation during pickup/state change and complete resource release. The original scheduler's missing return when its retained item is absent is preserved in the extracted body (compiler warning); its return value is never read. No padding or undefined return is normalized to manufacture a match.

Retained owner tests cover null and the source holder, owner motions340..361, and interaction with either fixture fighter. They do not claim arbitrary foreign-package rope cleanup or the unfinished early follower CPU/family-publication foundation. GPU rendering, real stage contacts, complete constructor engine internals and gameplay are outside this console proof. Retail imports stage88 named Popo values/defaults and20 Nana values; normal copied-base registration remains pending.

## Reproduce

Run from the repository root, using fresh output paths. The constructor runner prepares both forms, imports real DAT values, generates C with the frozen content tool, and compiles with the frozen compiler/SDK; it never calls snapshot script-build.

```powershell
python tests/native/ice_climbers_rope/primary_run.py --root . --baseline .local/baseline68 --reference .local/baseline66 --output A:/FreakMelee-test-archives/ice-primary68-root-repeat --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso"
python tests/native/ice_climbers_rope/constructor_run.py --root . --baseline .local/baseline68 --reference .local/baseline66 --output A:/FreakMelee-test-archives/ice-constructor68-root-repeat --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso"
python tests/native/ice_climbers_rope/run_solver.py --root . --baseline .local/baseline68 --reference .local/baseline66 --output A:/FreakMelee-test-archives/ice-rope68-solver-root-repeat --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso"
python tests/native/ice_climbers_rope/run_render.py --root . --baseline .local/baseline68 --reference .local/baseline66 --output A:/FreakMelee-test-archives/ice-rope68-render-root-repeat --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso"
```

No global build, game, audio, foreground window, installation or commit was performed.
