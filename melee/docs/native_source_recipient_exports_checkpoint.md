# Direct interpreter: recipient-owned exports

`targets.call/call_bool/call_int/call_float` now execute the captured recipient
Fighter's own exported helper through its actual script actor and VM. Fighter
and Article callers may belong to a different package. The recipient supplies
its local function ID, mutable state layout and immutable tuning; no stock
handler or caller-program substitute is used.

Program indexing accepts root, zero-argument exports returning Void/Bool/I32/F32.
It prepares up to 128 exports and the compiler prepares up to 128 distinct literal
call names. Names are bounded to 127 identifier characters. Sorted hash buckets
use exact pinned name bytes to resolve collisions; there is no runtime hashing,
parsing or heap allocation. Repeated literals at different offsets retain the
same prepared name token without changing their original source spans.

`nativeScriptFighter_Helper` now returns the actual VM status instead of
collapsing Error and SourceRetired into false. Existing owned-helper consumers
explicitly require Ok. Their stricter same-program/post-live contracts remain
intact; dynamic recipient lookup uses the captured destination's actual bank.

## Results and lifetime

| Situation | Result |
| --- | --- |
| Live recipient and matching export | Execute the destination-local helper; publish its exact typed result, or true for untyped delivery. |
| Missing target, unattached stock target, missing export or wrong typed ABI | Return false/0/0.0 without executing a helper. |
| Delivered helper retires its recipient while the caller survives | Untyped delivery returns true; typed calls return neutral because the VM withheld its return. |
| Caller retirement/fault, recipient fault, malformed matched table or substituted live identity/schema | Publish no result or subsequent caller writes. |

Physical pool removal/reuse is distinguished from a surviving object's changed
userdata/class/program/schema. A replacement is never reacquired after a
callback. Ordinary field changes on a surviving recipient remain valid.
Self-calls and cross-package nesting use the shared fixed 64-frame/65,536-register
scratch. Exhaustion is terminal, with balanced frames/registers/payloads/scopes.

See [recipient calls](scripting/targets.md#calling-a-recipients-named-event),
[function declarations](scripting/functions.md) and the
[source-backed contract](native_source_recipient_export_plan.md).

## Grouped evidence

The native client, content tool and reference targets build. All 40 selected
integration CTests and three reference CTests pass after final source changes.

- `A:/FreakMelee-test-archives/script-target-exports-final2/receipt.json`:
  757 checks, 86 cases, 47 top-level VM calls, three independently prepared and
  published banks, and 18 injected callback mutations. Complete compiler,
  binding catalog, Fighter/Item actors, Execution, VM, storage and GObj pools
  run together. Matching names with different local IDs return different bank
  values. `costarring` and `liquid` share FNV key `0x5e4daa9d` yet dispatch to
  distinct exports. Repeated literal spans, all typed results, neutral absence,
  root export validation, full-width handles, malformed tables, nested cycles,
  recipient/caller removal, reuse, faults and withheld tail writes are covered.
- `A:/FreakMelee-test-archives/script-helper-status-oct09-integrated/receipt.json`:
  2,537 checks in the expanded Fighter scheduler/Helper status cohort. Real actor
  execution covers healthy returns, preflight rejection, VM faults and retirement
  with withheld output; existing owned-helper status requirements remain strict.
- Existing source-index/compiler tests now cover export root/type/argument
  rejection, collision-safe name preparation, literal deduplication and preserved
  accepted banks after rejected source. Overlong export declarations are rejected
  during indexing rather than becoming uncallable at runtime.

The connected fixture controls asset graphs, the selected runtime package getter
and physical Fighter/Item userdata. Its position-write leaf updates the actual
native field, then injects explicit retirement/reuse/fault cases. It does not
run DAT import, production Fighter allocation, native callback scheduling,
rendering or whole-match rollback. No complete gameplay acceptance is claimed.

## Source preparation and next batches

`A:/FreakMelee-test-archives/source-recipient-exports-census-oct09-integrated/receipt.json`
pins the rebuilt tool and ten full templates. Peach passes its exported calls
and now reaches `item.bounce()` at line651. Mario, Dr. Mario, Game & Watch and
Zelda continue to prepare; Link/Young Link, Sheik and Popo/Nana retain their
secondary-resource metadata boundary.

The [remaining Peach audit](native_source_peach_remaining_api_audit.md) groups
the actual next contracts: Item victim/masked-stage responses and attachment
identity; guard callbacks plus dynamics/contact dispatch; float controls,
owned landing callbacks and animation-driven jump; held transitions and pure
observations; asynchronous effects. Typed source acceptance alone does not
prove those native hooks or resource admissions. Implement shared primitives
and let scripts own fighter-specific decisions.

The full project goal remains active. This checkpoint closes recipient dispatch,
not the remaining roster, resource lifecycle or gameplay acceptance.
