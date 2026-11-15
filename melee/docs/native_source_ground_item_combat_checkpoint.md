# Interpreted ground arbitration, Fighter hitboxes and Item observations

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

This batch adds shared engine operations needed by Game & Watch's complete
source template. Fighter policy remains authored in `.fms`; ordinary runtime
execution uses the interpreter. The generated C path remains a test oracle.

## Fighter combat and ground input

`hitbox.state(index)` observes the exact native capsule state, including states
beyond enabled/disabled. `hitbox.set_damage(index, value)` dispatches explicitly
to Fighter or Article native setters; their object layouts are not interchangeable.
Fighter damage preserves native U32 narrowing, Y scaling, smash release and
staleness order. The exact post-scale/post-smash signed conversion is checked
before writes, rather than clamping damage or invoking undefined conversions.
Dynamic integral indices are a deliberate extension over the reference emitter's
literal-only admission; they must still address one of the four native slots.

Twelve `controls.try_*` calls expose ordinary smash/tilt/jab/jump/dash/crouch/
turn/walk checks. Scripts choose their order and short-circuit after success.
`state.crouch_wait()` enters the common crouch state. Neutral/back aerial landing
lag observations read the current Fighter attributes, including foreign physical
Fighters through retained receivers. Game & Watch's up landing deliberately
continues to read back landing lag, matching the original source.

Audited common attack, dash, crouch, turn and walk entries now retain Fighter
identity across nested transitions/animation before accessing cached state.
Source-attached up/down smash arbitration uses the common motion entry rather
than dispatching through a stock fighter kind. This does not implement Ness's
specialized yo-yo entry: `ness_wip.fms` declares an entry hook that the direct
interpreter still rejects. Do not infer specialized-entry or Ness acceptance
from this batch.

## Item observations and canonical source

`item.disable_surface_response()` clears the native response field;
`item.sweep_stage_flags()` runs the actual collision/position updates and returns
their native combined contact mask. `item.animation_finished` is a read-only
property using the actual primary-graph completion query. A missing primary
graph is finished; present roots require admitted JObj ancestry/extent and
retained graph/class identity before traversal. No secondary model is silently
redirected to the primary graph.

Game & Watch callback comparisons now use `callbacks.matches`, and its aerial
turn cue uses the existing `cue.reverse.consume()` (the native throw bit3).
Its 18 former
`owner.call` sites use `if owner.exists: owner.helpers.Name()`. Here `owner`
already means the retained creator when it remains the current damage owner.
Attachment ownership is a distinct relationship and clearing attachment alone
does not remove this eligibility. The existence guard preserves absent-owner
neutral behavior without an unguarded helper call or compatibility alias.

## Evidence and remaining boundaries

Native client, content tool, reference exporter and selected test builds pass.
All 40 selected integration and three C-reference tests pass. Run CTest discovery
sequentially: parallel invocations can race Aurora's generated discovery files.

Focused receipts:

- `A:/FreakMelee-test-archives/script-fighter-hitboxes-final/receipt.json`:
  11344 checks, 3250 cases and 3207 native comparisons. It runs actual source
  preparation/VM/actor/backend and native conversion/scale/smash/stale bodies
  with controlled player lookup, assets and unrelated engine leaves.
- `A:/FreakMelee-test-archives/script-item-observations-oct09-final/receipt.json`:
  2110 checks, 372 cases, 321 native comparisons and ten VM calls. It runs native
  sweep/position and graph traversal/class bodies. Actor publication and stage
  geometry/adjustment are controlled; injected post-complete native mutations
  test publication guards without inventing an authored collision callback.
- `A:/FreakMelee-test-archives/script-ground-controls-final/receipt.json`:
  13255 checks, 109 cases and 777 native comparisons, including nine healthy
  entry comparisons against pinned pre-change revision `b9ec6b132`. Actual
  input predicates, common entry bodies and VM lifetime handling execute;
  motion delivery, animation, held-item classification, angle/common flags
  and immutable package facts are controlled. Foreign lag reads and nested
  fault/retirement cases are included.

None of these console checks proves
imported DAT, full collision geometry, rendering or whole-fighter gameplay.

`A:/FreakMelee-test-archives/source-ground-item-census-oct09-final/receipt.json`
pins the rebuilt tool and ten full templates. Mario/Dr. Mario/Zelda still
prepare; Peach still needs recipient exports. Game & Watch advances from line548
through common Fighter behavior to line1534, where an Article uses `attrs` for
a root Fighter parameter that its local owner scope does not contain. Audit all
such parameter relationships before choosing a direct ownership-preserving API.
Do not silently make local attributes inherit from a fighter or substitute a
live recipient for immutable package-owned data.

The Article root-parameter relationship, prepared Item joint-world-position output call and foreign Fighter smash
charge observation remain open, alongside recipient exported calls for Peach
and actual secondary graph/collection ownership for other templates. Complete
fighter, rendering, match and final rollback acceptance remain separate. The
overall project goal remains active; commits are local only.
