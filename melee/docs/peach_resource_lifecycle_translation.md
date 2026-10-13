# Peach resource lifecycle and Turnip phases

This is an **unregistered source preparation**, with no production Peach
activation. Frozen31 generates `tests/native/peach_turnip_phases.fms`; the
available Turnip callbacks pass **329 actual-original/generated pairs and
1,218 checks**. The accepted constructor candidate from `66abe5eac` is unchanged.
This phase candidate has no constructor or pickup initialization and must not
replace the complete stock article table.

The fixture extracts nineteen unchanged production bodies from frozen31:
fifteen Turnip callback/math bodies, the shared falling, reflection and expiry
implementations, and Peach's vegetable-reference clear. State, appearance,
bounce, spin and interaction dependencies record ordered arguments. The actual
falling/reflection/expiry math and actual callback generation registry execute.
The creator resolver is a controlled generation-aware dependency. This is
caller/phase comparison, not a new validation of every engine dependency.

Coverage comprises 30 animation pairs, 270 physics pairs, 24 drop/contact/
reflection pairs, three interaction-reference pairs and two destruction-owner
pairs. The callback table explicitly has **no thrown callback and no collision
callbacks for states 2/3**. Resource-chain cleanup, Parasol restoration,
constructor/pickup, removed creators and callback-driven mutation of the
controlled dependencies are excluded from these 329 pairs.

## Turnip translation and remaining generic contracts

States 0/4 have no animation expiry or physics; state 1 decrements lifetime and
applies falling physics without spin; states 2/3 decrement lifetime and apply
falling physics followed by spin. Dropped selects state 3 with flags 6 and sets
the face appearance. Damage dealt bounces then selects state 1; clank and shield
hit bounce without changing state. Reflection and shield bounce preserve the
original unconditional false return. Interaction clears the remembered
creator only when it equals the incoming reference, then performs shared item
interaction. Destruction clears the creator's vegetable reference, independently
of current damage ownership.

The full draft is `.local/peach-resource-lifecycle31/planned-full-turnip-phases.fms`.
It is **not compilable in frozen31**; its new operation spellings are proposals.
Three exact generic contracts are still required:

| Contract | Actual source and required behavior |
| --- | --- |
| Float `item.throw_scale`, read only | Read shared `Item.throw_scale`; do not read an obsolete private item union. |
| Absolute instance-scale operation | Bind `it_80274484(Item_GObj*, HSD_JObj*, f32)` directly. Evaluate scale once, pass the root joint, write `Item.scl` and root XYZ scale, then preserve `it_80275534`, `it_80274DFC`, `it_80274E44` refresh order. Existing `item.set_scale` changes the model only. |
| Int-mask stage response | Bind `it_80276FC4(Item_GObj*, s32)` directly, retaining the complete mask from existing `item.sweep_stage_flags()` (`it_8026DAA8`). |

Thrown selects state 2 with flags 6, sets face appearance, sets hitbox-0 damage,
then restores remembered absolute scale. Dropped does **not** restore scale.
The collision callback first handles mask 14 with the exact response and state-1
entry, otherwise removes on mask 3. These are the low-mask response family,
not the `Collision.floor_mask/wall_mask` environment bit family. Named Int
parameters preserve these separate meanings and their branch priority. The
confirmed native mask-type correction is included in frozen31.

## Ordered resource cleanup and restoration ownership

`ftPe_Init_OnDeath2` calls **Parasol, then Toad, then Turnip cleanup**
(`ftpeach.c:453`). This order can affect the final Turnip decision: Parasol
cleanup can restore the previously held Turnip before Turnip cleanup checks
whether the remembered vegetable is currently held.

Parasol cleanup (`ftPe_8011D598`) invokes the explicit Parasol destruction
notification, removes the item (whose destroyed callback may notify again),
then invokes `ftPe_8011D518` again. That helper clears the Parasol reference and
death-3/damage callbacks first. If the saved held reference is nonnull, it
assigns it to the held slot, clears the saved slot, **shows, resumes, then sends
pickup notification with true**, and returns true. The Parasol destroyed
callback uses its **current owner**, and clears that owner only when restoration
returned true. Repeated notifications are real source behavior, not redundant
calls to discard.

The only saved-reference producer is `ftPe_SpecialHi_8011D424`: save the old held
pointer, hide it, pause it, then send drop notification with true. The producer
and restoration helper do not reattach the item or rewrite its owner/creator.
There is no original creator check or current-owner check. A live saved item
whose current owner is null, still the source, or another fighter is restored
by the same source body, with damage ownership unchanged. Existing
`items.bind_held` has stricter creator/author eligibility and therefore does
not reproduce this operation. The replacement binding policy remains pending
root review; this candidate imposes no new ownership restriction.

Toad cleanup explicitly notifies its current owner before removal and then
invokes the fighter clear helper again. The helper resumes a still-referenced
Toad before clearing the Toad reference and death-2/damage callbacks. The
destroyed callback repeats the current-owner notification. Existing
`targets.call(item.current_owner, "ExportName")` already supplies full-width,
recipient-owned dispatch; `creator.call` and creator-restricted `owner.call`
are not equivalent. Kirby's copied Toad behavior remains outside this scope.

Turnip cleanup only removes/forgets a remembered item when it is **currently
held and its kind is Turnip** (`ftPe_SpecialLw_8011CFA0`). It leaves rare items
and nonheld Turnips alone. The accepted constructor comparison already covers
that vegetable-only cleanup boundary; this document does not replace it with
a blanket resource clear.

Show and resume are callback-free in `it_8026BB20`/`it_8026B73C`. Pickup and the
earlier drop notifications can synchronously call fighter callbacks. The
original Peach helpers have no source/item generation checks around those
continuations, and the saved pointer has no ordinary removal invalidation.
Removed/recycled saved references and source removal inside notification are
**unprotected original risks**, not established successful contracts. Future
generic restoration must validate live handles and stop cached continuations
after callbacks while retaining the live-reference no-rewrite/no-reattach
dataflow. Existing generated source lifetime guards do not by themselves
establish a retained-item binding contract.

## Reproduction and evidence

Use frozen31, not the live SDK now undergoing resource-constructor changes:

```powershell
.local/baseline31/bin/native_content_tool.exe script-c tests/native/peach_turnip_phases.fms peach_turnip_phases .local/peach-resource-lifecycle31/generated.inc
python tests/native/peach_turnip_phase_fixture.py --root .local/baseline31/sdk --output .local/peach-resource-lifecycle31
cmd /c .local\peach-resource-lifecycle31\compile.cmd
.local/peach-resource-lifecycle31/phase-test.exe
```

The compiler command in `compile.cmd` uses frozen31 clang, the pinned SDK's
`src` and Dolphin includes, the generated include directory, and the real
`src/melee/ft/native_callback_lifetime.c`. It runs after `vcvars64.bat` with
`-std=gnu11 -O2 -fms-runtime-lib=dll -DMELEE_NATIVE=1
-D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS -D_DLL -D_MT`.

Evidence: `.local/peach-resource-lifecycle31/{phase_source.json,result.json,
run.log,compile.log,resource_source.json,resource_actual.inc}`. The resource
extract is a source-hashed audit artifact, **not an executed chain comparison**.
No package, registry, shared API, default template or gameplay acceptance is
claimed. Full resource cleanup waits for the generic retained-held restoration
decision; complete Turnip phases wait for the three contracts above.

## Complete Parasol producer composition (SDK41)

The separate [producer fixture](../tests/native/peach_parasol_creation/README.md)
now compares the complete original producer and attachment chain with generated
script behavior. It imports the retail parameter through the ordinary importer;
the script owns its Parasol request, creation, attachment and final slot publication.
Attachment explicitly requests the additional holder notification through
[`items.attach`](scripting/engine_items.md#attach-and-inherit-scale).

A fresh independent run passes 3,840 original/generated pairs, six separately
classified safety cases and 96,384 checks. The fixture executes 54 complete,
unchanged original bodies, including actual drop, attachment and both debug
notifications. The drop clears the previous held slot before pickup; the new
Parasol is published only after attachment finishes. The auxiliary slot is preserved.

Private allocation/initial instance construction, selected HSD command/state,
matrix, sound/stat and pure lookup boundaries remain controlled. An invalid rig
discards an unattached creation and restores a live prior held item; this is
authored safety behavior outside retail's valid-rig assumption. Full Parasol
travel, complete Peach behavior and default template activation remain open.
Receipt: `.local/peach-parasol-creation41-root-repro/result.json`.

The same complete comparison passes freshly against the corrected SDK42 Host
layout: `.local/peach-parasol-creation42-root/result.json`, with unchanged
3,840 pairs, six safety cases and 96,384 checks.

## Normal clone/import path (SDK45)

Peach is registered as a stock asset clone base, with 318 actions, 14 demo clips,
114 physical joints, all 53 retail semantic bindings, five costumes and all
five fighter article slots. This registration deliberately retains stock motion
behavior until the full authored composition is ready; it is not an independent
scripted fighter activation.

The frozen45 tool creates a real Peach project from the retail ISO, including
its copied CPU profile, then imports the staged lifecycle composition through
that project rather than a standalone DAT fixture. The compiled composition
passes 827 original/generated comparisons and 4,688 checks. Receipt:
`.local/peach-fighter-composition45-root/result.json`. Presentation, animation
resource lengths, CPU collision probes and common jump operations remain
controlled fixture boundaries; no gameplay or package installation is claimed.
