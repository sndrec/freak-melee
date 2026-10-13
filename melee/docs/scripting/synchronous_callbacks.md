# Synchronous callback dispatch and source lifetime

An engine operation can invoke another script before returning. State changes,
capture/throw transitions, item notifications, constructors, article signals and
recipient exports are examples. That callback can remove the original caller,
including indirectly through its owner or an exported recipient function.

The interpreter checks the executing source before entry. After an engine
operation marked as potentially invalidating the source, it checks the retained
object identity before publishing the returned value or any output, and before
continuing the caller. Identity includes the scene, object class and generation;
a reused address cannot stand in for the removed object. Pure queries and math
do not acquire a synchronous-dispatch effect simply because they return a value.

## Removal during a call

For example, a Boolean assignment invokes its target once:

```text
state bool notified
state int recipient

action Notify:
    on load:
        state.notified = targets.call(state.recipient, "resource_released")
        command(0) = 1
```

If the export removes this fighter, neither the assignment nor the later command
write occurs. If it removes only its own recipient and the caller survives,
dispatch still reports that the export was invoked. The calling fighter can
continue.

Retirement stops the current invocation and its returning local helper chain.
The VM reports `SourceRetired` and leaves its return output untouched; it does
not manufacture a successful false/zero result or execute later state writes.
Engine callback entry points handle that retirement according to their callback
contract. This is distinct from an absent recipient and from a script fault.

Captured external receivers and assignment destinations are checked against
their selected object and owned program/storage. An invalidated receiver faults
the operation without publishing a result; it is never reacquired through a
reused address. Prefer explicit existence checks when a creator or owner is
optional. See [article helpers](article_helpers.md).

## Recipient exports

[`export func`](functions.md#exporting-a-recipient-callback) publishes a root
fighter helper with up to 30 typed numeric arguments. Dispatch resolves the live
recipient's own prepared program and export. Same-package and foreign recipients
require explicit exports; there is no stock handler fallback or foreign private
helper access.

| Call | Result |
| --- | --- |
| `targets.call(handle, "Name", arguments...)` | Whether an export was invoked; its declared return value is discarded. |
| `targets.call_bool(handle, "Name", arguments...)` | The matching Boolean export's value. |
| `targets.call_int(handle, "Name", arguments...)` | The matching integer export's value. |
| `targets.call_float(handle, "Name", arguments...)` | The matching floating export's value. |

An absent/dead target, missing source program or missing export returns false
for dispatch success, or the typed call's neutral value. A mismatched declared
return type also yields the typed neutral value. A matching export with incorrect
argument count or incompatible argument types faults before invocation.

If a delivered helper retires its recipient, `call` still reports true, while a
typed call returns its neutral value because that recipient VM did not publish
a result. Caller retirement stops the caller instead. A recipient VM fault,
changed program/storage identity or exhausted execution pool is an error, not a
missing export. See [typed recipient results](targets.md#typed-recipient-results).

Handles must be integral expressions. Integer state, locals, helper arguments
and integer queries retain all 32 identity bits. Converting an already rounded
float cannot recover lost bits. Export names are quoted preparation-time
identifiers of 1–127 characters. Each program admits up to 128 exports and 128
distinct recipient-call names. Lookup uses the prepared hash plus exact pinned
name bytes, so a hash collision cannot select a different helper.

Dispatch works in ordinary fighter/article callbacks and helpers, including
assignments, conditions, return expressions and nested helper arguments.
Arguments run once in written order. Boolean `and`/`or` preserves short
circuiting: an unvisited branch invokes no call. Immutable metadata/defaults and
read-only callback contexts do not gain mutating dispatch.

## Signals, notifications and ordering

Article signals and same-program creator/owner helper calls follow the same
source-lifetime checks. Their receiver policy does not grant foreign access to
private helpers or article state. Use
`creator.helpers.Name(arguments...)` or `owner.helpers.Name(arguments...)`
for a package's own fighter helper, and `targets.call_*` for an exported
recipient operation.

The preparation pass checks the static helper graph, including local signal
and same-program receiver edges, and rejects recursion. Cross-program exports
and engine reentry are checked at runtime using bounded shared scratch.

Article `created`, `spawned`, `picked_up` and `destroyed` callbacks may
remove their Item. The engine's continuation must also validate the retained
source before accessing it. Repeated removal while the same Item is already
being destroyed is ignored. A constructor's returned handle is usable only if
the calling source survives the synchronous construction path.

## Execution bounds and diagnostics

Preparation lowers checked source to immutable typed bytecode. Ordinary
packages do not generate C, invoke Clang or load a behavior DLL. Helpers use
prepared program-local function IDs; authored state belongs to the executing
actor.

Synchronous invocations share fixed pools of 64 call frames and 65,536 typed
registers, including engine reentry and recipient calls. Pool exhaustion reports
an error. Exit, retirement and failure restore the invocation's original scratch
depth; dispatch does not allocate runtime storage. Preparation also checks its
bounded syntax, expression, local, function and instruction arenas.

Diagnostics retain source locations and executing function, actor and frame.
The [frontend checkpoint](../native_script_frontend.md) records typed execution,
retirement, reentry and pool-error fixtures. The
[recipient checkpoint](../native_source_recipient_export_plan.md) records direct
export dispatch. Historical generated-C lifetime fixtures remain comparison
evidence for that earlier backend; they do not define the interpreter's current
return rules or authoring limits.
