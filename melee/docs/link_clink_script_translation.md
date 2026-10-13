# Link and Young Link script translation

Fresh Workshop clones now use `examples/link_full.fms` and
`examples/young_link_full.fms`. Both declare independent behavior: the compiled
Definition has no donor gameplay kind. They own all four specials, tether/grab
policies, down-air rebound, sword-combo interrupts, shield, trail, lifecycle and
article decisions; Young Link also owns Milk and its taunt. Shared engine
physics, collision, animation, camera and allocation remain engine primitives.
Saved project scripts remain authoritative.

The raw copied bases retain stock fallback tables for asset import. Their
physical skeletons have 75/79 joints and 53 semantic bindings each, with owned
reserved-joint omissions and all five/six article assets. Compiled scripts
replace every original non-null gameplay phase and applicable Definition
callback. Common forward-smash startup selects the angled motion, starts the
animation, then installs the authored interrupt; no extended rows are invented
for common motions 58–62.

Root source comparison passed 16,690,480 assertions across the composed
Link/Young Link policies. Independent normal clone/import/build/bundle/verify
passed 721/776 ownership checks, including 70/78 original gameplay phases and
168/179 article callbacks. Receipts are
`.local/link-fighter-composition50-root-repro/acceptance.json` and
`.local/link-fighter-activation51-root-repro/receipt.json`. The shared smash
startup lifetime fixture separately covers callback retirement and replacement.
These are source and package gates; drawn gameplay, all costumes and network
acceptance remain separate.

`examples/link_clink_wip.fms` is retained as the small Spin/Bomb comparison input,
not the Workshop default. The full templates are assembled distributions of the
separately maintained policies in `tests/native/link_fighter_composition` and its
listed components. Workshop splits its working source into editable move files
and fighter state.

## Historical component notes

The following records earlier component boundaries and proposals. Claims that
bow, tether or lifecycle remain unported describe those earlier batches, not the
current independent templates.

## Copied assets and parameter import

| Field | Link | Young Link |
| --- | --- | --- |
| Code key | `link_clone` | `clink_clone` |
| Fighter root | `ftDataLink` | `ftDataClink` |
| Actions / demos | 314 / 14 | 314 / 14 |
| Costume joints | 75 | 79 |
| Runtime parts, including inserted sword joint | 76 | 80 |
| Costume joint root | `PlyLink5K_Share_joint` | `PlyClink5K_Share_joint` |
| Costume material root | `PlyLink5K_Share_matanim_joint` | `PlyClink5K_Share_matanim_joint` |
| Sound-bank mask | `1 << 16` | `1 << 7` |
| Spin effect endpoint | joint 24 | joint 26 |

The counts and roots were checked by importing the fighter, animation and
costume DATs from the local USA v1.02 ISO. The motion table has 21 entries,
starting at motion 341; Spin Attack uses motions 356 and 357. Each definition
exposes the copied stock motion table as the initial fallback table.

The physical costume and Spin Attack figatree node counts are 75/79. The
`PlCo.dat` omission descriptors reserve runtime slot 68/72 under parent
67/71; `ftParts_SetupParts` skips that slot without consuming a costume joint,
then `ftLk_Init_OnLoad` / `ftCl_Init_OnLoad` inserts the copied sword joint
from fighter item slot 6. The runtime parts tables therefore contain 76/80
entries. Costume validation must account for this omission, while copying
and using the complete runtime parts table: its final entry is a valid part,
and dropping it would break the rig. The copied-base `joint_count` remains
the physical costume count.

Runtime raw-part operations must use the active parts count, rather than the
physical costume count. The compiler audit found physical-count guards in
`native_fighter_script_primitives.c` for `guard.create`,
`bones.world_position`, `hurtbox.init` and `hurtboxes.set_bone`, and in
`native_fighter_item_script.c` for `item.attach_to_owner`. These reject valid
final runtime parts 75/79. Root integration is replacing them with a generic
host bone-count query. Existing effect spawning, fighter geometry and native
attachment helpers already check the active `FighterPartsTable.parts_num`.
Spin's endpoints 24/26 are unaffected by the final-part bounds issue.

The standalone `native_fighter_preview.c` diagnostic compares figatree nodes
to runtime parts, but explicitly supports Mario only. The actual Workshop
has a separate index risk: `native_shell_workshop_bones.cpp` collects physical
figatree indices and passes them unchanged to `nativeHome_WorkshopBoneScreen`
and `BoneParent`, which address runtime `fighter->parts`. Model/pose editing
also combines physical model indices with the runtime bone-local/matrix APIs.
After omission 68/72, physical indices need to skip the inserted runtime
slot (physical 68 maps to runtime 69 for Link; physical 72 maps to runtime 73
for Young Link). This source-backed authoring risk is separate from Spin's
runtime behavior and is not repaired by this fighter batch.

Run `import-parameters` separately for each copied project. Its six draft
`attribute` declarations become named values in `fighter_parameters.fmp` and
ordinary `attrs` reads in the generated module. The source DAT is an import
source; changing donor data after import does not change these owned values.
`spin_effect_joint` defaults to 24 in the shared template. One-time stock
parameter import retains 24 for Link and selects 26 for Young Link when that
template literal has not been customized. Explicitly changed defaults remain
unchanged; subsequent edits belong to the project's parameter file. Neither
endpoint has a semantic part in the ISO's `PlCo.dat` joint-to-part table (both
are `FTPART_INVALID`), so importing a named finger or thumb would select the
wrong joint. The first endpoint resolves `TransN` through the project's bone
map; the second uses its owned direct joint index.

| Owned parameter | Stock byte offset | Meaning |
| --- | --- | --- |
| `spin_landing_lag` | `0x30` | Special landing lag / freefall landing lag |
| `spin_air_horizontal_multiplier` | `0x34` | Air entry horizontal speed multiplier |
| `spin_air_drift_multiplier` | `0x38` | Air drift acceleration and freefall drift multiplier |
| `spin_air_speed_multiplier` | `0x3C` | Air drift speed multiplier |
| `spin_air_vertical_speed` | `0x40` | Air entry vertical speed |
| `spin_gravity_multiplier` | `0x44` | Air gravity multiplier |

## Spin Attack source correspondence

The source of truth is `src/melee/ft/kinds/ftLink/ftlinkspecialhi.c`, which
both stock fighters already share.

| Stock operation | Authored operation |
| --- | --- |
| Ground entry changes motion, starts animation, assigns accessory callback | Same operation order in `special_hi` |
| Air entry additionally scales horizontal speed, sets vertical speed, consumes jumps | Same operation order in `special_air_hi` |
| Accessory spawns effect 1211 on ground / 1212 in air only when `x2219_b0` is clear | `spin_effect`, using `effects.active` and the owned rig endpoint |
| Accessory installs hitlag pause/resume callbacks and clears itself | Same operations after the optional effect spawn |
| Ground animation end enters the ordinary ground finish path | `state.wait()` |
| Air animation end enters freefall with `(1, 1, false, drift, lag)` | Same explicit arguments |
| Both interrupt callbacks do nothing | Both authored callbacks explicitly pass |
| Ground physics uses friction when grounded, air physics otherwise | Same branch, sharing `spin_air_physics` |
| Air physics applies multiplied gravity, then friction-aware simple drift | `physics.fall`, then `physics.air_accel_friction(0, accel, max)` |
| Ground collision loss forces air and consumes jumps, preserving current frame | `physics.force_air_and_use_jumps`, then motion 357 with `0x0C4C508A` |
| Air collision checks ground/ledge with direction 0, then special landing or cliff check | Same branch and call order |

The original stock air landing callback casts `dat_attrs` to `Fighter*`
and reads `facing_dir1`. That member's original GameCube offset is `0x30`;
native pointer widths move it, making the cast read the wrong attribute on
Windows. Both the authored callback and the repaired native comparison must
read `ftLk_DatAttrs.x30`, the landing-lag attribute also used by freefall.

The edge transition deliberately uses `ftCommon_8007D60C` via the existing
generic force-air operation. Ordinary ground-to-air uses a different jump
count and ECB lock. The continuation flags preserve hitboxes, effects,
material/color state, subaction progress and the current animation frame.

## Validation boundary

`tests/native/fighter_link_spin_script_test.c` links the actual stock
`ftlinkspecialhi.c` against recording engine stubs and loads the generated
script DLL with the same host operations. It compares callback call traces
and relevant fighter state for both source families and both entry modes,
including effect endpoint selection, entry velocity, jump consumption,
ground/air physics, animation completion, frame-preserving edge continuation,
special landing and cliff branches. It also rejects a host missing required
drift operations and checks unrelated motion callbacks remain inherited.

This verifies authored decisions and the engine arguments they produce. The
stubs do not prove full engine integration, graphics, hitbox timing, controller
feel, rollback replay or live gameplay. Runtime acceptance is a separate step.

`tests/native/link_clink_spin_parity.py` prepares two independent copied
projects, imports all six DAT values into owned parameters, asserts the
automatically imported effect endpoints (24/26), and rejects remaining direct
DAT reads in the generated module. It records ground/air entries with both
facings and candidate edge/direct-landing paths for each fighter, then compares
stock and packaged-clone traces in hidden accelerated matches with normal
drawing. Coverage checks require the intended motion, facing, ground finish,
continued edge frame and direct special landing; a matching replay that misses
its intended path fails. Each trajectory must execute the requested branch
before being recorded as runtime acceptance.

The isolated compiled-stock callback comparison passed after repairing the
native landing attribute cast. The first full-owned package attempt exposed
the loader's physical-joint versus runtime-parts count guard before any game
was launched; the loader now owns omission records and validates physical
and runtime counts separately.

The full-owned runtime batch in `.local/link-spin-owned-20260930-rig` passed
16 cases: ground entry/completion, air entry/freefall/landing, direct special
landing, and frame-preserving edge continuation, with both facings for both
fighters. Each compared 650
controller samples, 1298 fighter states and 649 RNG states exactly, including
pose, hitbox, hurtbox, visibility and action traces. Import assertions selected
owned endpoints 24/26, and generated Spin code contains no DAT attribute reads.
Games ran hidden and accelerated with drawing enabled. The copied host first
failed UI initialization because its `ui` directory was missing; that was
corrected before the accepted matches.

The first four edge fixtures entered Dash and then jump instead of ground
Spin; the second fixtures entered ground Spin but
friction stopped Link at x=85.037 and Young Link at x=85.265 before leaving
the platform. The ISO's Final Destination floor ends at x=85.565697. Both
attempts failed the mandatory 356-to-357 coverage requirement and were not
accepted. The final fixture places each fighter at x=83.8 in its facing
direction, then enters Spin with four walk samples. All four final edge
cases matched exactly and passed the required continuation/facing/nonzero
preserved-frame checks. The actual stock callback comparison separately
covers jumps and ECB state. Retained failures are in
`results-first-trajectories.json` and `results-second-trajectories.json`;
`results.json` records the four accepted edge reruns, and
`acceptance-results.json` combines all 16 accepted cases. These checks cover
the exercised paths and controller samples; they do not establish complete
fighter or article parity beyond this Spin Attack batch.

## Bow and arrow dependency map for the next batch

Bow fighter source is `ftlinkspecialn.c`; article sources are `itlinkbow.c`
and `itlinkarrow.c`. The fighter's six states are motions 344–349. Its charge
gate is command 2, its arrow spawn cue is command 0, and its release cue is
command 1 in the **collision phase**. Charge increases by one only after the
gate and clamps to attribute `0x00`; loop interruption sets maximum charge.
Releasing B selects the corresponding ground/air end motion. The arrow and
bow must be independent ordinary handles, with removal callbacks resetting
them rather than reading the Link union.

Stock article slots remain:

| Slot | Article | Kind attribute offset |
| --- | --- | --- |
| 0 | Bomb | `0x48` |
| 1 | Boomerang | `0x2C` |
| 2 | Hookshot | `0xBC` |
| 3 | Arrow | `0x0C` |
| 4 | Bow | `0x10` |

Generic attachment creation/release, item state changes, owner motion queries,
owned-handle cleanup, named article parameters and model-joint transforms
already support the bow attachment and arrow charge/launch math. The bow's
seven article states correspond to the six fighter bow motions plus an
inactive state; crossing ground/air preserves its frame if the previous state
is the paired counterpart. End states expire at frame 0 or at least frame 24.

The remaining article operations must expose measurements and mechanical
attachment updates. They must leave launch, collision response, timers,
damage, visual decisions and state transitions in the script. Proposed
operations and exact source contracts are:

1. **Segment sweep:** `item.sweep_segment(previous_x, previous_y,
   current_x, current_y, hit, normal_x, normal_y, line, flags)` with output
   names bound to article state fields. A host operation with the contract
   `bool item_segment_sweep(Item_GObj*, const Vec3*, const Vec3*, Vec3*,
   int*, u32*)` calls `it_8026EA20`. Arrow launch performs an initial sweep
   between its two hand positions; flight collision sweeps between previous
   and current position. This operation must preserve the stock sweep's
   collision flags and line output even when the caller subsequently chooses
   not to attach. It must not automatically enter an embedded state.
2. **Moving collision lines:** `stage.line_exists(line)` maps
   `mpLib_80054ED8`; `item.follow_collision_line(line, normal_x, normal_y)`
   updates item collision data with `mpColl_80043558`, applies line velocity
   through `mpGetSpeed`, and returns its current normal via `mpLineGetNormal`.
   The script stores previous/current normal angles, adjusts arrow rotation
   by their difference, and decides whether a missing line removes the item.
   A line of -1 means zero velocity in stock; it is not a valid attachment.
3. **Contact fighter geometry:** expose the contacted fighter from
   `xCF4_fighterGObjUnk` through a tracked generic handle; expose its object
   validity/category (`it_80272D40`), whether its motion is GuardOn, Guard or
   GuardSetOff (`ftLib_80086A18`), model scale (`ftLib_800869D4`), and shield center/radius
   (`ftCo_80094098`). On shield contact the script computes the midpoint of
   previous/current arrow position, stores its angle around the target shield
   center, enters article state 2, and follows the shield surface each physics
   callback. The geometry query must not make those decisions itself.
4. **Separate article model roots:** stock arrow trails are two joint roots
   loaded from its article attributes `0x24` and `0x28`, not clones of the
   arrow's existing model. A generic `item.load_model(slot, root)` operation
   needs immutable project-owned roots established at asset load, with
   explicit unload, root transform and visibility access. Existing model
   cloning alone does not reproduce this. The script owns the previous
   transform history and command 0/1 visibility. Destruction must release
   both roots in every launched-arrow exit path.
5. **Expiry and contact visual events:** stock stage embedding starts
   vibration at `random_int(4) + 3`, with the alternating per-index angle
   table, and switches to a 300-frame visual tail when the ordinary lifetime
   expires. It emits effect `0x421` through `it_802787B4`, sets collision mode 2
   through `it_802756D0`, hides the model through `it_8026BB44`, and sets the
   destruction-effect suppression flag `xDCF.b2`. The collision-mode and
   model-hide operations already exist. The remaining flag and effect emission must preserve the
   stock effect helper's RNG calls. The script owns the threshold, table,
   RNG cadence and tail counter.

Bow also needs a readable/writable article animation frame and the ordinary
`Item_802694CC` animation update. `it_802AF32C` preserves the current frame
when changing between paired ground/air states, reloads the state, updates
animation, and calls `Item_80268DD4` to restore that frame. Existing
`owner.motion`, `owner.model_scale` and `owner.action_detaches_items` cover its
owner policy inputs. The script can author the seven-entry identity/paired
motion mapping directly, without calling a stock fighter query.

`xDCF.b2` gates `ItemSwitch` inside `Item_8026A8EC`, suppressing the default
destruction graphic and sound. A generic `item.suppress_destruction_effects`
property describes this contract. Arrow's `it_80273130` decrements lifetime
and tests `<= 0`, matching the existing `item.expire_tick()` operation. Effect
`0x421` goes through `it_802787B4` / `it_80278800`, whose zero-variance position
randomization still consumes three RNG calls; a direct `efSync_Spawn` alone
would lose those calls. These remain proposals for a later Arrow batch.

An authored arrow that only flies would omit moving-platform embedding,
shield sticking and its destruction paths. This batch therefore keeps the
entire bow/arrow path native until those generic contracts are available and
the corresponding source comparison harness covers all five article states.

The native vibration baseline is now repaired centrally.
`itLinkarrow_UnkMotion4_Anim` previously cast its five-entry motion table to
`float*` and read `[index + 23]` and `[index + 31]`, relying on the original
92-byte 32-bit table region and adjacent static data. It now reads the intended
`it_803F6A84[index]` table at offsets 0/8, like the separate
`itLinkArrow_802A81C4` helper. `tests/native/link_arrow_vibration_test.c`
compares the actual stock definitions across 288 cases: indices -1 through 7,
four fixed RNG values, both expiry results and tail counters 0/1/299/300.
Rotation, index decrement, RNG count, first-expiry effect/collision mode,
model hiding, two trail removals and the strict greater-than-300 removal
boundary pass. The fixture generator extracts unchanged selected definitions
from the live stock source; unrelated Arrow systems are excluded. This proves
the scoped baseline repair, while the complete authored Arrow remains pending.

## Composed article cleanup (SDK40)

The separate [cleanup composition](../tests/native/link_article_cleanup/README.md)
now imports and compiles normally for both families. Its Bow/Arrow declarations
match the stock DAT layouts and are imported into owned parameters. Cleanup
removes Boomerang, Hookshot, Arrow, Bow and Young Link's Milk in stock order,
including the actual destruction callbacks and recipient-owned exported cleanup.

A fresh independent run passes 18,432 defined stock/generated cases and 66
separate safety cases (15,802,352 assertions). The fixture executes the actual
item removal body and production reference, signal, generation, resource and
joint operations. Allocation/model construction, selected scene bookkeeping,
effect/audio cleanup and final HSD entity storage remain controlled boundaries.
Exact-borrower detachment and callback identity checks are documented safety
extensions where stock teardown leaves stale references or clears blindly.

This is an unregistered component candidate, not whole-fighter activation.
Receipt: `.local/link-article-cleanup40-root-verification/acceptance.json`.
