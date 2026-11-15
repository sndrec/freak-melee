# Source Item model and lifecycle checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

This batch implements shared Article operations in the ordinary interpreter.
It adds no generated-C runtime or fighter-specific behavior API.

## Model operations

`item.reset_spin()` uses the owned descriptor's native spin joint and axis.
`item.scale_by_attribute(value)` multiplies the owned attribute scale and updates
the root. `item.set_instance_scale(value)` additionally refreshes hitcaps, grab
and collision ranges. `item.inherit_owner_scale()` follows the current damage
owner, which can differ from original creator and attachment controller.
These are separate native operations; they are not aliases to `item.set_scale`.
Prepared primary joint selectors now expose translation and single-node
`local_visible`, alongside existing recursive visibility. See
[Article models](scripting/article_models.md) and [Item API](scripting/item.md).

## Native continuations

`item.refresh_physics()` runs the actual immediate physics/collision sequence.
The engine retains source identity after nested callbacks before continuing.
`item.remove()` terminates the calling source invocation, including reentrant
removal while its Destroyed callback is already executing. Storage retirement
cannot affect a replacement actor that reused the old address.

## Direct source migration

Dr. Mario's controller accesses use `attachment_owner`, rather than confusing
that revocable relation with original creator or current damage ownership.
Single-child visibility uses the generic prepared joint property. The original
two-motion incapacitation predicate remains authored in the script, and live
rendered root scale uses `creator.model.root_scale_y`. Peach's eight-argument
creation call moves directly to the existing explicit velocity/creation-value
form, preserving zero initial velocity and the original collision policy.

## Acceptance boundary

Native client, content tool and separate reference exporter builds pass.
The final 40 selected integration checks and three reference tests pass.
Focused model evidence: 1928 checks, 34 new native spin/scale comparisons,
35 new rejection/continuation cases and 13 prepared selectors at
`A:/FreakMelee-test-archives/script-item-model-scale-final2/receipt.json`.
The actual frontend, VM and pool execute; actor/spec/storage views and animation
leaves are controlled. These checks do not validate retail DAT animation or
rendered appearance.

The grouped property suite passes 34406 checks, including 15 native root-scale
comparisons and 13 related rejection cases, at
`A:/FreakMelee-test-archives/script-root-scale-y-oct09-first/receipt.json`.
It mechanically extracts actual class ancestry and the native getter while
providing controlled initialized class metadata. It does not run the renderer.

The fresh preparation census is
`A:/FreakMelee-test-archives/source-model-lifecycle-census-oct09/receipt.json`.
Dr. Mario now prepares its full source: 111 functions, 35 imports, 2894
instructions. Mario and Zelda still prepare. Peach and Game & Watch retain
later shared API gaps; real secondary-resource admission remains open.

Complete fighter activation, actual asset animation/rendering and gameplay
remain separate acceptance requirements. No game was launched.

Connected lifecycle evidence: 232 checks in 29 cases with 23 actual source VM
signals at `A:/FreakMelee-test-archives/script-item-lifecycle-final/receipt.json`.
It runs actual actor storage, VM, fixed GObj/proc pools, native physics/collision
and removal bodies, including deferred deletion and reentrant Destroyed removal.
Article/model/attribute publication, geometry, animation, graphics/audio and
effect cleanup remain controlled. Its component snapshot requires matching
native Item/GObj/model/program peers; it is not full rollback acceptance.

The existing raw callback lifetime fixture now binds and recycles its actual
fixed-pool object slots; no unpooled source fallback was added. Its configured
build/test passes alongside the source actor fixture.
