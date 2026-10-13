# Connected Item terrain and position/scalar effects

The runner compiles the actual compiler, catalogs, VM, Fighter and Item actors,
storage and physical GObj pools. Native terrain kernels and effect dispatcher
cases are compared with checkpoint `50f2b476b` before compilation. The one
entered-air continuation correction is checked against the exact narrow diff.

Coverage includes 64 full-Item-state native/binding comparisons: follow,
zero-argument bounce, Bool false and Bool true across both floor results and
eight wall/ceiling combinations. Sixteen authored VM calls cover nested
StageLeft -> bounce -> StageLanded, both bounce forms, entered-air continuation,
an arithmetic fault, source removal, and eight actual Fighter scalar-effect
calls. A controlled userdata replacement confirms the cached entered-air write
does not run on the retired Item. Compiler negatives reject numeric Bool and
wrong arity; effect negatives reject two-scalar ABI and missing creator root.

Stage-contact response adds 48 native/binding comparisons covering both native
routines, the zero-argument form, both Bool values, and every floor/wall/ceiling
combination. The contact callback runs through the actual Item VM. Additional
cases cover authored entry, arithmetic faults, removal and userdata replacement
inside that callback, plus exact-Bool/arity rejection. Impact and reflection
responses are controlled leaves; the native routines determine their order and
the contact hook's timing.

Stage intersection results, motion/sound/particle constructor leaves and
imported source asset metadata are controlled and silent. Authored nested hooks
run through the actual shared VM. This does not run a complete stage solver,
audio device, renderer, DAT constructor or game. Completed native side effects
are retained when a lifetime check rejects subsequent script publication.
The actual terrain, spin/ECB, contact-reset control, fixed-ECB installation, collision facing, class ancestry and vector
addition bodies execute. Impact/reflection, surface-motion, victim-history
clearing, damage-reset and terminal removal leaves are explicit controlled
sinks; comparisons do not establish those subsystems' full native behavior.

Run with a fresh private `A:/FreakMelee-test-archives` output directory:

```powershell
python tests/native/script_item_terrain/run.py --compiler C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe --output A:/FreakMelee-test-archives/script-item-common-oct10-final
```

The common Item extension adds 832 full-Item comparisons: 256 seeded default
ECB, exact signed 32-bit collision-facing and command-stream-clear cases each,
plus 64 slide variants across floor, wall/ceiling, grounded flag and landing
counter combinations. The real E8C4/75D5C/mpColl_SetECBSource_Fixed and
mpCollSetFacingDir bodies execute unchanged from the checkpoint. Command-stream
clearing is the exact legacy field operation, not a substituted native kernel.
Authored VM entries execute both slide tails, a no-model Item, the three simple
operations together, and destructive StageLeft reentry. Compiler/Host negatives
reject implicit collision-facing conversion and wrong arity. Unselected line,
sweep and scheduler operations in the actual split backends abort explicitly.


The October 10 batch includes unchanged native `it_8026E414` landing and
`Item_80267454` camera release bodies. Landing adds 16 floor/wall/ceiling pairs
and checks the authored StageLanded callback, while camera release adds eight
mode/attachment pairs with an explicit camera-release leaf. The cohort passes
5,305 checks / 968 native pairs / 198 terrain cases / 22 VM calls. Camera manager
and complete stage solving remain controlled, not live gameplay acceptance.
Receipt: `A:/FreakMelee-test-archives/item-landing-camera-oct10-a/receipt.json`.
