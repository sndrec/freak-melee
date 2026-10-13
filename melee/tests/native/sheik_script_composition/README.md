# Sheik standalone source composition

This unregistered source composition has accepted Vanish59 evidence and a new
Needle plus unchanged Vanish comparison against frozen SDK61. Transformation states
361–364 and paired package/CSS behavior are excluded. No default script, clone
registration, independent declaration, installed module or gameplay acceptance
is implied.

The stock census covers custom states 341–364, all four article tables, lifecycle
callbacks, common/CPU dispatches and retail rig evidence. Needles occupy
341–348; Chain occupies 349–354; Vanish occupies 355–360. Every one of those
18 states has four nonnull stock phase callbacks, including intentional no-ops.
Camera uses the common camera callback. The four transformation states are
retained as an explicit excluded boundary.

Article slots from ftSk_Init_OnLoad are NeedleThrow=0, NeedleHeld=1, Vanish=2,
Chain=3. Slots 4/5 contain HSD_Joint pose graphs, not articles. The retail rig
receipt has 58 physical/runtime parts and 52 semantic roles; raw part26 maps to
semantic25, and raw part43 maps to semantic40. Raw array indices in the original
Chain/Needle constructors must remain raw indices. No guessed aliases are used.

The damage/death2 cleanup order is Needles then Chain. The neutral-only component
cannot claim combined cleanup until Chain is authored. The load decision enables
wall jump, copies attributes and registers article assets. Death resets the charge
and both retained item references, then model groups 0=0 and 1=-1. Color reset
restarts common color animation86 only at six charges. Item pickup/drop,
visibility and knockback callbacks use common engine primitives with stock
arguments. There is no Sheik-specific versus taunt decision. CPU transformation
requests via cpu.xF8_b5 belong to the excluded paired-form boundary; no other
Sheik-specific ranged/recovery branch was found in the stock CPU source.

## Required generic contracts

The Chain pose/capsule contracts below remain proposals. The two scalar surfaces
`input.previous_y` and `item.custom_floor_motion`, plus bounded
`item.disable_hitbox(index)`, are implemented in immutable SDK61 and used where
applicable. No Chain override is exposed.

* `input.previous_y`: Float RO, exactly input.lstick[1].y, alongside existing
  input.previous_x. Chain compares current-minus-previous values in its interrupt
  callback; a script variable updated only when that callback runs is not an
  equivalent previous-frame sample.
* `item.custom_floor_motion`: Bool RW, exactly Item.xDCD_flag.b3. Needle creation
  and both launch modes call it_8026BD6C. This bit suppresses common grounded
  displacement and participates in common entered-air response selection. The
  existing item.disable_surface_response writes xD5C and is not equivalent.
* Generic fighter pose operations over an imported, immutable joint graph and a
  bounded raw preorder start. Chain selects graphs in fighter item slots4/5,
  common animations305/308, and owns all blend decisions. The actual operations
  are listed in pose_contract.md; no combined Sheik update helper is proposed.
* Capsule current/previous positions need bounded XYZ RO/RW access to x4C/x58.
  Existing hitbox.set_world_position only writes jobj=NULL and b_offset via
  ftColl_8007B8A8. Chain separately writes current and initializes previous only
  when all previous components are zero. Its enable/disable/clear-victim ordering
  and fighter cleanup marker must stay explicit.

Existing item.sweep_segment already exposes its actual Boolean result as a sixth
scalar state output. Existing item.attach_to_owner(raw23,false) matches NeedleHeld
AB54 attachment without held-special registration. Existing item.physics_disabled
and item.begin_expiry cover Vanish BD24 and 7518C respectively. Existing cue.consume(0)
clears only the neutral entry's throw bit0; clearing all cue bits would change stock
behavior.

## Comparison boundaries

The intended fixture compares actual complete extracted original callbacks with
the final generated source and independently decoded retail attributes. Motion
entry/animation, stage queries, allocation/model construction and rendering are
controlled boundaries recorded in order. Healthy original pointer cases are
separate from authored removed/recycled-source safety. Incidental native RNG
evaluation ambiguity must be reported, not hidden by changing original bodies or
discarding comparison fields. The exact final receipt will list executed bodies,
counts, input hashes and pending boundaries.

## Executed source evidence

The original Vanish-only receipt remains unchanged:
`.local/sheik-vanish59-fifth/result.json`: 5,852 original/generated pairs,
449,847 checks and six authored safety cases. `vanish.fms` is the identical input
when composed into the SDK61 module.

The final combined SDK61 run is prepared for reproduction with:

```powershell
python tests/native/sheik_script_composition/run_needles.py --root . --baseline .local/baseline61 --with-vanish --output A:/FreakMelee-test-archives/sheik-needle-vanish61-final
```

Its two executable matrices compare the same generated module:

* Needles: 4,448 pairs, 540,890 checks and 12 safety cases. This covers both entry
  bodies, all32 fighter phases, charging/cancel/shoot/accessory, all Held phases,
  NULL and changed owners, both Throw initialization modes, all15 Throw phases,
  damage/clank/shield/reflect/destroy/interact decisions and ordered RNG draws.
* Unchanged Vanish: 5,852 pairs, 449,848 checks and six safety cases, rerun against
  actual complete SDK61 original bodies. The one additional check in each matrix
  verifies the combined module's declared article count.
* Combined total: 10,300 pairs, 990,738 checks and 18 safety cases. Exact finite
  float bits, Int timers/IDs, named gameplay fields, callbacks and events remain
  checked. No diagnostic digest or gameplay field is discarded.

The combined module owns 27 parameters (21 retail DAT attributes and six source
constants), 12 fighter fields in52 bytes, three article logics, and four callbacks
for each of the14 represented custom motions. Chain349–354 and Transform361–364
phases remain NULL in this unregistered base fixture. No clone import, installation,
independent declaration or recorded gameplay is claimed. The extractor preserves
retail `PlSk.dat` and independently decodes its big-endian fighter/article words;
normal registered Sheik clone/import remains pending.

The Needle matrix executes complete original neutral, Held and Thrown files,
actual shared math/lifetime/air-physics bodies and actual AB54 attachment. Sound,
rig attachment preparation/tail, model-tree flag traversal, stage responses,
allocation and motion/animation calls are controlled and recorded. The real
`native_model_joint.c` and `native_callback_lifetime.c` are linked. Stock combined
damage cleanup executes Needles then a declared no-active-Chain control; this
proves Needle cleanup composition, not the missing Chain cleanup policy.

Stock `ABS(table[HSD_Randi(8)])` evaluates the negative argument twice. The script
preserves that conditional repeated sampling explicitly. The original drop spin
multiplication's two RNG operands remain unsequenced by C; the script's explicit
order matches the frozen native compiler's observed order. This is native source
proof, not a retail PPC RNG-order claim. Failed generation/fixture attempts remain
in the numbered A: outputs; none is counted as acceptance.

Article-local helpers currently return void only. Typed article helper results are
a future generic language gap; Contact/Impact decisions are written directly in
callbacks, with exact original returns and no artificial persistent return field.
Chain still requires the pose/capsule contracts, and lifecycle/load/death/color,
common item notifications and owned rig metadata require later activation work.
No stock Sheik handler is called by the generated candidate.
