# Interpreted costume attachment checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Ordinary interpreted fighter packages now call `attachments.has_root`,
`attachments.load`, `attachments.add_dynamics` and `attachments.clear_all`
through their actual native attachment lifecycle. No generated module, stock
fighter handler, compatibility alias or per-tick allocation is involved.
See the [API documentation](scripting/model_attachments.md).

Creation and cloth setup are admitted only during initial actor attachment's
`on load` execution, including local helpers. A later invocation of that hook
does not enable allocation. The transient loading flag is false at idle and
cleared by typed actor snapshot restoration. Querying roots and releasing
attachments remain available outside loading.

Bindings retain the actual fighter, program, storage, rig and selected costume
identity before calling the native helpers and before publishing output. Costume
graph import may legitimately append arena bytes/records during loading; stable
arena identities and capacities are checked without requiring an unchanged
used-byte count. Literal roots retain their caller-source spans and use bounded
stack text. Visibility entries require the imported parts-descriptor schema,
with nested table spans checked against that fighter's imported arena. These are
distinct from article pointers, joint graphs and raw visibility lookups.

Jigglypuff's Rollout source now uses the existing generic
`effects.spawn_with_two_scalars_at_position` operation. Effect 0x3FF already
accepts precisely the original position, facing and floor-angle payload. No
new oriented-effect alias was needed.

## Verification

- The connected capture/lifecycle cohort passes **101,962 checks / 9,598 cases**.
  It executes the real compiler, actor, VM, top-level binding routing, retained
  source admission, DAT root lookup and native attachment load/dynamics/cleanup.
  Selected custom skin and fallback costume paths, body-chain preservation,
  attachment masks, repeated cleanup, typed root spans, visibility-table errors,
  allocation outside Load and retirement during model creation are covered.
- Attachment graph import, JObj construction, draw-list/material/visibility
  initialization and cloth-chain allocation/configuration/free are controlled
  leaves. Rendering and cloth simulation are not executed. This is lifecycle
  and admission evidence, not rendered costume or whole-fighter parity.
- Seven Windows targets build; the `native_fighter_script` reference CTest passes.
- The real read-only roster audit still prepares **19/26 forms**. All **192**
  Jigglypuff bodies now compile, with complete metadata/body visits. Package
  admission reaches `shield_break`, whose callback scheduler remains unmigrated.
  A passing body audit is not a passing package or gameplay acceptance.
- No game was launched and no installed content changed.

Evidence (independently rehashed against the final source):

- `A:/FreakMelee-test-archives/script-attachments-oct10-complete/receipt.json`:
  SHA-256 `42705f362bc5c487261356679f0aed300d1815ba83686bb229575c540613d191`,
  **1,826** input pins.
- `A:/FreakMelee-test-archives/source-full-roster-audit-oct10-attachments-complete/receipt.json`:
  SHA-256 `ba2a3cfc78becdb8ef59dbedba5e3cb5f30614a808ad77b699546da02e7ecad1`,
  **213** input pins.
- `A:/FreakMelee-test-archives/attachments-independent-pins-oct10.json`.

The overall goal remains active. Jigglypuff's shield-break scheduler, remaining
fighter package/resource gaps, full clone behavior coverage and compact
whole-match rollback acceptance remain open.
