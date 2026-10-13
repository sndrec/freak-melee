# Interpreted actor entry

`native_script_execution.c` connects actual actor registry liveness, declared
owned storage, native primitive operations and scoped event payloads. It accepts
immutable code/function bindings and a live storage instance. Function ownership,
event identity and ABI are resolved once while loading; parameters are checked
when binding the actor view. Running performs no source-name parsing, parameter
table scan or allocation.

Event entry copies validated input values into a reserved synchronous payload
frame and seeds the anonymous VM parameters from that same frame. Exit balances
the frame even on interpreter error/retirement. Nested dispatch uses separate
invocation contexts and shared fixed frame/register pools. Native/property,
declaration and event semantic domains have distinct owners; event values are
read-only. Storage/layout/configuration/generation mismatch rejects the entry
before execution, and failed execution leaves the external result unchanged.

The fixture parses authored helper/fighter/article code, compiles real event
signatures and semantic bodies, and runs against actual Fighter/Item structs and
the actual native actor registry. It executes 128 fighter ticks, writing actor
position and persistent script data, reads incoming history IDs and article
signals, applies native event truth/default conventions and rejects wrong
ownership, payload types and retired generations. General engine primitives
link the same extracted actual-body fixture used by the kinetic tests; HSD
graph/audio/immutable-definition boundaries remain controlled.

The client scheduler and Workshop/package loaders have not selected this entry
yet. This is native execution integration evidence, not gameplay acceptance or
complete template/metadata coverage. Changing receivers such as creator/owner
require their own retained identity capture; this unit does not provide a
legacy handler, raw pointer script value or authored-function fallback.

Build `native_script_execution_test` and run `ctest -R ^native_script_execution$`
from the configured native build. No window/audio opens for this console fixture.
