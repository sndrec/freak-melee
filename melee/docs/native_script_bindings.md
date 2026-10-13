# Direct interpreter binding inventory

This document covers the semantic boundary between authored FMS and native
engine operations. `tools/audit_native_script_bindings.py` inventories all ten
selected `examples/*_full.fms` templates and the six Rook Workshop sources.
It consumes the shared production expression lexer's tokens, preserving
balanced call/index/member chains and their argument text. Its executable is
`tests/native/script_binding_inventory/lexer_census.c`. No generated C, DLL,
Clang AST or native callback address forms part of this inventory input.

The inventory is **not production type acceptance**. It reports every executable
postfix access it sees, explicit lexer/statement framing failures, and unresolved
signatures/context/invalidation. Literal references in the old C emitters are
source candidates only. They cannot prove argument types, output behavior,
ownership or runtime support. `contracts.json` records the smaller source-checked
family. The production loader must reject operations outside its actual typed
catalog even when this audit finds their spelling in a legacy emitter.

## Semantic names and selectors

Use a load-resolved immutable binding catalog with semantic names, allowed
execution contexts, receiver/selector types, argument modes, result type,
read/write permissions, publication policy and invalidation effects. The IR
already represents `Name`, `Member`, `Call` and `Index` separately. Keep these
nodes and evaluate selector arguments in authored order.

`item.resource(node).joint("0/1").rotation.z` contains an article-owned
resource-index selector followed by a root-relative joint selector. The audit
key `item.resource().joint().rotation.z` groups that family; each occurrence
retains both argument lists. It is not a flat token named `item.resource` or
a callable raw member address. `articles.ref(handle, Kind).exists` similarly
retains an identity and declared article kind. Existence checks resolve the
live article against the caller's immutable author/Definition; current damage
ownership must not substitute for authorship. These references must never
expose a raw item pointer to scripts.

Distinguish authored helpers (including article-local helpers), recipient
exports, local arguments, immutable tables/parameters, mutable script state,
engine constants and native operations. Helpers resolve to program code
references, not per-fighter native callbacks. `owner.call(Helper)` and
`creator.call(Helper)` contain an authored helper identity; their dispatch
operation remains generic. Table lookup and script-state indexing lower using
their owned declarations; names and array indices remain in the occurrence
records.

## First binding family

Implement the following actual generic family first. It requires no fighter
name, motion-specific operation or authored-function C wrapper:

| Semantic property | Storage/result | Access |
| --- | --- | --- |
| `position.x/y/z` | `Fighter.cur_pos`, Float | read/write |
| `position.delta.x/y/z` | `Fighter.pos_delta`, Float | read/write |
| `velocity.x/y/z` | `Fighter.self_vel`, Float | read/write |
| `acceleration.x/y/z` | `Fighter.x74_self_accel`, Float | read/write |
| `input.x/y` | current left-stick Float | read |
| `input.special_held/pressed` | button-mask truth, Bool | read |
| `frame`, `frame.integer` | animation frame Float / truncating Int | read |
| `facing.direction` | `Fighter.facing_dir`, Float | read/write |
| `grounded` | ground/air enum comparison, Bool | read |
| `airborne` | ground/air classification, Bool | read/write |
| `effects.active` | effect activity bit, Bool | read/write |
| `movement.used_tether/fastfall` | fighter flags, Bool | read/write |

In article contexts the same `position.x/y/z` and `velocity.x/y/z` semantic
properties refer to `Item.pos` and `Item.x40_vel`, respectively; article
`facing` is the Float `Item.facing_dir`. The catalog must discriminate source
context instead of applying fighter field storage to an item. The reference
read/write implementation is `tools/native_fighter_item_script.c`.

`airborne = Bool` uses the substantive direct classification operation in
`tools/native_fighter_script_relations.c`: set `ground_or_air` to GA_Air or
GA_Ground. It deliberately introduces no transition-helper side effects;
paired fighter entry logic already performs the rest of that transition.

Read source: `tools/native_fighter_script_code.c` and
`tools/native_fighter_script_primitives.c`; write source:
`tools/native_fighter_script_code.c` and `tools/native_fighter_script_kinetics.c`.
`src/melee/ft/types.h` owns storage widths. These property operations can use
native field access internally. The immutable script artifact must contain
semantic IDs/types, never `offsetof(Fighter, ...)` or public C layout.

Use Float storage for these properties; the numeric IR determines whether
authored intermediates are F32/F64. Boolean properties must return canonical
Boolean values rather than leaking the original bitmask integer. Float-to-Int
storage conversion is explicit (`int` truncation); creation identity bits must
not pass through Float. Reading input and ordinary native field writes have no
synchronous recipient dispatch; reentry-capable value/selector expressions
must finish and pass generation checks before writing their captured target.

Declared `state` fields belong to each fighter/article instance's explicit
rollback state. Boolean fields use their declared storage width, Int fields
carry all 32 bits, and Float fields use native Float width. Validate field and
array bounds while loading; retain runtime bounds checks for dynamic indices.
Immutable `attrs`/`item.params` resolve to program/content-owned parameters.
These are declaration-backed accessors, not an open-ended wildcard namespace
that accepts an unknown field by guessing Float.

## Query outputs and source lifetime

| Script operation | Input and result | Native implementation |
| --- | --- | --- |
| `targets.read_position(id, x, y, z)` | Int identity, distinct Float output lvalues; Bool | `nativeFighterTarget_Position` |
| `targets.read_position_delta(id, x, y, z)` | same | `nativeFighterTarget_PositionDelta` |
| `articles.read_primary_translation(id, kind, x, y, z)` | Int identity, declared kind, distinct Float outputs; Bool | `nativeArticleRef_PrimaryModelTranslation` |

`tools/native_fighter_script_queries.c` checks input types and distinct output
destinations. Fighter callbacks/helpers/recipients accept Float scalar state or
typed Float locals. Article callbacks accept article scalar state or Float
locals. Arrays, computed expressions, duplicate destinations and undeclared
article kinds are rejected. Effect callbacks cannot use these query forms.

Evaluate inputs once in source order. Compute the native result into transient
fixed storage, check the original source generation, then publish outputs only
on success. Failure leaves destinations unchanged. A reentrant query must not
publish its Boolean result or coordinates after source retirement/recycling.
The old lifetime emitter lists these calls conservatively as invalidating
(`tools/native_fighter_script_lifetime.c`), even when a particular backend
currently only performs a registry lookup. Carry this effect metadata into the
native catalog and the VM continuation guard.

`nativeFighterTarget_Position` reads current fighter position; PositionDelta
reads movement delta. PrimaryModelTranslation reads the primary model root's
local translation, not world position. The actual native sources are
`src/melee/ft/native_fighter_target.c` and `src/melee/it/native_article_refs.c`.

Resource queries have a different existing contract. For
`item.resource(node).read_attachment_point(localX, localY, localZ, outX, outY,
outZ)` and `.read_line_speed(line, outX, outY, outZ)`, the reference emitter
initializes a zero Vec3 and publishes it even if the native Bool query fails.
Outputs must be three distinct article Float scalar state fields. Do not
accidentally give these operations the targets query's success-only policy;
either preserve their deliberate semantics or document and validate a language
change. The inventory records this discrepancy explicitly.

## Resource and joint properties

`tools/native_fighter_article_resources.c` contains actual typed geometry and
joint-property tables; `src/melee/it/native_article_resource.h` declares the
substantive backend. Position, velocity and collision current/previous XYZ
channels are Float and writable. Collision previous refers to
`CollData.last_pos`; moving a node does not reinitialize its collision history.
The selected resource index is Int and is evaluated once. Resources retain
article ownership, generation, capacity/reservation and teardown bookkeeping.

Joints support translation, Euler rotation, scale, quaternion, existence,
subtree visibility and self visibility. Existence is read-only. Joint paths
are load-validated literal child ordinals (0..255, at most 12 levels); the empty
path selects the root. `item.joint` accepts an optional literal model index
**before** the path; `effect.joint` accepts an optional literal member index
**after** it. The native setters retain HSD matrix invalidation and instance
boundaries. Missing-joint values, quaternion restrictions and visibility rules
are specified by `src/melee/lb/native_model_joint.h`; do not replace those
operations with blind field writes.

## Implementation and acceptance boundary

Bind these semantic entries directly in the engine's generic native catalog.
Use distinct read/write/call functions with typed VM values and current source
context. Resolve catalog indices at load; dispatch through immutable entries
and use bounded VM argument/output scratch. Allocate only while loading/applying
content. Each reentry-capable operation has a native continuation barrier; VM
helper calls propagate retirement through their synchronous frame chain.

The census is a migration checklist. A source-checked property is not yet an
installed production binding. Remaining operation signatures require actual
source audit or typed catalog implementation; spelling candidates alone must
never satisfy that gate. Acceptance needs direct frontend resolution, native
property/query tests, high-bit identities, failed-query output preservation,
source retirement/reuse, ordered helper arguments, chained selectors and real
fighter/article callback comparisons. No game acceptance, runtime speedup or
complete full-template typed coverage is claimed by this inventory.
