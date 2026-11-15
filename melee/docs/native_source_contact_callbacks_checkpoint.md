# Interpreted combat callback slots

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Implemented 2026-10-08. Reflector contact, completed reflection turns, body contact
and shield contact now use the existing per-instance interpreted callback system.
The native branch conditions and stock callback path remain in place; source
fighters can bind their own zero-argument Void helpers or track matching events.
See [callback slots](scripting/callback_slots.md).

## Engine and lifetime contracts

The four slots map to `reflect_hit_cb`, `x21F8`, `hurtbox_detect_cb` and
`shield_hit_cb`. The existing reflect/reflect_hit/hurtbox_detect event contracts
are reused; shield_hit adds the same source-audited `void(HSD_GObj*)` contract.
No callback pointer is generated or installed. Native processing calls the actor
entry at the original callback sites in `Fighter_ProcessHit_8006D1EC` and
`ftCo_800C37A0`, checking callback retirement before subsequent native reads.

Helper setters and named-event tracking validate the selected program, root
ownership and exact zero-argument Void ABI. Named events cannot be installed in
another slot. Explicit native callback assignments take over; explicit authored
binding clears the native pointer. Shared motion reset clears the four new slots
alongside the matching native fields. NewLife preserves nondeath registrations.
Packed actor snapshots include the new code IDs and validate their matching event
or helper before restoring. Aggregate native Fighter/scene peers remain required.

`ftColl_CreateReflectHit` replaces its contact handler even when its argument is
NULL. It now also clears the corresponding interpreted slot, so an old authored
handler cannot survive that assignment. Scripts re-arm their chosen event after
creation, preserving the original ordering rather than an implicit sticky handler.

Zelda's two older `effects.spawn_on_bone` calls now use the genuine semantic-part
`effects.spawn` operation followed by the original `effects.active = true` write.
The old translation resolved TransN semantically and set that flag; a spelling
replacement alone would have dropped behavior. No alias or special-case Zelda
primitive was added.

## Verification and remaining work

- `A:/FreakMelee-test-archives/script-contact-scheduler-oct08-second/receipt.json`:
  2,476 checks; actual compiler/VM/setup binding/actor and snapshot machinery.
  Includes all four named events, helper overrides, incompatible signatures,
  native takeover, reset, NewLife and snapshot/cross-slot validation. Contact tests
  invoke the actor entries directly; full contact collision/hit processing is not
  simulated. Native/scene/assets/presentation leaves retain documented controls.
- `A:/FreakMelee-test-archives/script-contact-feedback-oct08-root/receipt.json`:
  976 checks; actual shared reflector-create body and generic feedback comparisons.
  The fixture's callback-clear leaf is controlled; real actor clearing is covered
  by the scheduler/actor tests. No particles or live reflector collision acceptance.
- Complete native client/tool and affected fixture targets build. All 36 selected
  regression CTests pass (`.local/source-pose-unified-build.log`,
  `.local/source-contact-ctest-final.log`); the old actor fixture also verifies
  undeclared new events remain unbound.
- Complete-source preflight advances Zelda to line145's Article signal surface;
  Sheik still rejects resources metadata at1543. Evidence:
  `source-form-template-contact-preflight-oct08-root/receipt.json`. Original private
  draft inputs remain unchanged. Neither complete paired source is admitted yet.

The [complete binding inventory](native_source_form_binding_inventory.md) uses the
production lexer and actual descriptor catalog to identify all remaining groups,
rather than treating the first preflight error as the inventory. The separate
[Article resource audit](native_source_article_resources_audit.md) proves that
Chain's Link/Tip graph is already owned and that the two names alias one immutable
descriptor. A coherent source resource lifetime/matrix/model surface is still
required; metadata-only acceptance would not provide that functionality.

No game, public draft or installed-content update was performed. Full Zelda/Sheik
gameplay, companions and complete explicit rollback state/performance remain open.
