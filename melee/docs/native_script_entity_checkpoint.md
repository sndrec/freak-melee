# Connected interpreted entity runtime

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The direct interpreter now prepares immutable Article/Item programs and runs
their authored callbacks through the actual native Item scheduler. Ordinary
Workshop/project/package activation still uses the previous backend. This is a
runtime dependency closure, not completed package migration or roster acceptance.

## Implemented contracts

- Pure program preparation validates the candidate fighter layout and pinned
  entity assets, animation states, recipes, typed parameters and callback ABIs.
  Publication copies and rebinds resident records only after eligibility checks.
- Article declaration identity is separate from physical ItemKind. Multiple
  owned declarations can share a physical kind. Factory selection is lowered at
  source load; execution never searches script text.
- Actual Items own typed storage, callback/state code IDs and packed snapshots.
  The physical Item/GObj/animation/allocator owners remain separate.
- Native constructor, state setup, phase, interaction and retirement call sites
  dispatch interpreted code IDs. Missing source hooks have typed neutral results;
  they do not select a stock authored callback as a fallback.
- Creator, attached owner, current damage owner and holder are separate retained
  views. Pool generation, full scene lifetime, native birth and pinned
  program/storage identities protect reuse and reentrant changes.
- Shared held-item behavior preserves native pickup, throw, collision and
  removal contracts. Logical retirement and script faults stop continuation,
  including faults nested inside native state setup.
- Metadata-only entities have genuine zero-byte owners. Duplicate recipe fields
  and malformed/duplicate state or asset metadata fail before publication.

## Evidence and limits

The actual source fixtures and their controlled engine boundaries are described
in `native_script_item_actor_ownership.md`, `native_script_item_scheduler.md`,
`native_script_holdable_protocol.md`, `native_script_item_factories.md` and
`native_script_test_integration.md`. They exercise production compiler, storage,
VM, registry and scheduler implementations; asset/model/platform leaves remain
explicit controlled boundaries. Factory coverage includes all seven public
routes and 1,024 native constructor/free cycles.

The connected source-binding fixture additionally passes 17 cases/135 checks,
including a custom declaration sharing a physical ItemKind with another article,
retained owner writes, owned removal, held-item state and deferred retirement.
Receipt: `A:/FreakMelee-test-archives/script-spawn-bindings-oct07-i/receipt.json`.

The root client/content-tool rebuild and 32 focused CTests pass. A hidden/muted
eight-fighter regression completes 2,000 recorded inputs: four interpreted Popos
each complete 2,000 ticks, state hashes match their printed values, no actor is
faulted and the lifecycle census reports zero errors. It uses lifecycle/common
motion scripts alongside native stock behavior; it does not run fully scripted
Mario or Rook. GPU teardown emits device-destruction warnings after replay exit.
Receipt: `A:/FreakMelee-test-archives/live-entity-suite-root-final/receipt.json`.

No full match snapshot size, rollback deadline, live source-item rendering or
multi-PC acceptance follows from these results. Source-package loading must
jointly prepare assets, motion data, interpreter banks and holdable visuals;
effect scheduling, remaining Fighter contracts and source-qualified factories
still require implementation. See `native_source_package_loading_audit.md` and
`native_script_remaining_binding_closures.md` for those dependencies.
