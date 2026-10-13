# Direct script frontend: implementation checkpoint

The approved production interpreter transition is in
`native_interpreter_transition.md`. These new native frontend units consume
`.fms` source directly; they do not invoke a C emitter, Clang or a behavior DLL.
This checkpoint is not a complete executable program loader.

## Source and expressions

`native_script_source.c` builds a caller-owned source hierarchy of declarations,
functions, callbacks and statements. It preserves source spans, grouping,
comments and quoted contents, exported helper signatures, inline conditions,
article state aliases and intentionally empty state rows. Existing article
callback indentation is accepted without importing the old generator's fixed
callback/body limits. Executable blocks enforce sibling indentation and explicit
structure; unknown metadata is retained for subsequent declaration validation,
not silently accepted as runtime behavior.

`native_script_expression.c` builds typed semantic IR in caller-owned node and
argument arenas. Generic nodes preserve Name/Member/Index/Call structure,
selectors, output destinations, numeric conversions, short circuiting and source
locations. The semantic resolver must supply actual binding/context contracts;
unknown names fail instead of acquiring guessed types. Parsing rolls arena counts
back on failure. No native address or C field offset is part of this IR.

Public Float storage is F32; decimal floating literals retain F64 intermediates,
and integer masks/identities preserve their typed widths. Assignment and explicit
conversion are separate operations. Source-invalidation effects propagate through
expressions, while execution guards must remain at reached calls rather than
executing an unchosen short-circuit branch.

## Typed statements

`native_script_statement.c` resolves locals with lexical visibility and nested
shadowing, checks writable assignment destinations, scalar conversions, return
types, conditions and loop-control scope. Its semantic binding resolver is
supplied by the declaration/program compiler. Caller-owned local tables and
expression arenas have fixed bounds. Failed bodies clear executable annotations
and restore arena/local counts so they cannot become partially loaded programs.

Syntax parsing alone does not validate function signatures, all-path returns,
event contracts, metadata or engine operations. The program/declaration and
bytecode passes below cover signatures and executable control flow. Complete
event/metadata/binding validation remains required before a package can load.
No Workshop or package path currently executes this IR.

## Checked scope, 2026-10-07

- The syntax/hierarchy corpus parses all 27 checked-in example files, with
  35,471 fixture/corpus checks. It includes untyped locals inferred later, inline
  conditions, exported helpers, actual article callbacks and state aliases.
- Direct expression fixtures pass 195 checks across 61 expressions, including
  Nana's mixed precision, unsigned identity masks, nested resource/joint selectors,
  article array outputs, recipient results and invalidation propagation.
- Typed statement fixtures pass 57 checks for scopes, shadowing, conversions,
  control flow, output permissions and atomic failure. They use controlled
  semantic binding contracts; this is not full fighter or native-binding coverage.
- The native client/tool build passes, as do seven focused frontend/evaluator,
  replay and rollback-session/page CTests. No game or visible window was launched.

Evidence: `A:/FreakMelee-test-archives/script-source-ir-final/receipt.json`,
`script-expression-ir-effects/receipt.json`, and the matching CTest build.
`native_script_bindings.md` records 63 used source-checked contracts and 660
explicitly unresolved signatures in its selected 16-source corpus. That lexer
inventory is not production type acceptance or all-template coverage.

## Direct bytecode checkpoint, 2026-10-07

`native_script_program.c` indexes source-owned helper signatures and declarations
before compiling bodies. Forward helper calls retain typed parameter/return
contracts. Fighter and article declaration scopes remain distinct; owned arrays
have explicit counts and aligned internal layouts. Parameter defaults use the
same typed scalar operations as execution. Imported attribute offsets are
recorded for the asset import transition, not native VM field addresses.
Callbacks require an actual event-signature contract; missing contracts remain
unresolved and cannot compile. Index acceptance alone is not executable package
acceptance or metadata validation.

`native_script_bytecode.c` lowers typed statements and expressions directly into
immutable instructions/function records. Helpers use program code IDs. Selector
arguments and output destinations are captured before invocation/publication;
short-circuit branches remain lazy. Complete returning branch chains are
recognized, while partial non-void returns fail. Failed lowering restores the
instruction count.

`native_script_vm.c` executes these instructions using caller-owned registers and
frame pools. Synchronous reentry preserves suspended frames. Native continuation
guards stop a helper chain if its actor retires or changes generation. Output
types are checked before publication; failed queries retain old outputs. Errors
include source location, function, actor and frame. There is no runtime allocation,
generated authored C, AST input, machine-code value or authored native fallback.
See [the runtime fixture](../tests/native/script_bytecode/README.md).

The first actual native catalog family implements source-checked Fighter/Item
properties and typed math. Unsupported engine families still fail resolution.
This does not yet implement changing receivers such as `owner.state`: those need
an identity/generation captured before the right-hand side, rather than resolving
an owner again when publishing. See
[the native binding fixture](../tests/native/script_native_bindings/README.md).

The declaration fixture indexes 8,783 declarations across all 33 current example
files with 4,762 checks. This covers signatures/defaults/array layout, not all
body bindings. The runtime fixture passes 8,322 checks, including 4,096 numeric
inputs compared with C references, typed helpers, loops, native output ordering,
synchronous reentry, protected retired actor storage and explicit pool errors.
The actual native-property fixture passes 27,225 checks. The native client/tool
build and 11 focused frontend/runtime/replay/rollback/pool CTests pass. No game,
visible window or audio was launched. Whole-game migration and new performance
acceptance remain pending.

Receipts: `A:/FreakMelee-test-archives/script-program-index-recursive/receipt.json`,
`script-bytecode-checkpoint/receipt.json` and
`script-native-bindings-bytecode/receipt.json`.

## Owned storage and event contracts

`native_script_storage.c` derives active owner footprints from actual indexed
declarations. Instances use caller-owned Bool/Int32/Float32 state buffers and
shared immutable parameter tables, including UInt32 import values. Imported
attributes remain unresolved until supplied. Reads/writes check declaration
identity, exact owner, scalar/array shape and bounds; parameters cannot be written
during execution. Snapshots contain active state bytes and bind the live instance,
generation, layout and parameter configuration. A 713-check direct VM fixture
restores a snapshot and reproduces the exact endpoint after 64 ticks. Native
object resurrection and engine integration remain separate. See
[storage API and evidence](../tests/native/script_storage/README.md).

`native_script_events.c` records native slot/signature provenance and uses actual
source ancestry to distinguish fighter, article, Workshop item and effect
callbacks. Event inputs occupy typed anonymous slots; authored property names
come from the event catalog. Audited implicit returns are emitted as constant or
incoming-parameter return instructions. Typed helpers have no default return.
Read-only event policy blocks engine/owned-state mutation, loops and unproven
calls. Pure native calls carry an explicit semantic effect flag. Audited exact
event return types are enforced separately from ordinary nonzero Bool return
conversion. Event resolution does not install any engine callback or complete
the remaining payload binding families.

The event fixture passes 301 checks through 28 actual callbacks and three Bool
helpers. Its source census resolves 1,861 callback signatures across the selected
16-file corpus. All 181 catalog slots/function-argument hooks have checked native
declaration provenance. The client/tool build and 13 focused CTests pass with
storage and event units included. This is signature and focused execution
evidence, not whole-template bodies or game acceptance.

The mutable `NativeFighterTrail*` output event remains explicitly unsupported
until it has a proper typed output contract. A native pointer is not substituted
for a script value. Broader metadata and full template bodies remain acceptance
gates. See [event source audit and fixture](../tests/native/script_events/README.md).

## Native execution entry and kinetic family, 2026-10-07

Self article paths `item.state`, `item.params` and `item.attr` now resolve their
actual owned declarations. An article cannot inherit a same-named fighter value
implicitly. Changing owner/creator receivers still require explicit identity
capture and are not supplied by these self paths.

`native_script_event_payload.c` validates exact authored payload properties
against their callback signature, encodes the event identity with its argument
slot, and retains values in a fixed synchronous frame pool. Nested callback and
signal frames isolate their values; stale/suspended handles and different
source/frame/generation are rejected. Typed Object values contain retained
identity/generation/kind, never native addresses. The fixture passes 156 checks,
including protected-source retirement and no publication on failure.

`native_script_execution.c` combines actual actor registry liveness, owned
storage, native primitives and scoped payloads. It resolves function ownership,
callback identities and ABI at load, then binds a live storage/configuration
view. Entry does no source-name parsing, parameter-table scan or allocation.
The 418-check fixture executes 128 fighter ticks against actual actor structs,
writes game position and script-owned data, receives history/signal values and
rejects changed class/ownership/generation. Helpers do not acquire an implicit
event payload. See [native entry evidence](../tests/native/script_execution/README.md).

The kinetic family adds source-checked animation/frame/rate, motion and physics
operations. Compound scalar/vector expressions retain required Double
intermediates, while explicit native Float primitives marshal Float inputs.
Animation start preserves native stage order and checks generation between
reentrant stages. Its fixture passes 25,582 checks / 6,976 native-body comparisons,
including 91 Double-sensitive cases and subsequent-write suppression on
retirement. Engine/HSD observation boundaries are disclosed in
[the kinetic fixture](../tests/native/script_kinetics/README.md).

The client/tool build and 17 focused checks pass. The untimed live object census
completed 2,000 eight-fighter gameplay ticks without topology errors; its partial
payload estimate and limits are in [object census](native_match_object_census.md).
This does not select interpreted code in the live game scheduler or Workshop
loader, replace whole-game snapshots or establish interpreter timing acceptance.

## Next required work

The next checkpoint adds complete transactional compiler banks and actual live
load/tick/removal scheduler sites, with creation-identity rebinding at respawn.
The source-only diagnostic completed 2,000 actual VM ticks per four Popo actors
and reproduced all 64 real rollback windows with their variables included.
Twenty focused checks pass. Supported hooks, excluded first-run evidence and
remaining scope are in [live loading](native_script_live_loading.md).

Native GObj/proc allocation now uses fixed process-reserved pool backing and
logical scene epochs. Backing and bookkeeping remain in the current `.mstate`
oracle while other owners migrate. The actual-source fixture exercises deferred
deletion, reorder, reuse/restoration and exhaustion; a live eight-actor census
matches prior object/proc populations with no topology errors. This still is a
partial explicit-state owner, not replacement of the entire page mechanism.

Complete event/metadata contracts and the remaining generic native binding
catalog; integrate interpreted code references and actual per-instance owned
state into the existing native scheduler/registries;
then migrate Workshop apply and content loading and remove the superseded ordinary
C/DLL path. Keep the evaluation and accepted fighter behavior comparisons as
reference evidence. Complete mutable-state snapshots using the source inventory
in `native_match_snapshot_inventory.md`; live-object copies must also restore
ordered allocator/free-list metadata and linked object lifetimes.

## Typed imports and callback registration, 2026-10-07

`NativeScriptCompileRequest.parameter_provider` receives the validated indexed
candidate and supplies typed parameter values at its loading transition. It is
mutually exclusive with explicit supplies. Preparation copies values into owned
immutable slots before publication; rejected imports preserve the accepted bank.
The compiler fixture now has 111 checks, including provider failure, copied-value
ownership and conflicting-supply rejection.

`native_script_metadata.c` reads root/fighter Attribute starting values from the
actual DAT's owned special-attribute block using validated DAT references and
bounds. It returns typed supplies keyed by candidate IDs. That is a one-time
recipe import, not a runtime DAT-offset API; authored scripts use named owned
parameters. Entity-owned imports are counted as unresolved, never guessed from
the fighter block. The module and compiler-provider boundary are implemented;
Workshop/package metadata and full source-only loading are still separate work.

The setup reference is [direct bindings](native_script_setup_bindings.md).
Damage/death/hitlag/dealt-damage/state-change instance registrations store fixed
owned function IDs, with source-cached event lookup and ABI/owner validation.
Live source-only callback state now participates in64 verified broad-oracle
rollback windows. Actual event sites, limitations and timings are in
[live loading](native_script_live_loading.md). No full-template acceptance follows
from this partial lifecycle probe.

The importer fixture passed 35 synthetic assertions without external data, plus
144 actual Mario DAT assertions covering 31 root imports and four deliberately
unresolved entity imports. Evidence:
`A:/FreakMelee-test-archives/script-metadata-oct07/receipt.json`.
The native client/tool build and 23 focused CTests pass at this checkpoint.

## Motion layouts, constants and combat, 2026-10-07

The scalar literal resolver contract materializes declared native constants and
validated named motion IDs into typed IR, preserving source spans and signed /
unsigned widths. Locals and owned declarations resolve first. The 978-name
constant catalog passed 6,038 checks and 978 independent native-reference checks;
its 16-file lexical census is spelling coverage only. Reference and evidence:
`tests/native/script_constants/README.md` and
`A:/FreakMelee-test-archives/script-constants-oct07/receipt.json`.

Explicit owned motion layouts validate move metadata and actual phase aliases.
The real state-change/scheduler sites bind code IDs, with all eight special entry
routes migrated. See `native_script_motions.md`; wider package metadata and
unsupported body operations remain open. Common-state startup diagnostics now
have accepted live/rollback observations; extended tables require installed owned
definitions rather than guessed bounds.

Generic damage/status/effect and RNG primitives are source-audited and use the
existing engine directly. The combat fixture passed 36,964 checks, including
12,240 exact RNG result/seed pairs and 30 native body comparisons. The effect
cleanup guard stops after parent retirement/replacement. Spawn/receiver, hitbox
and damage-reaction families still need migration. See
`native_script_combat_bindings.md` and
`A:/FreakMelee-test-archives/script-combat-review/receipt.json`.

The native client/tool and 25 focused CTests pass. Ordinary Workshop apply/content
loading still uses the old backend; this checkpoint does not accept complete
fighter templates or the finished production interpreter.

## Transitions, presentation and constant defaults

The complete bank compiler now configures a load-only constant resolver before
indexing parameter defaults, then removes its temporary context pointer. Defaults
use literal/namespace-only arithmetic and lazy evaluation with no host reads;
unknown/nonconstant names are rejected even in skipped expression branches.
UInt parameters, typed locals and helpers work; unsigned state/imported word
attributes reject their unsupported storage/schema shapes explicitly.

Contextual move transitions lower once into explicit owned motion/flags operands.
No source lookup occurs during execution; source retirement stops native startup
stages and later statements. Source/evidence: `native_script_transitions.md`.
Presentation outputs retain captured F32 lvalues, stage primitive results and
publish only after successful source guards; duplicate output destinations fail
compilation. Source/evidence: `native_script_presentation_bindings.md`.

The client/tool build and 27 focused checks pass. Fighter pool live/rollback
observations in `native_fighter_snapshot_ownership.md` do not close the remaining
whole-snapshot or ordinary Workshop/package migration gates.

## Item self operations and script snapshot owners

The direct catalog now supports generic native Item fields and self operations,
including unsigned command/group identities, holdable schema checks, physics,
lifetime, surface reflection and audio. The source-backed Item fixture passes
10,966 checks, 2,688 native physics comparisons, 96 sound comparisons and 128
production source-to-VM ticks. See `native_script_item_bindings.md` for pending
cross-owner, spawn and scheduler boundaries.

Declared Fighter script state and bindings have explicit same-process snapshots
with scene/program/lifetime checks and restoration without authored callbacks.
The actual registry fixture passes 191 checks; the live optional owner overlay
passes 64 complete rollback windows. The full match remains on the broad oracle.
