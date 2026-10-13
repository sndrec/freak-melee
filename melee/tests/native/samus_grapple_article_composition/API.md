# Remaining generic Grapple Beam contracts

The healthy nine-state geometry, deferred normal/accessory3 updates, interaction
and resource cleanup compile against immutable SDK53. These are the actual
remaining gaps; no source-specific native solver is proposed.

1. **Physical controller reads:** fighter `input.up_held` bit8 and
   `input.z_held` bit0x40, Bool and fighter-context only, matching existing
   `input.down_held`/`input.attack_pressed`. `samus_grapple_state_sync` advances
   ordinary sequence0→1 on Up,1→2 on Down,2→3 on Up,3→4 on pressed A. It runs
   before the state0/state1 normal updates; constructor selects modified mode
   only when the sequence is4. Do not map Z to analog shield.

2. **Nearest eligible fighter ID:** a safe Int handle result0 on no match,
   substantive generic backend matching `ftCo_800A4A40`. That actual body walks
   the live fighter list, excludes self and blast-margin/ineligible fighters,
   applies the caller's CPU half-width/height margins, retains the first tie
   and picks strictly smaller `ftCo_800A1AB4` distance. It is not inherently
   enemy-only. `it_802B9328` normalizes target position minus tip position and
   multiplies by launch speed only when physical Z is held and modified mode1.
   The authored policy can select the safe target in a creator helper.

3. **Immutable complete fighter hitbox descriptors and joint binding:**
   `it_802B9328`, `it_802B99A0` and `it_802B9CE8` modified ground branches clear
   current capsules, then `it_802B7160` builds the source descriptor on pressed
   A and increments modified mode. Other modified ground frames clear capsules.
   Existing damage/group/position/enable/clear APIs cannot create that complete
   descriptor or bind it to a reserved model part. Use authored immutable specs
   and generic validated part binding, not a callback to `it_802B7160`.
   The source constant is `it_803B8660`; its raw byte reads/native bitfield
   packing and the uninitialized trailing `x134_4` need separate source judgment.
   The accepted fighter namespace's API.md records exact field values.

4. **Indirect animation-component import:** special-attribute offsets
   0x74..0xAC are pointer-to-pointer component arrays; source takes element0.
   A generic immutable component recipe needs aligned source offset,
   direct-versus-array representation and validated constant index. Import
   relocation once into the owned arena, including explicit unresolved
   external components; never reinterpret external-chain words as graphs.
   Roots0x64/68/6C/70 already import normally. `it_802B743C`, constructor
   `it_802B7C18` and their Add/Req/Anim order are the authoritative consumers.

5. **Resource-scoped effect dispatch with provenance:** state1 AirCatch wall
   contact transitions the fighter to358, changes the article to6/scales it,
   then calls `efSync_Spawn(0x41C, endpoint->gobj, &position, endpoint)` and
   `efSync_Spawn(0x3F1, endpoint->gobj, &position, &facing)` before returning.
   The endpoint is an actual owned resource GObj, not the article GObj. Standing
   and dash launch sparks likewise use resource GObj provenance. A generic
   `item.resource(index).spawn_effect_at_position(...)` should validate
   collection/index/generation/author and use that real GObj directly. Its
   effect result type/lifetime policy must follow the existing effect classifier.
   Actual async source only consumes the Vec3 for0x41C; no unsafe ItemLink
   argument needs exposure. Do not create fake Item contexts or float handles.

6. **Throw accessory:** this remains the previously inventoried generic
   structured fighter-item model import and post-common throw-enter event.
   Samus fighter item slot4 contains a base root, base joint/material animation
   and four selected throw animations; raw attachment51 has no semantic alias.
   `ftSs_Init_CreateThrowGrappleBeam` owns the exact Add/Req/Anim/rate/bind order.
   It is independent of the nine article solver callbacks compared here.

No generic callback/geometry gap is needed for the healthy modes. The ordinary
counter `ftSamus_State7Vars.x0` is actually f32, and remains a Float script field.
The native decomp's uninitialized direction pointer and narrow double-literal
precision are explicitly classified in the receipt/README rather than proposed
as engine APIs. Source and frozen input hashes accompany the runner receipt.
