# Article state transitions

`item.change_state(state)` uses the native animation-update flag.
`item.change_state_flags(state, flags)` supplies the native unsigned flag word.
Both require a state in the executing Article's immutable state table and retain
its actor, program, storage, attributes and model across the native operation.

The native transition selects the authored state and animation, performs common
effect/model/animation/command work, then clears the native transient callback
slots and their saved interpreted callback IDs. A nested transition increments
the state revision; the interrupted outer setup stops before clearing callbacks
installed after the inner transition. Normal state and owner changes do not
retire the actor. Fault, removal or identity substitution stops VM continuation
and leaves the operation output unpublished; completed native effects remain.

State setup requires the native Item's JObj/anchor: its first translation write
asserts that pointer exists. This permits a model-less imported descriptor,
because the native constructor allocates an anchor when the descriptor root is
NULL. Revision exhaustion rejects before mutation, matching the actor's native
state-selection contract. Flag bits are passed through without normalization.

The Sheik source failure at `attrs.needle_held_part` is a separate parameter-owner
issue. Article-local `attrs` cannot read a root parameter; use
`program.attrs.needle_held_part`. The state-flags operation already exists.
