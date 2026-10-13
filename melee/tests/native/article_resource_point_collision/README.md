# Swept-point collision and synchronous resource retirement

This console fixture runs complete frozen SDK63 and current `mpColl_80048844`
bodies with their complete local dependency chain: previous/history setup,
substep calculation, bounding box, ECB interpolation, point contact dispatch,
surface response and `mpCollEnd` callbacks. The reference kernel is mechanically
renamed, without algorithm changes. It is native source comparison, not a retail
PPC execution claim.

It also runs complete current resource lookup/Begin, SweepStage/SweepPoint,
retirement/cancellation and teardown bodies with the actual
`native_callback_lifetime.c` registry. The point entry calls 48844; the existing
stage entry remains a separate 471F8 dispatch. A synchronous callback can retire
resources while the kernel still retains their `CollData`. The fixed collection
pin makes the set unavailable immediately and postpones freeing until its last
active sweep returns. No source-owner continuation is read after the kernel.

Run from the repository (the output directory must be new):

```powershell
python tests/native/article_resource_point_collision/run.py --root . --baseline .local/baseline63 --output A:/FreakMelee-test-archives/resource-point64-final
```

`run.py` uses frozen63 Clang and current headers/source, extracts the complete
bodies with `fixture.py`, compiles one console executable with the real registry,
and writes source/compiler/header hashes plus commands to `result.json`.
No game process, CMake build, imported fighter package or script activation is
involved. The archive is needed for the old-source reference; it is intentionally
not a mandatory global CMake dependency.

Coverage:

- 7,168 old/current pairs: four surfaces, all 128 combinations of timestamp,
  collision category, ECB restore and desired-size flags, seven displacements
  including the six-unit subdivision boundary, and successful/failed line
  resolution. Semantic fields are compared explicitly, including current/last/
  previous/requested positions, contact, four surface IDs/flags/normals, current/
  previous/desired/restore ECB, history flags and timestamp. No struct padding or
  diagnostic digest is compared or removed.
- Independent floor/ceiling/left/right anchors check real response application,
  subdivision count and remap phase. Skip/only/no-surface cases reject contact.
  An earlier contact followed by a missed final substep preserves accumulated
  flags while returning false, preserving the actual kernel's return policy.
- Live collection/resource ownership checks reject absent/recycled owner IDs,
  changed Definition/kind/logic, invalid node generation, missing nodes, retiring
  collections and invalid indices. Full-width high-bit fighter/item IDs are used.
- Both outer dispatches retire through Release, generation/creator/Definition
  cancellation, Clear, owner-generation recycling or owner removal. Nested point
  and stage sweeps keep the outer pin. Reentrant Clear during destruction does
  not double-free. Begin fails during retirement/destruction and can reclaim the
  address only after outermost return with a new collection generation.
- Freed ItemLink pages become `PAGE_NOACCESS` immediately at the controlled HSD
  free boundary. This catches kernel reads after premature release, rather than
  relying on a fake always-live predicate. Active-count overflow rejects entry.

Controlled boundaries are explicit: four analytic static axis planes stand in
for real stage topology/intersection/moving-line remap; allocation and package
publication are controlled; HSD free changes page access rather than destroying
a full model graph. The 471F8 probe is not an ECB algorithm comparison, but calls
the actual End body to test the same callback/pin lifetime. Model borrower
detachment is unchanged and outside this fixture (resource model roots are NULL).
No Chain behavioral script or drawn gameplay is accepted by this receipt.

The old resource source does not have the new pin and is not executed with
intentional destructive callbacks: that would be an undefined freed-storage
read. Healthy old/current parity belongs to the unmodified point kernel;
retirement safety is a separate new ownership contract.
