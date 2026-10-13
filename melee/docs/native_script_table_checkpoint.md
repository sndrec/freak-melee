# Ordinary interpreter constant-table checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The ordinary source compiler and VM now support generic typed constant tables.
Fox/Falco's blaster presentation scripts already used these declarations, but
the interpreter treated them as unsupported entity metadata. The implementation
supports root, Article, Item and model-effect scopes; it contains no blaster
lookup values or fighter-specific operations. See the [table reference](scripting/tables.md).

Tables are evaluated once into the pinned program index, with fixed capacities
of 64 declarations and 2,048 four-byte elements. Reads use a compiled table ID
and one checked integral selector. Local entity tables shadow root tables;
`program.table` selects root data explicitly. They are immutable and shared,
not per-actor state or mutable snapshot contents. Package preparation recognizes
their source declarations in the common and entity validation paths.

The same constant evaluator now serves parameter defaults and table elements.
Existing integer width/conversion semantics remain unchanged. Floats in tables
must be finite. Empty lists/elements, duplicate names, dynamic defaults, unsupported
scopes, float indices, nested indices and assignments reject. Runtime range or
scope failures leave the read output untouched and fault the invocation.

## Checks and boundaries

- Seven Windows targets build. Script and parameter CTests pass.
- The connected compiler/actor/VM and common-scheduler cohort passes **102,541
  checks / 9,661 cases**, adding **169 checks / 46 cases** for tables. The actual
  fighter actor reads computed indices through the production execution path;
  negative/out-of-range reads fault. Typed integer/Uint/Bool/Float values, scope
  shadowing, explicit root reads, immutable flags, rejected statements and both
  exact/overflow fixed capacities are checked. An indexed table alone consumes
  zero mutable state bytes. Entity scope resolution and direct reads execute;
  this cohort does not launch a live blaster Article or rendered match.
- Broader scheduler coverage caught an initial semantic-domain collision with
  event payloads. The table domain is now separate, and the whole existing cohort
  passes. Rejected nested indexing was also tightened at preparation time.
- Fresh isolated Fox and Falco drafts use the existing shared importer to copy
  assets, convert DAT tuning into owned declarations and omit the other variant's
  stock Article declarations. Both now pass metadata validation and reach all
  **257 / 255** bodies. Each reports **47 body diagnostics**, including accessory
  scheduling, parameterized signals, factory operands, physics/presentation,
  stage conversation and receiver/item operations. **Neither package is accepted.**
  These are the currently reported failures; a body reports its first failure.
- Normal installed content and user drafts are unchanged. Roster preparation
  remains at the earlier 20/26 checkpoint. This is a capability milestone, not
  complete Fox/Falco behavior or rollback acceptance.

## Evidence

- `A:/FreakMelee-test-archives/script-constant-tables-oct10-g/receipt.json`:
  SHA-256 `49515445cd20ccca3946ba1828160e3aaa560ef0112c166b60289df43b867335`,
  **1,832** independently rehashed input pins.
- `A:/FreakMelee-test-archives/fox-falco-owned-tables-oct10-final/receipt.json`:
  SHA-256 `3debe12183d879eb9245269020cd6308729f4a5a78f75fb40c0befb87c85757a`.
  It repeats the owned-draft audits with the final executable and records the
  independently verified connected-cohort receipt. The earlier draft creation
  and import logs are under `fox-falco-owned-tables-oct10-a`.
- That earlier directory's `body-inventory.json` groups the reported operations
  for the next batch; it is an inventory, not an acceptance result.

Continue grouped migration of the remaining shared operations and source recipes,
then test actual fighter/Article behavior. The overall goal remains active.
