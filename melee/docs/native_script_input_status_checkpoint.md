# Interpreted input/status hooks and Fox/Falco publication — 2026-10-10

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Fox and Falco now pass full ordinary-interpreter preparation with zero
diagnostics, covering all 256 and 253 functions respectively. Both complete
sources also package, verify, stage and publish through the real runtime with
three owned articles and one FireTravel effect each. Connected gameplay parity
was pending at this checkpoint; the subsequent
[gameplay checkpoint](native_script_fox_falco_gameplay_checkpoint.md) records
twenty passing interpreter comparisons. Earlier generated-C comparisons do not
establish interpreter acceptance.

## Scheduler contract

- `on wait_input:` returns Bool at the original idle-input call site, after
  attack/grab/shield checks and before common taunt/jump/movement checks. True
  consumes input; false continues. Fault or retirement stops continuation.
- `on action_status:` returns Bool after the common action-ID shortcuts used
  by statistics bookkeeping. Fault, retirement or absent hook returns false.
- Both events execute directly through the existing actor/VM scheduler and
  exact typed hook contracts. Source-owned actors do not fall through to
  native authored predicates or stock Fox handlers, even without a hook.
  Native predicates and stock paths remain available for ordinary native
  fighters. No new special-specific primitive, wrapper or compatibility alias.
- Existing lifetime guards validate the retained actor/program/storage before
  consuming a result or continuing native work. The script owns conversation
  decisions using the generic stage/statistics APIs from the prior checkpoint.

References: `ftCo_Wait.c:ftCo_Wait_IASA`, `ft_0892.c:ft_800898C0`,
`ftfoxappeals.c`, [language hooks](scripting/language.md),
[stage/statistics checkpoint](native_script_conversation_checkpoint.md) and
[independent model construction](native_script_primary_model_checkpoint.md).

## Grouped checks and boundaries

- Seven Windows targets and script/parameter CTests pass (2/2).
- Connected Fighter scheduler fixture passes 3,001 checks. New cases run the
  exact input/status bodies with real compiler, storage, actor and VM. They
  exercise both Bool choices, every predecessor/successor stopping position,
  common-state shortcuts, absent hooks, execution faults, retirement and native
  predicates on detached native actors. Common input decisions and trace-write
  fault/retirement leaves are controlled. This proves scheduler routing/order,
  not the complete radio conversation or input physics.
- Isolated source audits report complete metadata/body traversal and zero
  diagnostics for both whole sources. Unlike prior audit stages, package
  preparation is accepted, not merely individual bodies.
- Isolated publication batch passes source save, source package creation and
  verification, then 43 actual runtime checks per fighter. Real common assets,
  package importer, staging, program/data-table publication, failed-stage
  preservation, package leases and initialized page-history retirement gates
  execute. No Fighter/Item/Effect actors or renderer are created in this test;
  it does not prove a whole match, explicit snapshots or rollback performance.
- Stale scheduler-fixture assumptions were corrected: an already-faulted
  recipient reports its own delivery failure instead of copying the shared
  VM's latest diagnostic. Unselected setup leaves abort if reached. The first
  publication attempt reached real publication but expected zero effects;
  the source correctly owns its declared FireTravel effect. Failed logs are
  retained separately. No gameplay behavior was changed to satisfy fixtures.
- Final receipts independently rehash 1,704 scheduler inputs, 20 audit inputs,
  seven batch inputs and 34 inputs plus a cache packet for each runtime check.
  Shipping sources, seven binaries and archived build/CTest logs also match.
  No game launch, normal installed-content changes or user-draft changes.

Evidence under `A:/FreakMelee-test-archives/`:

| Receipt | SHA256 |
| --- | --- |
| `fox-falco-input-hooks-scheduler-oct10-d/receipt.json` | `fdcd948af9bdeebcddeea958c5d02af6f3a3d2084abe5238e66a842c8de1f3a6` |
| `fox-falco-input-hooks-audit-oct10-b/receipt.json` | `bdcac529e90da48b3b3ab1b489b92b1727e9e3b1f16cce87c79fc53e8235f82e` |
| `fox-falco-input-hooks-publication-oct10-c/receipt.json` | `9c62706fbe1317ee1a9f7d14ead425a794afddc4a39ff4d8453bc2dffea843f4` |
| `fox-falco-input-hooks-publication-oct10-c/Fox-runtime/receipt.json` | `436e5f12cea43bef60c0591e1156974543d5a0b1b8fc3b23e0480c8178b01ebb` |
| `fox-falco-input-hooks-publication-oct10-c/Falco-runtime/receipt.json` | `fa831e9d8296e0eb105ee34913c783fcd4f7ea1a6963ae5c80dbef65d7320b2a` |

`fox-falco-input-hooks-independent-oct10-b/receipt.json` records independent
rehashes and archived logs. The prior model checkpoint remains valid for its
earlier build; its pinned binaries are intentionally superseded here.

Subsequent work: [connected interpreter gameplay comparisons](native_script_fox_falco_gameplay_checkpoint.md)
now pass twenty cases. Remaining fighter groups, untested gameplay branches and
compact rollback acceptance remain open. Goal active.
