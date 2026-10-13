# Zelda/Sheik direct primitive closure contracts

Read-only source audit; no production/template/project edits, builds or games.
The relevant templates are `examples/zelda_full.fms` and `sheik_full.fms`.
These are generic joint/position/motion operations, separate from paired runtime
publication and destination form callbacks.

| Source need | Current direct behavior | Exact generic closure / source rewrite |
| --- | --- | --- |
| `effects.spawn_on_part(effect, rawjoint)` | `effects.spawn(effect, part)` resolves a **semantic** FtPart through `ftParts_GetBoneIndex`; raw104 is outside the54-part semantic domain. | Use canonical `effects.spawn_on_part(IntegralAny effect,I32 rawpart)->Void`, selecting the actual instance joint at that raw index. The former `spawn_on_joint` spelling has been removed; scalar payload calls remain separate. Do not remap104 or alias the semantic operation. |
| `effects.wind_at_position(x,y,z,frames,strength,decay,frequency)` | Six-argument `effects.wind(part,xoffset,frames,strength,decay,frequency)` queries a semantic joint, then adds `xoffset*facing` to its world X. | Add seven-argument `effects.wind_at_position(F32,F32,F32,I32,F32,F32,F32)->Void`, copy a Vec3 and call `lb_800119DC` exactly once. Keep the authored position call; do not re-query a joint or add facing. |
| Ground momentum save/reset/divide/restore | `ground.velocity` is already readable/addressable/writable F32, mapped directly to `Fighter.gr_vel`. | Keep these expressions; no new alias or field primitive is needed. Preserve F32/F64 operand promotion and single narrowing to the native F32 field. |
| `state.change(motion,flags,frame,1)` | `state.enter_at(motion,flags,frame)` calls the same `Fighter_ChangeMotionState(...,frame,1,0,NULL)`. | Rewrite to existing `state.enter_at`; preserve the following explicit `animation.start()` and subsequent rate changes in their original order. |
| `state.change(motion,flags,frame,rate)` with non-default rate | Existing `state.enter_speed(motion,frame,rate)` has flags0; `enter_at` fixes rate1. | Add generic `state.enter_at_speed(integral motion,U32 flags,F32 frame,F32 rate)->Void`. Preserve all four values; use it for flag-bearing rate0/variable calls. No implicit animation start. |

## Native evidence and arithmetic

`ftZelda/ftzeldaspeciallw.c::ftZd_SpecialLw_8013ADB4` directly spawns effect1276
on `fp->parts[104].joint`; `ftZd_SpecialLw_8013AE30` spawns1277 on raw4.
`ftSeak/ftseakspeciallw.c::fn_80114034/fn_801140B0` use raw43/raw4. The template
parameters preserve those indices. `native_fighter_module.c::spawn_effect_on_part`
also indexes the instance parts array directly; this old test-reference helper
is evidence of the contract, not an implementation route for source execution.
`ftparts_table.c::ftParts_GetBoneIndex` instead indexes `part_to_joint[part]`.
The current `native_script_binding_feedback.c::bone(...,true)` deliberately uses
that latter semantic contract for `effects.spawn` and `effects.wind`.

`efsync.c` cases0x4FC/0x4FD consume an `HSD_JObj*` and create native particle
generators. They are not interpreted model-effect programs; do not route these
calls through a dummy authored Effect spec. A raw-joint spawn must admit only
native effect dispatch signatures compatible with its one-JObj argument, with
source-audited signatures rather than arbitrary vararg reinterpretation.

`ftZelda/ftzeldaspeciallw.c::ftZelda_SpecialLw_StartAction_Helper` divides
self X/Y and ground speed, queries raw TopN world position, then calls
`lb_800119DC(position,120,0.4,0.003,MTXDegToRad(60))`. Sheik's
`ftSk_SpecialLw_Enter/ftSk_SpecialAirLw_Enter` and `ftSk_unk_inline` preserve the
same order with `M_PI/3`. `tools/native_fighter_script_spatial.c::emit_wind`
materializes Vec3/I32/F32 locals and calls the same wind primitive.
`lb_00F9.c::lb_800119DC` copies that position, stores count/strength/decay/angle
and uses `lb_800100B0`. Frequency is already radians: do not convert to degrees,
scale strength, substitute actor position, or recompute source world coordinates.

`tools/native_fighter_script_code.c` emits ground reads/writes as `gr_vel`;
current `native_script_binding_kinetics.c::Read/Write` does the same.
`native_script_bindings.c`'s W macro includes readability. In
`native_script_value.c::float_binary`, F32 division uses float operands and F64
division uses double operands; float conversion narrows directly. IEEE zero/
overflow behavior is preserved for float arithmetic (integer divide-zero is an
error). Do not invent zero clamps, finite-only ground writes or extra Float64
intermediates. Keep imported float parameters as F32 and preserve expression
parentheses/evaluation stages, including teleport/vanish speed/cosine formulas.

The reference `state.change` emitter in
`tools/native_fighter_script_primitives.c` calls
`Fighter_ChangeMotionState(motion,flags,frame,rate,0,NULL)` without animation
start. Current `native_script_binding_transition.c::Invoke` already provides
layout validation and post-callback checks. Rate0 is intentional in the templates
(for example flag0x0c4c508e transitions); replacing it with rate1 or dropping flags
changes behavior. `state.begin` also starts animation and is not an equivalent
rewrite when the source already calls `animation.start()` separately.

## Source identity and retirement

New operations must use the real bound source, exact selected bank/layout/data
and owned part table. Raw indices use the signed Int native contract. Negative/out-of-range raw parts
or absent joints produce the native healthy no-op; valid joints must satisfy
owned rig/class/lifetime admission. Semantic indices retain their independent
54-role validation. No stock Definition lookup or one-fighter
raw104 exception is appropriate.

Capture scene/slot generation and source program/storage identity before native
calls. World-position sampling, effect construction, wind creation and motion
changes must stop publication/continuation when the source retires or faults;
revalidate before a second callback-sensitive stage. Preserve completed native
side effects without replaying or silently undoing them. Do not dereference a
cached Fighter/joint after a failed guard. Keep flags and motion IDs integral,
frame/rate as F32, and preserve legal native rates instead of clamping them.

Acceptance should compare actual raw104/raw43 pointers against deliberately
different semantic mappings, preserve absent raw-joint no-ops and reject stale lifetimes, verify exact copied wind
arguments and float bits, exercise ground division/momentum and rate0/1/variable
flags, and trigger retirement/reentrant motion callbacks between native stages.
Then compile complete source bodies against real owned rig/layout metadata and
test hidden/muted form gameplay. This audit is a contract proposal, not any of
those execution or rendering proofs.

## Implemented feedback checkpoint

The feedback batch now implements distinct raw-joint and copied-position wind
operations, plus generic `effects.spawn_with_scalar(effect,semanticPart,value)`
and `effects.spawn_on_joint_with_scalar(effect,rawjoint,value)`. The latter pass
the address of a local copied F32, never a pointer into actor fields or authored
argument storage. `native_script_effect_dispatch.c` admits82 exact one-JObj and32
joint+immediately-consumed-F32-pointer signatures across the three real dispatchers;
erased void pointers, forwarded va_list and nonmatching/multiple argument
contracts remain unsupported. The metadata is fighter-independent and fixed.

Admission also applies to existing semantic `effects.spawn`. This corrects a
confirmed prior misuse: effect1146/0x47A consumes a JObj and an additional F32
pointer in `efAlt_Spawn`; the former one-JObj call was undefined varargs behavior.
The actual Mario source's neutral-special effect therefore requires the explicit
scalar operation with `facing.direction`, preserving the stock immediate sign
test. No effect ID substitution is justified. The parent owns the source rewrite.

Fighter feedback now validates its exact SourcePackage/program/layout/storage/
parameter identities, native pool epoch/generation, actual data and imported part
table/arrays before native calls and between callback-sensitive stages. Failure
preserves output and stops continuation; completed native side effects remain.
Existing semantic joint mapping and wind-facing-offset contracts are preserved.
No stock Definition or authored Effect spec selects these primitives.

`A:/FreakMelee-test-archives/script-form-feedback-second/receipt.json` passes
66,735 checks /61 cases, including every16-bit effect ID, all114 admitted signatures,
raw104/43 versus different semantic joints, local scalar bit/address isolation,
exact copied wind, incompatible signatures and39 retirement/identity mutations.
Actual feedback/catalog/guard/mapping/pool code is linked; package/program views
and effect/wind/sample leaves are controlled. This is not native particle/render,
full VM, complete form or live game acceptance. Rate-preserving transitions and
catalog/source composition are separate parent/worker components.
