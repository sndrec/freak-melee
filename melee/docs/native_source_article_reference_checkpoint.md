# Direct interpreted Article references

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The ordinary interpreter supports read-only `articles.ref(handle,DeclaredArticle)`
from Fighter and Article callbacks. This is not a generated callback, native
Definition adapter, or lookup by arbitrary ItemKind. A literal source declaration
selects its owner; fields resolve to immutable value-declaration IDs at load.
The root capture is logically pure and works in read-only predicate callbacks.

## Selection and observation

The root call produces an invocation-retained Item token before later selectors,
including helper-computed array indices, execute. The existing fixed receiver
pool retains physical GObj slot/generation, scene epoch, native generation,
userdata identity, program/storage pointers, layout/parameter identities and
storage owner. Article validation additionally checks exact published author
and declaration. It never requires current damage ownership to equal the caller.
Fighter relation captures retain their existing behavior; caller userdata and
storage-owner changes now also invalidate that invocation.

Supported projections are `.exists`, `.current_owner`, scalar `.state.field`,
and `.state.field[index]`. Scalars retain Bool/I32/F32 declaration types; owner
IDs return U32 without losing their high bit. Fields are read-only, parameters
and Fighter-native projections do not leak through, and arrays are semantic
namespaces until indexed. No fake aggregate Object is returned for a bare array.

Initially missing, foreign-bank or different-declaration handles capture null.
Their scalar reads return typed zero, as the original reference API did. Negative
or out-of-range integer array indices also return zero. A captured target that
then retires, changes schema, or is physically reused faults a scalar/model read;
it cannot reacquire a replacement through the original integer handle. Existence
queries report false for retirement. Frame/caller faults preserve outputs.

Removing Items remain admitted for state/existence as in the original API, while
current-owner and model queries retain their separate retiring-item behavior.
A faulted Item's still-live data view remains readable without executing its
behavior. Script signals independently preserve their fault-delivery contract.

`articles.read_primary_translation(handle,DeclaredArticle,x,y,z)` captures one
instance and reads its engine-typed primary JObj's **local** translation, not Item
position or a world transform. Three captured F32 outputs publish only on true.
Missing/retiring targets and absent models return false without writes; replaced
roots or invalid identities/classes stop publication. Engine class/extent and
pointer checks do not claim a separate per-JObj allocator-generation mechanism.

## Evidence and remaining acceptance

Private receipts under `A:/FreakMelee-test-archives/`:

- `script-article-ref-catalog-oct08-root-settled/receipt.json`:93 checks, actual
  source/index/compiler/VM and receiver dispatcher, precise owner/field/index
  lowering, readonly predicate capture, literal declarations, output layout,
  malformed calls and accepted-bank preservation. Native reference leaves
  deliberately reject; asset metadata is controlled.
- `script-article-ref-core-oct08-root-settled/receipt.json`:4352 checks and9 actual
  execution runs, existing receiver routes plus Article core capture/validation,
  native registry and physical pool reuse. Program/storage views and unrelated
  engine/motion leaves are controlled; no complete game/render acceptance.
- `script-article-refs-oct08-root-final/receipt.json`:84 checks, actual source
  preparation/publication, Fighter/Item actors, VM and receiver pool. Bool/int/
  Float reads, absent defaults, high-bit reflected owner, retiring/faulted data,
  local translation versus Item position, model absence/class/root replacement,
  and invalidating index-helper deletion/reuse are covered. Assets and native
  translation/write leaves are controlled; no DAT constructor/render acceptance.

The client/content tool and affected fixtures build;36 selected CTests pass.
Actual private-owned-data preflight advances Zelda from reference196 to the
unsupported vector-angle call287. Sheik still stops at Chain resources1543.
Whole forms are not yet accepted. Continue generic vector math and ownership/
child creation, followed by Chain collection/model/collision/matrix closure and
paired runtime acceptance. Explicit rollback state/performance remains open.
No game launch, installed-content update or public/user draft migration occurred.
