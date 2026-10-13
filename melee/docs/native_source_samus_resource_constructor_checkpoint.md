# Retail Samus resource constructor checkpoint

2026-10-11. Samus remains an unregistered source authoring candidate. This closes
the real-graph constructor and bounded palette-animation prerequisite, rather
than declaring the whole fighter independently cloneable.

## Production change

Article animation preparation now admits constant palette-selection tracks
(`TCLT`) and their imported palette tables. Complete stream validation uses the
same native scalar truncation rules as image-switch tracks. Tables are bounded
to 255 entries; values may select 0 through 254. Interpolation, nonfinite and
out-of-table values remain uncovered. Native value 255 is the default-palette
sentinel and is not an ordinary authored table index.

Live admission validates palette copies against the immutable animation recipe:
data pointer, format, entry count, terminal null, current selection, controller
origin and bounded controller endpoints. Renderer-assigned palette names remain
mutable. Native HSD allocation, update and removal code is unchanged. This is a
generic texture-animation capability; no Samus exception or unchecked graph
path was added.

## Connected evidence

The [resource fixture](../tests/native/script_item_resources/README.md) composes
all of the current Samus source into a privately prepared program. It runs the
real Fighter load hook and actual Item `Created` hook through the interpreter,
with normal parameter providers, published program lookup, the imported costume
rig and actual primary/resource model and animation scenes. Fixture setup now
provides real Fighter attribute scratch storage and retains source text for the
entire prepared program lifetime.

The comparison calls unchanged native `it_802B75FC` on copied mutable tuning and
separate actual HSD graphs. The passing receipt is
`A:/FreakMelee-test-archives/samus-resources-oct11-final/receipt.json`:

- 72 constructor pairs; 837,301 assertions.
- Standing/dash/air-catch motions 212, 214, 357 and 358; three scales; ground/air;
  ordinary and modified-mode sequence inputs.
- Exact graph topology, attachment joint 51, linked nodes, initialized collision
  data, scaled tuning, flags, reserved part 139 and primary animation setup.
- Joint, material and texture controller state after native frame requests and
  evaluations at 0, 1, 7, 23 and 60; 15,612 live palette-table comparisons.
- 17 palette census cases and 174 live-mutation refusals. Invalid descriptors
  preserve the output; altered palette data/format/count/table/selection/terminal
  are refused before an animation request proceeds.
- Native graphs and source collections are retired after every pair. Frozen
  input hashes remain unchanged and matched the live inputs at run completion.

The existing Arrow/Boomerang resource suite also passes on this production
change: `A:/FreakMelee-test-archives/script-item-resources-palette-oct11/receipt.json`
records 7,033 assertions, 25 cases, 39 interpreter/native animation pairs,
constructor-failure borrower cleanup and a captured-region restore/replay check.
The client and source-asset targets build. Four focused CTests pass (character
assets, script compilation, fighter parameters and resource metadata), and both
changed production modules pass strict syntax warnings. That compiler-test
build also exposed duplicate unavailable-Item relation definitions; the duplicate
copies were removed from its boundary file, retaining the shared fixture source.

This cohort constructs the Fighter and Item records explicitly. It does not
exercise real Fighter creation, the Item factory, attach/scale ordering around
the factory, the nine-state solver, modified homing/contact behavior, catch
callbacks, or rendered gameplay. It does not establish whole-match rollback.
The private independent declaration is only a fixture input; the production
candidate remains behind its normal independent-package gate. No game was
launched and no installed character package changed.

## Next closure boundary

Connect the complete fighter-to-item creation path, including attachment and
scale-after-attach ordering, then integrate the already composed solver/catch
and modified-mode branches with real resource ownership. Owned rig metadata
was completed on October10 and reverified against retail on October11 (41
semantic bindings,60 joints, no omissions). All callbacks and lifecycle
acceptance still precede independent registration.
