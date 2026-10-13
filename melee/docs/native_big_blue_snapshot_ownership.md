# Big Blue track offsets and loaded mode-flag ownership

Big Blue's twelve track records now live in actual `stage_hazard_state.c`
storage, with their original indices, padding and zero deltas. Every source
consumer uses that table directly. Setup writes offsets through a local entry
pointer; auditing only direct global assignments missed this mutable payload.

The existing stage-hazard component copies the full table and its cached
parameter pointer. Scene setup freezes the joint/start/end indices and padding;
only the offsets change during history. It also captures/restores the actual
loaded parameter's `x18` mode flag, which source runtime code changes from zero
to one. This field is read/written in its original backing, not redirected to a
live mirror. Other parameter bytes and variable payloads are not implicitly
claimed. The parameter address stays leased for the scene/history lifetime.

Candidate identity/reserved fields validate before writes. Capture rejects
output/length pointers overlapping actual state or the borrowed parameter
record. Load restores track records and the one owned parameter field after
complete local validation, with no allocations/callbacks/physics/draw. The
component is now 5,728 bytes, including a scalar field/reserved word; there are
still thirty live census components. No format compatibility layer is retained.

`tools/audit_stage_snapshot_state.py` adds a conservative compiler-backed
candidate audit for all 29 selectable stage modules plus generator/car/shared
stage helpers. It includes private header storage and static locals. Pointer
aliases are tracked as flow-insensitive unions, so they reveal potential writes
but also deliberately produce false positives. Call/address/return escapes are
candidates, not proof of mutation or immutability. External storage/pointees,
interprocedural/field-sensitive alias agreement and full allocation ownership
remain separate audits. Ambiguous eligible locations are reported. No game runs
or complete snapshot coverage are implied by this inventory.

The widened audit identifies the shared generator's private header state and
direct data allocation as remaining work. Variable generator configurations,
Fountain reflections, mutable attributes and other world/HSD/HUD/effect globals,
multi-heap/resident closure and complete aggregate restore/history/performance
acceptance remain unfinished. Production still uses the broad page oracle.

Initial receipts under `A:/FreakMelee-test-archives/`:

- `big-blue-oct11-owner-a`: 42 added checks/six malformed candidates, exact
  first/last initializer values, twelve track records, sixteen repeated
  controlled updates, exact actual mode-flag restoration without changing other
  parameter bytes, candidate identities, overlap and activity/epoch retirement.
  Actual snapshot/storage; stage/GObj/parameter/idle setup is controlled.
- `big-blue-oct11-gm-a`: existing connected production control/player/camera-target
  restoration fixture passes with the expanded component.

- `stage-storage-oct11-audit-c` and `stage-storage-oct11-audit-verification.json`:
  32 modules, 241 eligible storage declarations and 115 conservative candidate
  roots; no reported eligible location gaps. Source/header/compiler/tool hashes
  are pinned. The original Big Blue alias-write and generator header storage
  are explicitly checked. This is a static inventory, not closure acceptance.
- `big-blue-oct11-body-verification.json`: all 65 source function bodies retain
  their decisions/arithmetic with direct actual-storage naming only; the entire
  twelve-entry track initializer is preserved.

- `big-blue-oct11-stages-a`: Big Blue, Poké Floats and Final Destination
  old/current hidden/muted rendered CPU scenarios, 4,800 samples each. Exact
  populated traces match; forty stage-private capture/validation boundaries
  pass per scene. Big Blue mean Save 2.845 us, Validate 0.522 us, 5,728 bytes.
- `big-blue-oct11-eight-a`: stock/independent interpreted eight-actor scenarios
  pass all thirty components at ten boundaries each; previous/current and
  diagnostic-on/off traces match. No live Load in either cohort.
- `big-blue-oct11-independent-verification.json`: source/frozen/live input hashes,
  original body/initializer preservation and actual/host compiled sections pass.
  Four build targets and focused GObj-consumer CTest pass.
