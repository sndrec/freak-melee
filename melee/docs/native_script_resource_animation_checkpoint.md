# Interpreted resource joint animations

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

All five methods are now available on `item.resource(index).joint("path")`:
`add_animation(Name)`, `request_animation(frame)`, `advance_animation()`,
`remove_animation()` and `set_animation_rate(rate)`. The shared retained-resource
backend executes the original HSD operations; no duplicate animation engine or
fighter-specific native primitive was introduced.

The source compiler resolves bounded literal paths, exact owned bundle names
and typed arguments into immutable call recipes. The scene derives the allowed
root/subtree/bundle tuples and their native controller/track provenance from
those recipes. It deduplicates identical tuples, rejects an Add with no compatible
declared root, and retains the existing fixed application/origin limits. Tick
execution has no text lookup, recipe generation or new allocation path.
The former external recipe-input API was removed and its call sites updated.

Runtime validates the caller, collection/node generation, selector bytes,
prepared bundle ordinal and native graph/controller provenance. Missing or
incompatible healthy resources return false; invalid recipes or retired owners
do not publish an output. Model changes still use the existing fixed pools and
retained-history teardown guards. See [the scripting reference](scripting/article_resources.md#animated-resource-joints).

Ness's PK Thunder trail choice now uses explicit script branches naming its four
authored child declarations. The original conditional ordering and spawn inputs
are preserved. This avoids conflating stock numeric ItemKind values with
package-local declaration identities and adds no fighter-specific factory API.
The choice remains ordinary editable script logic.

## Verification

Seven Windows targets build and the focused script/parameter CTests pass 2/2.
The frozen linked resource cohort passes **7,031 checks across 25 cases**:

- Imported Arrow static controls and existing lifetime/collision/history cases.
- Imported Boomerang root and joint bundle, running all five methods through the
  real Item VM against direct original HSD calls: **39 exact graph/controller/track
  comparisons**, including fractional/backwards requests, rates, replacement and removal.
- Three changed selector/bundle/recipe refusals preserve graph state and output;
  active-operation fences remain balanced and OS heap free space is unchanged.
- Both profiles retain actual Regions/Pages graph replacement, complete-region
  hashes and deterministic replay, with four completed frames/two restores total.

The read-only roster census still accepts 24 of 26 non-Kirby forms. Ness's
canonical audit compiles all **418 indexed bodies** without a body-level error,
but its package rejects at `up_smash_enter`. Both up/down smash entry hooks lack
interpreter scheduler integration. Samus remains unprepared. Later package and
runtime checks can expose further work.

Receipts are under `A:/FreakMelee-test-archives/`:

- `source-resource-animation-oct10-b/receipt.json` for the linked resource cohort.
- `source-roster-resource-animation-oct10-a/receipt.json` for the read-only census.
- `ness-resource-animation-oct10-final/receipt.json` for current/frozen input,
  build and audit closure.

The earlier `source-resource-animation-oct10-a` run predates the three added
refusal cases and is retained separately. No game window or audio was launched.

## Remaining scope

The linked fixture controls collision/effect leaves and uses a small attachment
rig with real imported model/animation graphs. It does not prove GPU/audio,
material/shape resource playback, active-controller rollback, normal package
publication/construction or full Ness stock/clone gameplay. PK Thunder branch
selection has source and compilation coverage here, not a new native replay
comparison. Ness activation, its yo-yo glitch correction, and Samus remain open.
