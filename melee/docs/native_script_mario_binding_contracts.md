# Complete Mario source binding closure

The initial whole-source compile and exact production body-resolver census found
55 of 76 bodies compiled, with 21 unresolved. The source-backed suite is archived
at `A:/FreakMelee-test-archives/script-mario-contracts-initial-oct07/receipt.json`.
It uses an explicit move/entity/effect-header metadata provider, controlled
assets and typed import values; it does not infer package or gameplay acceptance.

The final source has an authored `ResetRootX` helper that calls the existing
generic root bone rotation operation. Tornado binds that helper's code ID through
`callbacks.damage` and `callbacks.death2`. This preserves the old generated
helper's replacement behavior without a specialized native callback policy or
authored C function. The old convenience remains in `drmario_full.fms` and
`luigi_wip.fms` for their later focused source migrations.

The complete 77-function source now passes `nativeScriptCompile_Build`, and all
77 bodies type-check and lower independently through the extracted production
compiler context, resolver, motion lowerer and bytecode emitter. The receipt is
`A:/FreakMelee-test-archives/script-mario-contracts-oct07-final/receipt.json`.
The existing typed `item.attr.minimum_bounce_speed` alias already resolves to
the Article's owned parameter; no inherited C attribute read was introduced.

New generic runtime families cover CPU grab/level observations, common CPU
scripts and ranged/candidate primitives; ground/air controls and input facing;
standard and parameterized special-landing collision; visibility groups;
reflector create/disable/retain/update; accessory cue clearing; bone effects,
wind and Item bone effects. Their actual native sources are `ftcmdscript.c`,
`ftcpuattack.c`, `ftcommon.c`, `ftparts.c`, `ft_081B.c`, `ftcoll.c` and the
substantive Item bone-effect primitive in `native_fighter_module.c`.

Special landing preserves `ft_800831CC` collision, environmental response,
landing, walljump and cliff ordering. It passes its Boolean/Float landing
arguments directly to the common landing primitive. It installs no generated
landing callback, per-call C helper or shared parameter scratch. Source guards
stop before further userdata writes if a probe retires the actor.

`articles.stop_cue` uses the actor's generic motion-phase clearing operation.
That clears the selected accessory code ID and actual engine slot and preserves
the packed phase state. Normal motion reselection reinstalls its declared phase.

Runtime source comparisons pass:

- CPU/actions: 2,289 checks, including the actual common command walker, bounded
  candidate append behavior, ranged geometry branches, facing and visibility.
- Collision: 1,163 checks across all 256 collision/control combinations, full
  Fighter byte comparison and native call ordering, plus probe retirement.
- Feedback: 668 checks comparing actual reflector field writes and updates,
  part selection and Item indexed/child-chain effect selection, wind arguments
  and source retirement during effect calls.

Receipts are in `script-action-contracts-oct07-final`,
`script-collision-contracts-oct07-final` and
`script-feedback-contracts-oct07-final` under
`A:/FreakMelee-test-archives`. These fixtures mechanically extract current
native primitive bodies. Collision geometry, command execution leaves, effect
creation, joint position and platform/model leaves are controlled and explicitly
recorded. Their reference extraction is test-only; ordinary FMS never generates
C or loads a script DLL.

The separate effect closure supplies `effect.joint` selectors and
`effects.spawn_root` in semantic domain `0x03000000`. The catalog's ordinary
Read/Write/Invoke and call layout route that domain to the direct effect unit;
the compiler resolves it with the exact indexed source scope. `atan2_float`
already supports Effect context. Native model-joint missing-path default/no-op
behavior remains the audited common primitive contract.

Whole-source compilation and these primitive comparisons establish the typed
binding closure. Complete Mario asset imports/publication, actual fighter ABI
dispatch and effect scheduling are separate connected owners. This receipt is
not a claim of full live Mario gameplay parity, package activation, billboard
rendering or rollback/netplay acceptance.
