# Linked Ice Climbers source contracts (unregistered preparation)

SDK66 is the immutable reference. This namespace does not activate a family,
inherit stock callbacks, or modify the accepted Ice Shot/Blizzard component.
Companion coordination means two simultaneously live fighters; it is separate
from Zelda/Sheik transformation and package transfer.

## Existing operations that already close source dependencies

- `collision.ground_box(12,0,-6,6,6,6)` binds **ft_80082888**, and
  `collision.air_box(...)` binds **ft_800824A0**. The similarly named
  ground-box-edge operation is different and is not substituted.
- Ordinary fighter variables, typed helper returns, typed recipient exports,
  named attributes, root rotations, `physics.air_decel_normal()`, `physics.fall`,
  and `state.freefall` express the scalar decisions. AccelToVel is a short
  authored arithmetic helper; `physics.air_accel` is a different shared rule.
- Rope needs at most 40 nodes per fighter. Existing fixed resource collections
  hold node position/velocity/collision/model storage. Only active flags and
  small scheduling state need script bytes: do not duplicate six 40-float
  arrays in the bounded 512-byte article state.
- `it_802A24A0` only invokes HSD_GObj_JObjCallback if ItemLink.x2C_b0. Existing
  resource draw visibility expresses this, including the always drawn tail.
  Geometry/matrix and animation lifetimes still require direct comparisons.

## Concrete missing generic contracts for paired source policy

1. **Live family member ID**: `(Fighter_GObj*, unsigned role) -> uint32_t`,
   absent=0; resolve same player/family and full creation ID without retaining
   a pointer. Source uses Player_GetEntityAtIndex(player_id,0/1) throughout
   ftpopospecials.c, ftnanaspecials.c and both specialhi files. Family/package
   foundation is owned by root. Numeric roles are authored policy, not a
   Nana-specific operation.
2. **Synchronous hitlag link**: validated full-ID endpoint assignment/clear
   for Fighter.x1A5C, plus the existing conditional release request semantics
   of Fighter_UnkSetFlag_8006CFBC (x2219_b7 implies x221A_b1). Squall cleanup
   requests source release first, then partner release, clears partner link,
   then source link. A script variable cannot replace this: fighter.c (2727-2776)
   consumes the link for actual shared hitlag propagation. Missing/dead/reused
   endpoints must be unavailable and must never unlink a recycled fighter.
3. **Collision history copy**: destination-owned operation taking a live
   source ID, implementing actual mpCopyCollData(src.coll,dst.coll,2), with
   validation before native storage reads. ft_800849EC is exactly this source
   operation. It precedes secondary air-to-ground in Squall recruitment and
   precedes secondary launch in Belay; copying just floor identity is wrong.
4. **Cross-fighter pose/state observations or bounded transfer**: current
   source reads position, velocity, acceleration, ground velocity/acceleration,
   facing, frame/rate, hitlag, raw root X and raw bone world positions. An
   owned recipient can expose scalar getters through existing typed exports;
   source-order faithful efficient vector/bone transfer needs a reusable
   validated target query/transfer, not public private-union offsets. Belay
   accessory fn_80123218 writes primary endpoint from the secondary hand at
   that precise callsite; merely polling it later changes scheduling.
5. **Initial follower policy**: ftCo_800A101C sets mode6/flagF9_b2/spacing15
   before existing CPU initialize events. Later CPU mode6 consumes the real
   30-sample input FIFO. An ordinary late hook cannot recreate early setup.
   Parent handles follower/controls foundations; this namespace will not
   replace it with an opaque native Nana callback.
6. **Owned air-speed multiplier/setup flags**: OnLoad sets Fighter.x40 from
   Popo attrs0 or Nana attrsC4; ftCommon_800804EC returns x40*scale.x. It is
   distinct from model scale. Source sets primary x2222_b5 (secondary teardown)
   and secondary x2222_b4 (pickup policy). Need generic ownership metadata or
   lifecycle fields, not raw booleans exposed with fighter-specific names.

## Exact decisions and source order to preserve

Squall recruitment rejects secondary categories1..13. Otherwise distance
comparison is **dx²+dy² < (s32)(primary.instance_scale_y * radius²)**; retain
integer truncation and strict comparison. On success, secondary entry/bindings
run before primary motion entry; copy primary positions/velocities/facing,
reset follow delay, rotate Y, then reciprocal link publication. Secondary
collision delay saturates at5; horizontal separation is1.5*delay*facing.

Belay eligibility rejects categories1,3..8,10..13 but accepts2/9/0/default,
provided x221F_b3 and hitlag are clear. Secondary enters361 and binds its
accessory before primary song-immunity publication. Primary launch offsets
vector X by -3*secondary.facing, Y by+5, normalizes, and multiplies by
base_speed+sqrt(distance²)/distance_divisor. Primary scheduler increments before
its command8 creation; commands9..83 use article-owned integer schedule values
with if/else-if priority;83 removes after possible command execution.

Full primary damage cleanup is **Ice, Blizzard, rope, Squall links**. Secondary
damage/death2 cleanup is only Squall links. Rope destruction clears the CURRENT
holder's retained rope slot, not an inferred creator; its solver endpoint still
reads its saved original source. Repeated notifications and callback bindings
must remain. Existing immutable author/reference APIs should validate retained
handles independently from reflected current owner.

## Source quirks and boundaries

- Primary Squall air collision compares `(env_flags & Collide_CeilingMask)==1`.
  This is preserved as written pending retail instruction evidence; it is not
  silently replaced with a nonzero predicate.
- Belay scheduler's missing retained rope branch falls out of a Bool function
  without a value. Existing callers discard the result. An authored void
  scheduling helper can preserve defined actions without synthesizing a value.
- Rope original first-allocation-failure uses uninitialized prev_link, and one
  error cleanup traverses after free. Do not reproduce undefined storage reads;
  fixed resource construction must be atomic and failures explicitly tested.
- Squall retained effect transport is mv.specials.x8 -> GObj. Before replacing
  it, identify the actual animation command/typed effect recipe writing that
  transport; no guessed recipe or stock updater is admitted as owned behavior.
- This is not a full versus activation census receipt: family lifetime,
  follower controls, shared friction/voice/pickup policies and owned package
  metadata remain the separate root foundation.
