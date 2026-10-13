# Samus clone closure: bounded implementation inventory

Source audit, 2026-10-04; implementation checkpoints updated 2026-10-11.
**Fresh Workshop clones now use the independent `examples/samus_full.fms`.**
Its complete composition owns Grapple Beam, common tether decisions, throw
presentation and load/death cleanup alongside the specials, four combat
articles and CPU policies. The WIP file remains a maintained source component
and focused-fixture input; it is no longer the Workshop's selected template.
The original read-only inventory and dated prerequisites below are historical.
Current checkpoints distinguish shipped integration from exhaustive acceptance.

## Reuse before extending the engine

The [normal-loader gameplay checkpoint](native_source_samus_gameplay_checkpoint.md)
now compares a private fully composed independent package through actual Fighter
construction, capture/throw victims, wall entry/effects, hang expiry and terminal
pull. The shipped full template passes 28 exact trace comparisons, including
modified modes, damage interruption and live-chain death/reuse, after correcting the generic effect
payload pointer and preserving the stock hanging-displacement expression order.
Blocked terrain, exhaustive retirement/reuse, exact ledge-available completion
and coordinated visual acceptance remain open. The full template is selected
for fresh Workshop drafts; independent preparation replaces authoring-reference
native callbacks rather than delegating to them. The rebuilt client also passes
ten 1,800-sample CPU comparisons and two repeated-missile cases through fresh
normal packages; the gameplay checkpoint records their distinct evidence limits.

The [2026-10-11 retail constructor checkpoint](native_source_samus_resource_constructor_checkpoint.md)
now exercises the full composed `Created` hook on the real imported rig and HSD
graphs: 72 native/interpreter constructor pairs, including material/palette
evaluation and malformed-table refusal. Its private record setup still excludes
the actual Fighter/Item factory and attach/scale orchestration. Reuse that
connected corpus rather than repeating the earlier controlled-constructor work.

The [connected factory checkpoint](native_source_samus_factory_checkpoint.md)
extends that proof through real Item creation, bone attachment, post-attach
scaling and removal. All 72 factory sequences pass after correcting the final
scale signal to use the attachment owner rather than a nonexistent held-item
holder. The parent Fighter is still staged; solver/catch, full Fighter lifecycle
and rendered gameplay acceptance remain open.

The [held-update checkpoint](native_source_samus_held_update_checkpoint.md)
adds native/interpreted held matrix, animation and pre-launch hitlag comparisons,
and connects the installed accessory callbacks through the actual factory.
It fixes generic animation admission after a supported root world-matrix override.
The owned retail rig map was already complete; integrated flight/catch and full
lifecycle/gameplay acceptance remain the closure boundary.

The [capture-retraction checkpoint](native_source_samus_capture_retraction_checkpoint.md)
adds72 real factory cases with staged deployment and holder capture state.
The interpreter matches the original mode5 retraction through completion,
including retained HSD graphs, RNG and endpoint borrower cleanup. An actual
grabbed victim, flight/contact and other constraint modes remain unaccepted.

The [ordinary-retraction and hitlag checkpoint](native_source_samus_retraction_hitlag_checkpoint.md)
extends the real-resource matrix to mode4, its ground/air speed and damping,
return to held mode, and576 post-launch hitlag constraint updates. No production
or scripting changes were necessary; flight/contact, wall modes, actual victim
and full lifecycle/gameplay still require connected acceptance.

The [controlled-flight checkpoint](native_source_samus_flight_checkpoint.md)
adds 144 mode-1/2 pairs on real graphs, comparing extension, falling motion and
controlled wall/floor responses through the first transition or eight updates.
It also corrects the native integer hang-duration attribute's host layout.
Actual geometry, aerial wall transition, downstream slack/wall constraints and
full victim/lifecycle/gameplay acceptance remain open.

The [slack and wall-constraint checkpoint](native_source_samus_constraints_checkpoint.md)
continues flight through its next states and compares modes 3/6/8, including
moving-line math, owner displacement and hang duration. Mode-7 pull, blocked
terrain/hang expiry, actual victims and full construction/gameplay remain open.

The [pull and retail-reference checkpoint](native_source_samus_pull_checkpoint.md)
reconciles the decompiled direction pointer with actual US 1.02 instruction
loads and compares 72 nonterminal mode-7 pairs, including explicit endpoint
clamps and ground exits. Terminal ledge/jump completion, blocked-terrain and
hang-expiry exits, actual victims and full construction/gameplay remain open.

The existing source evidence is substantial but deliberately split:

- `tests/native/samus_tether_composition/`: SDK52, 8,338 comparisons/442,325
  assertions for fighter AirCatch357/358, standing/dash Catch and common
  notifications. Grapple creation, endpoint reads/signals and removal were
  controlled boundaries. Its lifecycle draft was not accepted.
- `tests/native/samus_grapple_article_composition/`: SDK53, all nine healthy
  physics/deferred-accessory updates, hitlag constraints, interaction and
  resource disposal. Its 13,392 normal timelines include 1,284 that require
  a separately counted decompiled-C undefined-pointer substitution. Constructor,
  indirect animation installation and modified-mode behavior were excluded.
- Existing resource/joint/lifetime tests establish shared machinery; they do
  not turn the above controlled constructor/render boundaries into acceptance.

Already available: article-scoped typed helpers, scalar/array state, full-width
article references/signals, creator calls with typed results, physical
`input.up_held`/`input.z_held`, accessory2/3/death1 callback binding and identity
queries, `callbacks.damage_secondary`, positioned resource creation, matrices,
ECB point/shape sweeps, floor following, moving-line queries, scale, direct
joint animation controls, ledge/probe/fastfall, and the eight common Catch hooks.
The old API files' Up/Z and secondary-damage requests are resolved.

The pool already owns all nodes and exact endpoint borrowers: six collections,
declared capacity at most90, one collection per Item, reserved part139 beyond
the physical rig. `nativeArticleResource_BindExternalPart`,
`SetCaptureAnchor`, and `detach_borrowers` validate original creator generation
and Definition, clear only matching live part/capture/hitbox bindings before
freeing graphs, and handle Item/creator/Definition retirement. No linked-node
pointer API, native solver or additional teardown wrapper is needed.

## Core Catch and article composition

`ftCo_0D95.c:fn_800D9558/fn_800D9930` implement standing/dash schedules:
create -> bind accessory2/death1/accessory3 -> spark frames -> optional standing
obstruction check -> launch -> retract -> removal. `fn_800D9C64` removes on exit;
`ftCo_800D9C98` removes then clears death2, secondary damage and damage slots.
`ftCo_0D8E.c:fn_800D952C` disallows a second grab while the beam exists.
These decisions fit existing Catch hooks and ordinary retained article IDs.

`ftCo_CatchPull.c:fn_800D9CE8` chooses frame first, then Samus state5 plus the
endpoint capture anchor, then enters the pull motion. `ftCo_CatchPull_Anim`
waits for missing/completed article; Samus's `fn_800DA190` follow callback is
actually inert. Use authored anchor/ready hooks and `return false` for follow,
preserving the NULL accessory behavior. CatchWait release follows pummel OR
directional-throw acceptance, not grab-cut. Existing shared notifications and
guards cover this protocol; do not add another grab dispatcher.

`ftCo_AirCatch.c:ftCo_800C3B10/3BE8/3CC0` and its AirCatch/Hit callbacks contain
raw Samus-kind policy. Owned motions357/358 and `on air_tether_input` must replace
those decisions using the accepted fighter component. Passing a custom kind
through the old stock table does not select the raw Samus branches.

`itsamusgrapple.c:it_802B75FC/7C18` must become an authored constructor: choose
mode from the creator's ordinary sequence and ground/air state, derive scaled
lengths and integer count, construct each node in source order, select the
three repeating chain models and final endpoint, install animations, bind139,
attach to the creator and apply owner scale. At scale1 the motion-selected
counts are30, or45 for motion357. Only modified mode1 (grounded creator) doubles
them to60/90; modified mode2 does not double them. Keep the creator's ground flag
and motion coefficient separate. Keep explicit pool exhaustion/partial cleanup;
do not clamp. The source's mutable scaled attribute scratch belongs in local
article state, not shared immutable attributes. Physics0..8 only schedules a
solver; accessory2 executes it later, accessory3 remains a separate hitlag path.

## Genuinely missing generic contracts

| Contract | Source evidence and smallest useful shape |
| --- | --- |
| **Indirect immutable animation components** | `it_802B743C`, `samus_grapple_setup_tail`, `it_802B7C18` dereference element0 of pointer arrays at article offsets0x74..0xAC. `itArticle_ImportModelAnimations` currently treats each word as a direct graph. Extend the existing per-component recipe with direct/array mode and a validated constant index; import once into the author arena, then reuse named Add/Req/Anim. Five bundles fit the current limit8. Null and external references must remain distinct; never decode external-chain words as local graphs. The typed fighter importer already demonstrates the real array path in `import_graph_pointer_slot`. |
| **Resource-GObj effect spawning** | Ground sparks use0x3F3; state1 wall attachment uses0x41C then0x3F1 with facing, all against the actual endpoint GObj (`fn_800D9558`, `fn_800D9930`, `fn_802B805C`). Add typed position and position+facing effect statements on `item.resource(index)`, validating collection/node generation/author before direct effect dispatch. Current item/fighter effect calls use a different GObj. Actual async0x41C consumes only position; no ItemLink pointer parameter is needed. Preserve result provenance and continuation guards; scripts own RNG and ordering. |
| **Complete fighter hitbox initialization and reserved-joint binding** | Modified ground branches in `it_802B9328/99A0/9CE8` clear capsules and call `it_802B7160` with `it_803B8660`. Existing hitbox coordinate/damage/group/enable setters cannot initialize its full descriptor or attach it to139. Add a semantic immutable hitbox specification plus bounded creation/binding to an own rig or declared live reservation. Preserve damage/angle common helpers, group-dependent initialization/victim reset, offsets, scale coefficient0.003906, flags and cleanup marker. Do not expose arbitrary joints or raw packed command words. **2026-10-11:** packed-read prerequisite is resolved: the host raw damage read supplied17 instead of the console descriptor's0. The native initializer now reads named fields, and all three callers zero the formerly undefined trailing victim-only flag. See [descriptor checkpoint](native_source_samus_hitbox_descriptor_checkpoint.md). Generic immutable definitions and `hitbox.create` now initialize owned or live reserved joints, preserving native activation/history rules. Samus owns the modified-ground creation/mode transition in all three branches. See [creation checkpoint](native_source_hitbox_creation_checkpoint.md) for connected VM/resource comparisons and remaining constructor/gameplay limits. |
| **Nearest eligible fighter, including allies** | Modified Z homing calls `ftCo_800A4A40`: self exclusion, blast-zone margins using caller CPU half-size, `inlineD1` availability, **2D** `ftCo_800A1AB4` distance, first tie retained. Existing `cpu.nearest_enemy_at` instead excludes allies and `ftCo_800A1C44` candidates and uses a different distance; it is not a replacement. A substantive generation-ID query for this general eligibility search can directly refactor the actual search, returning0 on absence. Then use existing `targets.read_position`. No native homing solver or nearest-enemy substitution. |
| **Owned structured accessory asset and dynamic presentation controls** | **2026-10-11: implemented prerequisite.** The grapple archive decoder normalizes slot 4 into generic `FtAccessoryAsset`: owned root, base and up to 16 joint/material/shape variants. Native Samus consumes this type directly; the anonymous pointer-layout payload is removed. Generic `accessory` controls construct, scale, Add/Req/Anim, follow an own bone and release the standard transient model. Samus authors the complete throw-setup helper and invokes it through `on throw_presentation`, without a native special handler. Retail DAT identity/provenance checks and connected VM/native setup comparisons pass. Runtime construction/evaluation/follow leaves in that comparison remain controlled; rendered acceptance and a Workshop recipe editor are still open. See [accessory checkpoint](native_source_fighter_accessory_checkpoint.md). |
| **Conditional joint-animation rate** | **2026-10-11: implemented prerequisite.** Generic `accessory.set_geometry_animation_rate(rate, include_looping)` traverses geometry channels, including joint/reference/shape clocks, while excluding material and texture channels. False preserves looping clocks and flags, matching `ftSs_Init_80128770`; true updates them too. Samus chooses false in its authored throw helper. Actual HSD traversal/clock helpers and both loop policies are checked in the connected accessory cohort. See [accessory checkpoint](native_source_fighter_accessory_checkpoint.md). |
| **Post-common throw presentation event** | `ftCo_800DD398`: holder motion -> holder animation -> capture mask -> complete victim thrown setup -> Samus accessory -> `ftColl_8007B7A4` throw invincibility. Existing `on_throw_victim_setup` is inside `ftCo_800DE3FC`, before victim statistics/flags/animation; existing throw animation/finish callbacks are later. Neither has the original slot. **2026-10-11:** the generic typed `on throw_presentation` event is implemented at that slot, with retained reciprocal holder/victim lifetimes before continuation. Connected interpreter checks cover four throws and stock/owned victims; native motion/animation/accessory leaves remain controlled. The source-owned accessory helper is now connected; controlled setup comparisons and retail recipe import pass, while live construction/rendering acceptance remains open. See [throw presentation checkpoint](native_source_throw_presentation_checkpoint.md) and [accessory checkpoint](native_source_fighter_accessory_checkpoint.md). |

The external component inventory is preserved in
`.local/samus-tether-assets52/inventory.json`: chain shape animations and top/
start material/shape entries are external. Resolve them only from explicitly
owned input assets, or report the unsupported channel. Silently replacing an
unresolved channel with a fabricated/default graph is not rendering closure.

**Gate1 update, 2026-10-04:** generic indexed component recipes, primary-model
subtree animation selection/Bool attachment result, and resource-GObj position/
facing effects are implemented. Fourteen focused CTests and the full native build
pass. Real-disc import compares all8 local animation graphs and4 model roots
with the existing typed stock import. The seven external channels are retail
NULL bindings, proven by the source loader and an805-DAT symbol census in
`tests/native/samus_grapple_import/asset_audit.md`; explicit `None` matches stock.
The constructor composition in `tests/native/samus_grapple_constructor/` is
still unaccepted WIP, not production activation or full Gate1 closure.

## Load, death and immutable rig: composition rather than new APIs

`ftsamus.c:ftSs_Init_OnLoad` sets walljump, copies attributes and registers four
articles. Article declarations/bindings replace those registrations.
`LoadSpecialAttrs` reloads before multiplying x8, x74.y, x54, x58 and all height
attributes by instance scaleY. Use owned derived state and replace **every**
scaled read in specials/roll shapes; do not rescale immutable FMP repeatedly.

`OnDeath` sets group0 and resets charge handle/charge, missile serial, spin flag,
tether handle and modified-input sequence. The ordinary script fields need all
six explicit resets. Compose damage/death2 cleanup in original Charge -> Spin ->
Grapple order, then clear the exact callback slots. `lifecycle.pending.fms` is a
starting draft, not evidence. The primary-death reset and retained callbacks
cannot be supplied by native union writes.

The existing normal DAT audit establishes60 physical joints/60 runtime parts,
raw attachment51 without a semantic binding, and reserved139 as an extra part.
The owned base now declares the complete retail semantic map: 41 bindings,
60 physical/runtime joints and no omitted parts. The October 11 refresh of
`samus_rig_stock_compare.py` matches the current ISO common tables and all five
costumes. See [authoring checkpoint](native_source_samus_authoring_checkpoint.md).
Raw attachment51 remains a package-owned physical-joint parameter; do not invent
a semantic binding absent from the retail map. Reserved139 remains an extra part.
Keep four combat Article rows0..3 and the independent typed slot4 accessory
recipe separate. `source_kind=None` must wait for complete callbacks,
article constructor/modified mode and lifecycle; stock lookup is not metadata.

## Smallest coherent implementation/acceptance sequence

1. Implement indirect animation recipes and resource effects; compose the
   actual constructor/held setup and both existing fighter/article fixtures
   against real imported data. Test every allocation failure, model/component
   selection, Add/Req/Anim order,60/90-node mode, slot139 capture borrowers,
   retirement/reuse and free-all. Preserve the healthy nine-state solver.
2. Add complete capsule construction and eligible-target query; own Up/Down/
   Up/A sequence, modified mode1/2 and homing/contact branches. Test allies,
   availability, ties, Z input, each capsule field, existing unrelated hitboxes,
   reservation replacement and teardown. Reuse actual originals, measure Float
   deviations, separate `dir_ptr` undefined-C cases from defined comparisons.
3. Implement generic accessory metadata/controls plus correctly placed throw
   event; compare all219..222 throws, base/selected animation order, loop/rate,
   borrowed bone ownership and caller/victim retirement. Compose load/death,
   specials/roll, then normal independent package gate. Owned rig maps are done.
   Gameplay/drawing validation is a later coordinated gate, not inferred here.

## Actual-client resource and modified-mode checkpoint, 2026-10-11

The private independent composition passes 22 stock/interpreter gameplay cases,
including both facings of the completed input code, doubled ground chain,
endpoint capsule and modified wall hanging/pull. The trace now compares every
separate chain node's motion, collision history and model/material hashes:
142,480 count/node records match exactly. See the
[gameplay checkpoint](native_source_samus_gameplay_checkpoint.md) for the sealed
receipt and remaining lifecycle/terrain/visual limits. The later shipped-template
28-case cohort adds damage interruption and two live-chain deaths with reuse;
it does not mark the entire lifecycle matrix accepted.

The current article uses32 of32 helper slots (`ITEM_FUNCTION_LIMIT`) and
two90-element Bool arrays within the512-byte state capacity. Check the **complete** constructor/modified
composition before declaring those limits sufficient. At present only helper
headroom is proven small; no concrete compiler overflow has been observed, so
do not add a capacity API or compress/drop behavior speculatively.
