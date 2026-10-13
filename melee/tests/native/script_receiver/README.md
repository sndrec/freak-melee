# Retained receiver integration

The source fixture is compiled through the complete direct bank pipeline and
executed by production `NativeScriptExecution`/VM on registered native Fighters
and an Item allocated from the actual `NativeMatchPool` GObj implementation.
This proves receiver behavior by observing native positions and owned state,
not by comparing instructions with an expected instruction list.

The generic runtime distinguishes:

- `creator`: retained creator, independent of reflection/current damage owner.
- `owner`: the creator only while `Item.owner == creator`, preserving the authored
  attached-owner contract. Once captured, an RHS owner transfer does not retarget
  that token.
- `current_owner`: current live Fighter damage owner, selected once.
- `holder`: current owner only when `fighter.item_gobj` is this Item.

Source Items use actual `nativeItem_Creator` checks and their captured generation,
kind and Definition. They do not require legacy article logic/union data.
Unscripted legacy articles use the actual `nativeArticle_Creator` contract.
Effect creator selection uses `nativeModelEffect_Creator`; the current isolated
fixture exercises Item/Fighter identities and does not claim effect scheduling.

Receiver records are invocation scratch with caller-reserved arrays. Values hold
opaque slot/serial/class tokens, never native pointers. Records retain actual
GObj pool references, game creation generation, kind/Definition, and optional
program/storage/layout/parameter identities. Native field access can use another
fighter kind; owned state/attributes, helpers and callback code identities require
the exact expected root program and fighter kind. There is no field-name/schema
guessing across programs. Qualified helpers execute direct code IDs in the actual
receiver view; reading their Callable value returns a validated code ID.

The compiler captures receiver roots before index expressions/RHS/arguments.
`ValidateBinding` then checks an assignment target after every selector and
before RHS evaluation. Native reads/writes/calls validate again before publishing.
Every guard uses the captured record, never the source's potentially changed
owner relationship. Missing `.exists` is explicit false; other missing/stale
receiver accesses are errors. Parameters are read-only. Owned array bounds and
native property writability are validated without an observable getter/write.

The parameter cases deliberately supply different root values to the creator
(2), reflected damage owner (7), and executing Item view (11). Source functions
read `creator.params`, `owner.params`, `current_owner.params`, and `program.attrs`
through production lowering and Execution/VM. Reflection preserves creator
reads, redirects new current-owner reads, and rejects the attached-owner read;
the executing program's immutable root read remains independent of both.
Changing a retained owner's parameter identity rejects the read without
publishing a value. Unknown fields and parameter writes fail source validation.
These controlled views verify receiver selection, not actual package asset
loading, reflection collision callbacks, or Bowser flame gameplay.

The strict fixture uses named argument descriptor fields in the production
catalog and includes the current binding resolver inventory. Unrelated engine
operations fail or abort if reached; they cannot provide successful synthetic
behavior. Collision-box extraction uses the native body rather than a duplicate
fixture definition. Receipts pin all source headers, extraction scripts and the
compiler as well as the executed translation units and input script.

Nested invocation frames reserve disjoint record ranges and suspend outer
handles. Pop restores the outer frame; serials are monotonic, so popped/reused
tokens cannot become valid again. Identical records are interned within one
frame: the 4096-capture test adds no records after the first capture. Frame/record
and serial exhaustion return diagnostics rather than clamping execution.

Engine Object inputs use tag `0x40000000|source_class` with GObj pool slot and
slot generation. `CaptureExternal` records a source in the trusted scheduler's
nonzero scene epoch; `Import` requires that full epoch separately and publishes
an invocation token tagged `0x80000000|source_class`. `Export` validates a retained
record and produces the original engine reference for a native primitive.
`ResolveExternal` checks caller, class, scene and pool slot before returning a
native pointer through native API out arguments. External inputs are synchronous
and never persistent. No scene-epoch truncation or inference from cached values
is permitted. Epoch-zero entry frames support existing isolated non-receiver
programs, while actual capture/import/export requires the real pinned scene.

Tests cover RHS owner transfer, exact cross-owned state and parameter reads,
native motion/command/floor fields, explicit helper execution, nullable holder,
retirement/generation change inside an index helper before RHS, result preservation,
nested frame suspension, schema mutation, readonly/unknown/type/bounds negatives,
helper code-ID reads, external import/export/class/epoch checks, and GObj address
reuse with the same game generation but a different actual pool generation.

```powershell
python tests/native/script_receiver/run.py `
  --compiler C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe `
  --output A:/FreakMelee-test-archives/script-receiver-fresh
```

The runner extracts actual native creator/command/common-animation bodies and
records their source hashes. Motion installation, geometry, immutable Definition
observation and animation leaves are controlled fixture boundaries. A generic
motion-change hook induces owner transfer/retirement for the source-executed
ordering tests. Unrelated binding families abort if invoked. This fixture is not
complete package/asset/entity-scheduler or game acceptance. Build/run subprocesses
are hidden and output goes to a fresh directory on A:.

Source motion helpers now use the package-embedded actual compiled bank and an
explicit owned motion layout. Two controlled Fighter Views share that bank and
retain separate generation/storage identities. Source/data/program selection is
controlled fixture metadata; the SourceFighterLayout validator, Execution, VM
and receiver capture are substantive production units. No Definition fallback
is used for these source motion calls.
