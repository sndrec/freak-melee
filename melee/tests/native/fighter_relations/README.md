# Fighter relations actual-source console fixture

Final source matrix: **429 scenarios / 13,355 checks**, no compiler warnings.
Receipt: `A:/FreakMelee-test-archives/fighter-relations-hitlag68-complete/receipt.json`.

```powershell
python tests/native/fighter_relations/run.py --snapshot .local/baseline68 --output A:/FreakMelee-test-archives/fighter-relations-hitlag68-reproduction
```

Run from the repository root with a fresh output directory. The snapshot is
used for its hash-verified compiler only. The runner extracts **live** actual
function definitions and compiles with live headers. The receipt states this
distinction and hashes key source/header/fixture/extraction inputs. All **18**
extracted bodies match frozen68 individually; that does not claim complete
transitive live-header correspondence to the snapshot. No game or UI starts.

## Actual code and controlled boundary

Complete extracted bodies execute `nativeFighterRelations_Link`, `LinkedId`,
`CopyCollision`, actual `ftLib_8008741C`, `mpCopyCollData`, and
`nativeFighterTarget_BonePosition`. Actual `native_callback_lifetime.c` is linked
directly and walks the actual HSD fighter-list representation populated by the
fixture. The actual `GET_FIGHTER` inline/header is used; registry/lifetime
answers are not replaced with successful mocks.

Shared hitlag entry/exit execute complete actual `setBit`,
`Fighter_UnkSetFlag_8006CFBC`, `Fighter_8006CFE0`,
`Fighter_UnkRecursiveFunc_8006D044`, `Fighter_8006D10C_Inline1` / `Inline2` and
`Fighter_8006D10C`. Actual Popo `setRefGObjFlagAndClear` /
`ftPp_SpecialS_8011F68C` and Nana `ftNn_Init_801238E4` execute link cleanup.
Those cleanup bodies use the actual `nativeFighterTarget_Id` and
`ftLib_80087460`. No helper/body is replaced with a successful policy mock.

Joint-world-position evaluation (`lb_8000B1CC`) is the controlled boundary. It
counts one call, provides a chosen position, and can retire/recycle/replace or
change the target during that call. Model-effect lifetime branches have explicit
rejecting fixture stubs and are outside this fighter-only matrix. No renderer,
joint animation engine, generated DSL callback or package import is exercised.
Cleanup `ftPartSetRotX` is a counted zero-rotation boundary. Pre/post callbacks
are fixture callbacks that record ordering and emulate synchronous retirement,
recycling and link replacement; effect pause/resume implementations are not
linked.

## Coverage

- Full-width IDs including `16777217`, high-bit IDs and `UINT32_MAX`; rejected
  zero/unmapped IDs, absent registry, null userdata and live lookup identity.
- Link, explicit two-way link, unlink, self-link rejection and preservation of
  the old link after rejected assignment. Link is intentionally one-way;
  reciprocal relations are authored as two explicit calls.
- Reused target address with a changed creation ID invalidates only the stale
  direction; valid reciprocal current-generation references remain observable.
- Unlinked object and Fighter storage set to `PAGE_NOACCESS`: retained lookup,
  link query, unlink and collision rejection must avoid retired memory reads.
- 256 distinct collision inputs: mode2 copies positions, contact/ECB/environment
  and surfaces, while retaining destination object, auxiliary vector/flags,
  source geometry, selected joint and ledge controls. Source collision and
  unrelated Fighter bytes remain unchanged; invalid copies do not mutate;
  same-object copy is harmless. This checks observable field ownership rather
  than replacing the collision copier with a fixture answer.
- Raw-part/output/kind/table/count/joint validation before pose evaluation;
  rejected outputs remain untouched. After pose evaluation, removal, generation
  reuse, userdata replacement, kind and ft_data changes reject publication.
  A separate retirement case protects both target object and Fighter memory
  during the pose boundary, proving continuation does not read those addresses.
- Healthy no-link, one-way, reciprocal and three-member cyclic graphs, with
  pre/post callback presence and child allow-SDI/damage gates. The real marker
  stops entry traversal after the selected endpoint; the fixture does not assume
  arbitrary graph traversal. Callback order, b5/b7 transitions and conditional
  x221A_b1 notification match those actual bodies.
- Pre/post retirement or same-address generation replacement stops source
  continuation. Endpoint replacement uses the current link: an unmarked new
  endpoint skips exit notification; a marked new endpoint receives it while the
  old endpoint remains unchanged. Stale/recycled/unlinked/protected endpoints
  are ignored.
- `PAGE_NOACCESS` parent retirement from a child callback, direct and recursive
  child self-retirement, plus nested Inline1 -> Inline2 -> CFE0 -> D10C cleanup
  where the nested callback removes the original child. Both GObj and Fighter
  storage are protected before the continuation returns.
- Popo/Nana cleanup across valid/stale/recycled/protected links and both marker
  bits. The source always clears its own pointer/generation; only a live matching
  endpoint receives borrowed link cleanup and the conditional marker flag.

## Direct-writer audit

The same invocation inventories **49 direct x1A5C assignments in 25 files**:
48 existing stock/common sites plus the new native relation helper. It removes
comments/strings from the lexical scan, excludes comparisons and records every
site, original assignment, receiver, line and affected file hash in the receipt.
Module and CaptureCaptain chained writes are included. Each must be immediately
followed by the same receiver's generation stamp, obtained from actual live-ID
lookup. The relation helper's explicit `target` stamp is accepted only with its
actual Snapshot, target lookup and null/self rejection checks present.

This is an intentionally bounded audit of the current direct
`identifier->x1A5C` assignment grammar, not a complete C parser or an alias/whole
struct-copy proof. Unknown receiver/compound assignment or stamp shapes fail
for review rather than being guessed. It does not introduce a writer wrapper.

The actual lookup now rejects null registry and zero IDs before indexing. No
remaining production defect was found in the tested bodies. The first compile
attempt is preserved at `fighter-relations-live68-first` (fixture omitted the
actual ftlib declaration header); the second passes 290 scenarios / 11,556
checks. Final adds the protected pose-retirement scenario. No production fix
was made by this worker. The latest `current` receipt refreshes actual target
source hashes and includes the actual fighter.h declaration source as requested
after root's include correction; its matrix is unchanged from `final`.
The hitlag extension preserves those prior291 scenarios and adds138 scenarios.
Its first executable pass produced426 scenarios/13,323 checks. The completed
receipt adds protected recursive child retirement and marked replacement
coverage, plus the writer audit. All earlier receipts remain preserved.

## Integration

Root owns CMake. The console command above is ready for coordinated invocation.
An extracted-body executable target needs `test.c`, generated `bodies.h`, actual
`src/melee/ft/native_callback_lifetime.c` and the usual Melee/Dolphin include
paths, `MELEE_NATIVE=1`, C11/MS extensions and Windows allocation APIs. Its
registry/model-effect symbols are owned explicitly by the fixture. The runner
intentionally requires fresh outputs; fixed CTest output paths need a root-owned
rerun policy before direct registration.

Actual runtime `MemberId` is separately exercised in the sibling family runtime
fixture. Full hitlag timer/SDI scheduling, effect pause/resume, full Popo/Nana move
policies, form swapping, player mapping, script lowering, asset import and
gameplay remain separate evidence.
