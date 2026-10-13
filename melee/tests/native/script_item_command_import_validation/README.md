# Item DAT command admission

This fixture compiles the current command importer, Article collector, fighter
asset importer and common Item importer directly. Other native dependencies use
the existing real native libraries. It performs no global build or game launch.
Compiler and console processes use `CREATE_NO_WINDOW`, a 120-second bound and a
fresh private A: directory. Input source/header/library and retail DAT hashes
are recorded and checked at completion.

```powershell
python tests/native/script_item_command_import_validation/run.py `
  --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe `
  --iso 'C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso' `
  --output A:/FreakMelee-test-archives/item-command-import-validation-fresh
```

Current acceptance:
`A:/FreakMelee-test-archives/item-command-import-validation-review8/receipt.json`
reports 2,669 checks / 47 focused cases, plus actual imports of all 33 stock
fighter archives and ItCo.usd: 185 Articles, 281 script entries and 1,063 unique
imported instructions. Zelda and Sheik are included. All 29 production stage
archive specifications also import, with 80 Item script entries and 150
instructions. The stage specifications are extracted unchanged from
`grdatfiles.c`; the actual `GrStageImportOptions.stage_item_script_article` points
to the imported common Random Pokémon Article for the complete stage import.
This common owner's import arena remains live throughout the stage pass.

The NULL stage-owner negative exposes exactly one current dependency:
GrGr.dat rejects opcode 10 at DAT offset `0x59024`. Its `ALDYakuAll` stream is
installed on Random Pokémon by `Ground_801C05D4`. The other 28 archives have no
reached command requiring this external geometry context. A missing context
does not skip geometry validation.

The importer now validates the native operands that actually index storage:

| Commands | Admission |
| --- | --- |
| 11, 12, 13 | Hitbox index below the actual four-element Item array |
| 14 | Full 26-bit index below four |
| 11 with nonzero bone | Dynamic table enabled; index below the actual 100-element table |
| 10 with indexed model | Bone index below 100 |
| 10 without indexed model | Native nullable repeated-child traversal retained |
| 17–19 | Scalar command variables, not hitbox indices; all values retained |
| Ignored operands | Remain unrestricted |

An unpopulated indexed joint slot may legitimately be NULL. The command handler
loads that slot; it does not require an existing JObj. Native joint-position
operations define NULL behavior. Admission does not impose a populated-count
restriction or reinterpret `modelDesc.bone_count` as an exact joint count.

For indexed models, the importer models default `JObjLoad` allocation order,
parent/child/next links, postorder descriptor-ID insertion and instance
resolution before following the exact `Item_802682F0` walk. Tests compare against
actual `HSD_JObjLoadJoint` and native JObj links for ordinary trees, forward
instances and duplicate descriptors whose last ID insertion wins. Cyclic
instance walks and 101-entry walks are rejected before Item's 100-entry table
would overflow. Cyclic reference fixtures are observed for at most 101 nodes
and retained in the private process heap; they are never drawn or recursively
destroyed. An absent descriptor root corresponds to the native single anchor.

The import analysis has an explicit 4,096 constructed-node bound. Unknown named
JObj load classes fail with `HSD_DAT_SYMBOL` and the descriptor's DAT offset,
rather than receiving an unproved default topology. No named load classes occur
in the scanned fighter/common imported joint records. Those records contain 25
instance descriptors, including fighter graphs; this is not a claim that all
25 belong to indexed Article models. Maximum indexed fighter Article walk is 38.

Control-flow admission tracks return addresses and typed loop pairs in the
actual three-slot `CommandInfo.event_return` array. Loop counts are represented
by their repeat/exit possibilities; count one exits directly and count zero
retains native unsigned wrapping behavior. Invalid pushes, underflow, mismatched
Return/EndLoop, recursive overflow and malformed reached operands reject before
publishing any script pointer. End may legitimately halt with pending frames.
NULL Call/Goto targets retain native halt semantics. Analysis is bounded to
65,536 unique `(pc, typed stack)` states. A shallow, finite, valid-depth fanout
fixture exceeds this bound and fails explicitly with `HSD_DAT_CAPACITY`; it is
not claimed to be an invalid native script.

Two entries sharing identical bytes but different actual Article owners prove
that a successful first analysis cannot bypass the second owner's geometry
checks. Failure preserves all destination pointers and allocates no command
records in the native import arena. Successful control fixtures execute the
real common command handlers. Asset topology, opcode decoding and importer
transaction semantics are exercised; gameplay, effect dispatch safety,
collision results, rendering and whole runtime safety are not proved here.

Earlier review4/5 failed because this console supplied the fighter's 2 MiB
import arena to ItCo.usd instead of its native 8 MiB arena. Review6 and later use
the proper owner sizes. No operand or structural policy was weakened to resolve
that fixture error.
