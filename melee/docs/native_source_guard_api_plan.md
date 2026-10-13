# Direct source guard-volume batch

Original read-only proposal, checked against the source on 2026-10-09. The
subsequent implementation and evidence are recorded in the
[guard checkpoint](native_source_guard_checkpoint.md). The contract audit below
records the pre-implementation reasoning; it is not the current missing-API list.

Peach needs one shared volume constructor, the existing ShieldHit instance
callback slot, position refresh, and scalar fields. Its source sites are
`peach_full.fms:725–727,779–788,816,829–844`. This does not require a Toad
handler, a new native event slot, or an authored C function pointer.

## Actual native contract

`ftcoll.c:3143–3154` (`ftColl_8007B1B8`) consumes only `ShieldDesc.bone`,
`pos`, and `radius`; the additional `dmg_mul`, `vel_mul`, and `flags` fields
in `lb/types.h:134` are unused by this constructor. It sets active (`x221B_b0`)
true, clears bits 1–4, assigns `shield_hit_cb`, and copies the selected
`fp->parts[bone].joint`, radius, and local offset into `shield_hit`. It does
not reset the hitlag overrides or contact damage/facing and does not execute
the callback. The bone is a **raw costume joint-table index**, not a semantic
FtPart; the reference `bone_count` is `ftPartsTable[kind]->parts_num`
(`native_fighter_module.c:165`). Resolve a semantic bone separately with the
existing `bones.index` operation.

`ftColl_8007AEE0` (`ftcoll.c:3054`) only clears
`shield_hit.skip_update_pos`. This is the exact `guard.update_position()`
operation, not a transform calculation or immediate contact test.

The actual Peach calls are `ftpeachspecialn.c:183,266`: create the descriptor
with `onUnkHit`, then enable `front_only`; the first activation also writes
the two hitlag overrides. `onUnkHit` at 378 is a zero-argument Void reaction
(native ABI `void(HSD_GObj*)`), reads recorded facing, changes motion, starts
animation, and notifies Toad. Those authored decisions belong in a script
helper. Ground/air motion switches clear callbacks and then recreate the
guard through `setupColl`, so source recreation must bind that helper again.

| Proposed field | Type/access | Native field |
| --- | --- | --- |
| `guard.active` | Bool RW | `x221B_b0` |
| `guard.reactive` | Bool RW | `x221B_b1` |
| `guard.skip_contact_details` | Bool RW | `x221B_b2` |
| `guard.front_only` | Bool RW | `x221B_b3` |
| `guard.filtered_attacks_only` | Bool RW | `x221B_b4` |
| `guard.melee_hitlag` | F32 RW | `shield_unk0` |
| `guard.article_hitlag` | F32 RW | `shield_unk1` |
| `guard.hit_damage` | I32 R | `x19A4` |
| `guard.hit_facing` | F32 R | `specialn_facing_dir` |

`types.h:1457` declares `x19A4` as **int**; the old guard documentation's
Float claim is inaccurate. Reference property emission also reads that
integer directly (`native_fighter_script_primitives.c:186`). Fighter and
Item collision paths record the largest damage and facing before reaction
(`ftcoll.c:448–479,933–945`); `fighter.c:3206` clears damage afterward.
The bit-3 filter compares attacker horizontal position with facing, allowing
equal positions; bit 4 tests the independent hitbox `x42_b4` flag. Item
contact details use bits 1/2 (`ftcoll.c:1778–1804,2216–2249`). Preserve these
fields independently rather than creating a counter-specific policy.

## Reuse the owned helper and existing delivery path

The installed slot is `NativeScriptFighterCallback_ShieldHit`, already
mapped to `Fighter.shield_hit_cb` (`fighter_actor.c:187`). Existing
`callbacks.shield_hit(Helper|None)` uses the generic CodeRef compiler gate:
Null or a direct owned function-name semantic, never an integer, computed
callable, or receiver helper (`bindings.c:703`). The actual actor binder
requires root owner 0, zero arguments, Void result, and the current actor
(`fighter_actor.c:199–217`). This is sufficient for the constructor's
helper argument without an extra callback enum or native thunk.

Preferred explicit source signature:

```text
guard.create(raw_joint: IntegralAny, x: F32, y: F32, z: F32,
             radius: F32, helper: CodeRef | None) -> Void
guard.update_position() -> Void
```

Both calls are Fighter-only, effectful/MayInvalidate; constructor is
SourceOnly because the CodeRef belongs to the executing program. Use exact
owned SourceFighterLayout plus retained NativeFighterLifetime and the actual
selected package/rig table. Validate helper ownership/ABI and all arguments
before any writes. Construct a local ShieldDesc and call
`ftColl_8007B1B8(source, &shape, NULL)` directly, then install the validated
code ID with `nativeScriptFighter_BindCallback(...ShieldHit, helperID)`.
None explicitly clears. No stock handler is selected. Make the native
constructor's interpreted-slot replacement match its raw pointer assignment,
as `ftColl_CreateReflectHit` already does for ReflectHit; otherwise old code
IDs survive unrelated native reconstruction with a NULL callback.

For reference parity, negative/out-of-span joints and negative/non-finite
radius are healthy no-ops preserving the old volume and callback. The
reference guard admission is explicit in
`native_fighter_script_primitives.c:989–998`. Check the integral width before
narrowing. An admitted missing or invalid-class/extent joint, changed owned
table, or retired/faulted actor is an error. There is no reference or native
finite restriction on offset coordinates or hitlag overrides: preserve F32
bits rather than silently extending the radius policy to those values.

`fighter.c:3043–3054` already runs a bound ShieldHit code ID before native
fallback and stops on false RunCallback or physical retirement. Use this
single delivery path. The next implementation should add retained
NativeFighterLifetime validation around that continuation so a successful
helper cannot replace live userdata/program/schema before cached `fp` reads.
Keep the native bool3 ordinary-shield reaction branch and subsequent hitlag
ordering intact. No callback bound means no authored reaction; source
construction always leaves the raw pointer NULL.

Peach's `on guard_hit` descriptor is Void0 (`events.c:615`) but currently
fails Prepare: the root hook table/instance-name table recognizes
`shield_hit`, not `guard_hit` (`fighter_program.c:114–126`). The smallest
canonical migration is moving its body to a root `func ToadGuardHit()` and
passing that helper at each guard recreation. No static hook addition or
alias is needed. Existing `on shield_hit` plus `track_shield_hit()` is already
available for authors who want a declared event, but constructor helper
binding should not depend on a source-text scan or mandatory legacy event.

Implementation ownership: new guard Read/Write/Invoke unit; appended catalog
and property dispatch; narrow native constructor slot clearing and Fighter
ShieldHit continuation guard; canonical Peach helper rewrite; guard docs and
reference exporter argument contract. Snapshot/reset support for the
ShieldHit code ID is already present in the actor. Focused evidence should
compare native descriptor/reset order and all fields, test invalid-volume
no-op preservation and exact CodeRef rejection, and execute the real
ProcessHit callback path with motion change, fault, physical retirement and
schema replacement. Controlled collision-contact inputs are a fixture
boundary, not full in-game counter acceptance.
