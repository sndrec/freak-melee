# Article helpers

Declare reusable functions inside an `article` block. Each article owns its
helper names; two articles may use the same name with different typed arguments.
The interpreter runs them with the calling article's Item context and owned state.
Helpers do not replace engine callbacks or export recipient callbacks.

```text
article It_Kind_Ness_PKFire states 1:
    state int last_id
    state float amount

    func Record(int id, float amount):
        item.state.last_id = id
        item.state.amount = amount

    func Update(float amount):
        let int id = item.creation_id
        Record(id, amount)

    func Scaled(float amount) -> float:
        return amount * 1.25

    func Positive(int amount) -> bool:
        return amount > 0

    on signal update:
            Update(signal.value)

    state 0 anim -1:
        on physics:
            let float amount = Scaled(1.5)
            Update(amount)
```

Helper declarations use four spaces; their bodies use eight, with four additional
spaces per nested block. Article callback bodies retain their existing twelve
spaces. A call may precede its helper declaration. Helpers may call other helpers
in the same article, and may use `owner.helpers.Name(...)` and `creator.helpers.Name(...)`
operations to call a package fighter function. Those operations retain their
ordinary live creator/package and current-owner checks.

Arguments must declare a supported scalar type, such as `float`, `int`, `uint`, or `bool`. Typed-expression rules
apply to calls and local assignments: use an explicit `int(...)` when converting
a float into an integer. `let int`, `let float`, `let bool`, and legacy float
`let` locals are available. A local may be assigned again with a compatible
value. Block locals leave scope when the block ends; a later sibling block may
reuse their names. Parameters remain in scope throughout the helper and count
toward its local limit. Omitting a result declaration defines a Void helper;
`return` exits it without a value. A typed helper declares `-> bool`, `-> int`,
or `-> float` and ends with an unconditional `return expression` at its body
indentation. Earlier returns are allowed. `pass` is an empty statement.
Typed calls can supply locals, state assignments, conditions, return expressions
and arguments to other helpers. Their result may also be discarded by a call
statement. Void helpers remain statement-only.

The same assignment rules as fighter helpers apply to results: Float accepts
Int widening, Bool converts a numeric value to its nonzero truth value, and Int
rejects Float without an explicit conversion. Integer results and arguments
retain full 32-bit values; neither handles nor ordinary Int results pass through
Float storage. An explicit `float(...)` conversion may lose integer precision,
as it does elsewhere in the language.

Helpers receive no implicit callback payload. They cannot read `signal.value`,
an interaction callback's `other`, or a callback's local variables. Pass the
needed scalar values as typed arguments at the callback call site. A helper
cannot be called by a fighter callback or another article merely because its
name matches.

Article helpers share the package's capacity of 512 functions/callbacks. Each
body has space for 512 local declarations, including up to 32 arguments. The
source, expression and bytecode arenas also have fixed capacities; preparation
rejects overflow rather than truncating a body. There is no emitted-C line limit.
See [function boundaries](functions.md#current-boundaries).

The helper graph must be acyclic, including edges through local article signals.
Direct recursion, mutual recursion, and helper-to-signal-to-helper cycles are
rejected. Helpers may signal declared local callbacks, including forward
declarations, subject to the same cycle checks.

Synchronous operations can invalidate the calling Item. After an operation
marked as potentially invalidating its source, the interpreter checks the
retained identity before publishing a result or executing a continuation.
Arguments evaluate once in written order, and Boolean expressions preserve
short circuiting. On retirement, the invocation returns `SourceRetired` with
output untouched: `item.remove()` stops the current callback and its returning
helper chain. See [synchronous callbacks](synchronous_callbacks.md). Helpers
cannot retain native pointers or make a removed entity accessible.

`creator.helpers.Name(arguments...)` and `owner.helpers.Name(arguments...)` may also
return a package fighter helper's declared Bool/Int/Float value. These remain
same-program private calls, not recipient exports. The creator is the
validated retained original creator, independent of reflection/current owner;
`owner.call` additionally requires that creator to remain the current owner.
Guard optional recipients with `creator.exists` or `owner.exists` before
calling. Arguments evaluate once in written order. The retained receiver's
generation, class and owned program/storage are validated before invocation and
after return. A retired, reused or ineligible captured receiver faults the call
without publishing a result. If the helper removes the calling Item, its source
lifetime check stops the Item continuation. Void helpers remain statement-only;
typed helper calls may be used in expressions.

Helpers receive no native pointers. Existing restrictions still apply to held
status queries: a read-only held-status query cannot invoke mutating helpers.
`targets.call*` can be used in ordinary callback/helper expressions with the
[typed recipient](targets.md#typed-recipient-results) contract. Numeric expressions
use the interpreter's [checked arithmetic](language.md#callback-locals).

## Historical generated-C evidence

These fixtures describe the earlier reference backend, not the ordinary package
runtime or its current capacity and retirement rules. The current interpreter
contracts are documented in the [frontend](../native_script_frontend.md) and
[recipient dispatch](../native_source_recipient_export_plan.md) checkpoints.

`tests/native/article_helpers_compiler_test.c` checks both generation routes,
typed forward calls, article namespaces, local scopes, required lifetime Host
entries, limits, and original source-line diagnostics. The generated-module
lifetime fixture separately exercises nested helper removal and local signals.
These are compiler and callback tests; they do not establish tether gameplay
acceptance.

`tests/native/article_helper_returns` separately compiles both generated forms
and executes actual generated callbacks with full-width integer results, numeric
truth/widening, ordered nested mutation, source removal/recycling (including
PAGE_NOACCESS Item storage), and creator/owner argument/result revalidation.
The registry/recipient Host boundary is controlled in this console fixture;
it does not claim full engine dispatch or gameplay acceptance.
