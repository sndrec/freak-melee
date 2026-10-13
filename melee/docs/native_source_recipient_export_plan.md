# Direct interpreted recipient exports

Source-backed contract, implemented 2026-10-09 and extended 2026-10-10. Connected fixture evidence and
remaining acceptance limits are recorded in the
[recipient-export checkpoint](native_source_recipient_exports_checkpoint.md).
The API is `targets.call/call_bool/call_int/call_float`
with an integral Fighter handle, literal exported name and up to 30 typed
argument values, available to Fighter
and Article source callers.

## Current interpreter boundary

The source parser preserves `export func`. Program indexing validates root
owner, up to 30 Bool/I32/U32/F32 parameters and Void/Bool/I32/F32 returns,
then prepares a bounded
sorted export table. The compiler registers literal recipient names; target
dispatch uses the recipient's table, local function ID and owned storage.

`native_script_binding_receiver.c:124` resolves `creator.helpers.Name()` to a
function ID in the caller's program. `native_script_receiver.c:397` then requires
the expected root program/kind and revalidates the recipient after execution.
This existing owned-helper contract is not dynamic recipient export lookup and
must not be repurposed into a fake same-program implementation.

Actual reusable engine interfaces:

- `nativeScriptActors_Lookup` / `nativeScriptFighter_View`: the bound destination's
  actual indexed program and live storage/schema view.
- `nativeScriptFighter_Helper`: code-ID execution through the actual actor's
  `runFunction`, Execution and VM, returning the actual Ok/Error/SourceRetired
  status. Existing owned-helper consumers explicitly require Ok.
- Receiver records and native match-pool references: pinned scene epoch, slot
  generation, native spawn generation, userdata, kind and program/storage identity.
- `native_script_execution.c:180` and `native_script_vm.c:81`: balanced nested
  invocation contexts, shared fixed scratch and unpublished results on failure.
  Runtime scratch has 64 frames and 65,536 registers, not an unbounded call stack.

## Reference behavior and adopted retirement contract

`native_fighter_events.c:68` implements C-reference dispatch using the recipient's
own Definition/export table. `native_fighter_script_code.c:361` validates the
literal name, computes its key while emitting and casts the handle to U32.

| API | Healthy result | Missing target/export or mismatched typed ABI |
| --- | --- | --- |
| `call` | true when delivered; discard any export return value | false |
| `call_bool` | exact Bool | false |
| `call_int` | exact I32 | 0 |
| `call_float` | exact F32 | 0.0f |

The C dispatcher does not require the recipient to survive the call;
`tests/native/fighter_events_test.c:142` explicitly exercises removal followed by
a Bool native return. Generated source lifetime guards, however, return neutral
values after their own source retires. The direct interpreter contract is:

- Recipient SourceRetired after delivery: `call` returns true if the caller
  survives; typed calls return their neutral default. Do not fabricate a native
  value from a result the VM withheld.
- Caller retirement/fault: publish no result or subsequent caller writes.
- Recipient VM fault, stale matched identity or malformed schema: explicit error,
  distinct from a genuinely absent target/export. Preserve output on failure.
- Never reacquire a target after callback removal/reuse. Ordinary field/link
  mutation is not retirement, and reflected/current owners may be foreign sources.

## Coherent implementation shape

Prepare a bounded sorted export table per immutable destination program containing
name key, destination-local function ID and exact Void/Bool/I32/F32 ABI. Require
exported root noncallback helpers with bounded typed parameters. Permit colliding keys only
with distinct exact names; reject duplicate names. Compile literal call names
into immutable keys; no tick
string hashing or parsing. If keys use hashes, establish collision-safe matching
against pinned names rather than silently equating different names.

At invocation, capture the actual full-width Fighter handle once. Resolve the
export in that captured Fighter's selected bank, even when it differs from the
caller's bank. Execute its local code ID with a typed VMStatus actor API, then
validate caller continuation before publishing the adopted result. Existing
owned-helper calls retain their stronger same-program/post-live contract.
Dynamic cross-program cycles cannot be proved by the current same-bank static
helper graph; runtime fixed-capacity depth/register failure must remain terminal,
balanced and tested, including self-calls.

Peach supplies the first connected acceptance cases: `peach_full.fms:148`
exports Bool `restore_held`, while `:164` exports Void `forget_toad`. Article
callers use `item.current_owner`, not immutable creator identity. Test separate
programs with the same export name but different local IDs, missing/wrong ABI,
full-width handles, self/nested calls, recipient deletion, caller deletion,
logical fault, physical pool reuse and source result publication.
