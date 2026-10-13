# Alternate forms

An alternate-form package owns two independent members: their rigs, assets,
source banks, parameters and persistent fighter variables. One member is active and
the other is dormant. A simultaneous companion package uses a different
lifecycle and cannot use this transfer operation.

## Methods

| Method | Result | Description |
| --- | --- | --- |
| `forms.activate_partner()` | `Void` | Activate the dormant member of the current player's alternate-form family. Rejected transfers fault the callback. |

This fighter-only method takes no arguments. It rejects a missing partner,
invalid relationship, recursive transfer or exhausted callback budget. Both
authored members must own their behavior; there is no stock special-handler
fallback. A package must define `on form_activated` for each member before the
loader accepts an alternate-form family.

The shared engine operation transfers position, facing, damage, status effects,
held/worn items and the original common reset/bookkeeping. It then invokes the
destination's own `on form_activated` callback. The script decides the destination
motion, grounded/airborne arrival and callback bindings. The transfer does not
re-run `on load` or erase either member's persistent fighter variables.

On success the call completes without a value. The source normally remains alive but dormant; scripts
should finish their switch callback rather than continuing to modify that source.
Interpreter lifetime checks stop continuation if a callback retires or replaces
the source. The transfer also validates both fighter identities and retained
items after returning callbacks. Failure after a callback can follow mutations
already made; a runtime error does not roll back arbitrary authored side effects.
The destination must be live and outside any running script invocation before
its generation is reset. The source callback is expected to be running.

## Destination callback

`on form_activated` is a fighter-wide callback, like `on load`. Each member's
source bank supplies its own zero-argument `Void` implementation. Use ordinary fighter variables for
member-specific state and [movement operations](movement.md) for its arrival.
Member identity and the player's mutable active/dormant roles are different:
switching roles does not change which member owns a source bank, rig or parameters.

See [paired fighters](paired_fighters.md#member-identity) for immutable member
lookup, independent of the active/dormant roles.

## Current scope

The direct source package path prepares both alternate members before publishing
either bank or the player mapping. The generic interpreter binding and actual
shared transfer now dispatch the destination's source event without a stock
special handler. See the [form contract audit](../native_source_form_contract_audit.md)
and the [runtime checkpoint](../native_source_form_runtime_checkpoint.md) for
evidence and controlled boundaries.
Complete Zelda/Sheik source templates still need mutable accessory callbacks and
other move/resource closures before gameplay acceptance. Simultaneous companions
remain disabled while their CPU/history and shared-behavior contracts are pending.
