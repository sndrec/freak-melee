# Scoped event payload values

```powershell
python tests/native/script_event_payload/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-event-payload-oct07
```

This unit uses the actual Source/ProgramIndex/EventDescriptor/Statement/Bytecode/
VM pipeline. The payload resolver accepts exact descriptor properties only
inside that callback's source range. Anonymous incoming slots remain anonymous;
helpers receive event values through explicit typed arguments. Event semantics
encode both descriptor ID and argument index in the reserved 0x30000000 domain.
Program 0x3f namespaces and unrelated native paths remain available to their
own substantive resolvers.

Payloads are read-only fixed copies. The one native UInt32-to-authored-Int history
identity conversion preserves all bits without Float. Actor inputs carry only
retained identity/generation/kind; unsupported mutable trail pointers stay
rejected. No raw native pointer becomes a script value. Derived object member
families still require their own accepted native selector/binding contract.

Caller-owned LIFO frames use unique serial handles. Reads require the active top
frame, correct event identity, matching actor/source class/generation/frame and
the caller's live-source predicate. Nested callbacks cannot read suspended outer
frames or confuse same-typed properties from different descriptors. Balanced
pop restores the outer payload and invalidates old handles, including reused
slots. Successful zero-argument frames return a non-null native argument-array
pointer with count zero. Frame pools are transient, not persistent script state.

Tests execute numeric and Boolean inputs, high-bit identity, actor references,
explicit helper argument passing, nested fighter callbacks and nested article
signals with distinct payloads. They check actor retirement/reuse, stale handles,
out-of-order pop, capacity/serial exhaustion, type/width failures, and unchanged
outputs after errors. On Windows retirement protects the actor page with
PAGE_NOACCESS; subsequent payload reads never touch the retired actor.

Compile failures cover payload writes, unavailable namespaces, unknown fields,
property call/index forms, helper implicit payload access and unimplemented
object members. Actual event property evidence is in
`tools/native_fighter_script_code.c` (event modes/motion/history/geometry) and
`tools/native_fighter_item_script.c` (held action/frame/duration and signal value),
with descriptor native signatures from the event catalogue. The lifetime provider
is controlled here; engine registry/execution/scheduler acceptance is separate.

The production controller should derive the descriptor from the actual source
callback, push its typed arguments, seed VM parameters through Arguments, route
the event semantic domain through Read, reject event writes and always pop after
execution. It should additionally verify that current interpreted event reads
belong to the entry callback rather than a helper, when defending against
malformed immutable code. Root owns that integration; this unit adds no adapter
around legacy callbacks and launches no game.
