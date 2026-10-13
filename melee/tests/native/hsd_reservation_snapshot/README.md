# Reserved HSD owner snapshot

The current native implementation uses shared contiguous HSD backing for
ordinary class pieces and ObjAlloc chunks. `class_arena_cases.h` tests prebound
copy-only history over both families, including their counters/free lists and
initialized metadata prefixes. Class-library amnesia preserves ordinary objects
until the actual heap reset; private history retires at both boundaries.
`hsd-span-oct12-owners-final/receipt.json` passes 88,570 checks. The original
packed-component and OS-heap cohorts below remain diagnostic companion tests;
ordinary chunk growth now uses the shared arena, not OS allocations.

Run `run.py --compiler <clang.exe> --output <fresh A: directory>` under the VS
native environment. The fixture executes actual HSD class-piece/ObjAlloc,
reservation, native pool/serializer and Aurora OSAlloc code. Only diagnostic
assert/report leaves and caller-owned synthetic payloads are controlled in the
original cohorts. The camera cohort additionally controls the GM idle predicate,
camera constructors and unused graph/render/callback endpoints as described below.

Two reservations share an ObjAlloc descriptor alongside an ordinary free slot.
Four bins contain four occupied records. Save/load preserves bytes, generations,
free order and the ended construction failure flag. Reuse invalidates old refs;
restore resurrects them. Invalid late owners/pools, truncation, trailing data,
duplicate free slots, wrong epoch, overlap and an active claim reject without
writing any owner. The shared descriptor and ordinary heap remain unchanged.

This tests physical ownership, not arbitrary graph-pointer validity. Borrowed
graph references, shared allocator/class counters, ordinary heap storage and
source registries require companion owners in the aggregate snapshot. No game,
GPU, whole-match rollback or timing claim is made here.

The shared ObjAlloc component is tested with two actual descriptors, ordinary
fixed-arena allocations and a reserved allocation. Restoring both components
recovers usage/peak/limits, free-list heads and the arena cursor. Ordinary payload
and free-link bytes are restored by the actual component, without a copied arena
companion. Ten malformed/stale descriptor
candidates reject without writes; same-shape reinitialization retires old history.
The 256-descriptor ceiling and empty registry are exercised. No OS heap or class
counter ownership is claimed by this component.

The same component now owns ordinary OS-heap allocation blocks as well. An
additional cohort mixes ordinary and reserved allocations, mutates live/free
records, grows the OS heap, rejects malformed late block records, restores exact
bytes/counters/free order, and checks the same future growth allocation address.
OS heap/Cell metadata now restores through Aurora's actual explicit component.
Reserved payload restores through its actual reservation component.

The ordinary class owner saves actual class-piece chunks, size-class tables and
entries, initialized class live/peak counts, and free links. The fixture uses
real hsdNew/delete, table growth, splitting and recycling. It mutates occupied
records and grows the heap, rejects malformed candidates without any writes,
restores exact bytes/counts/free order, then verifies the same subsequent
allocation addresses. Aurora's actual OS component restores headers while the
class component restores payload bytes. A late initialized class or
retired scene rejects old history. This is not aggregate or timing acceptance.

The OS cohort covers real split, reuse, free/coalesce, exact next allocation
addresses, preserved payload (headers only), current heap selection, two heaps,
foreign ownership/overlap/cycle/truncation refusals, disjoint ownership after
fixed carving, geometry/lifetime retirement and the 65,536-cell scratch limit.
The ID cohort pairs real root/entry and OS components across colliding full-width
keys, removals, reused slots and growth; absent and present-null lookup remain
distinct. Root loading never walks an intermediate graph. Root setup/forget
retires history. No whole-match timing or aggregate coverage is claimed.

The camera cohort uses actual class-piece allocation, class snapshots and the
generic GObj pool companion with native CObj records and production camera
state/snapshot methods. It checks fixed class-backing references, scalar/debug/
coefficient restoration, sixteen repeated controlled state updates, valid union
callback tags, unaligned input, fourteen malformed candidates, buffer overlap,
activity and scene/heap retirement. Reused primary/quake GObj generations reject
until the pool companion restores them. Class payload/type corruption rejects
before camera writes; the class companion restores the matrices/flags separately.
No callback, draw or constructor is invoked by camera Save/Load. The idle
predicate and synthetic CObj initialization are controlled; unused camera graph
endpoints abort if called. This is owner restoration, not camera gameplay or
complete match restoration. Live stock/scripted eight-actor census is separate.


The Green Greens grid cohort now binds actual Ground and Item pools and ordinary
class joints to controlled GObj records. It exercises active cell generation,
userdata role, material owner/joint and class-type agreement, while dormant cells
retain invalid pointer bits without being followed. Late disagreements reject
before copying any grid; actual companion snapshots resurrect reused slots.
The item destructor endpoint frees the actual pool only; no item asset teardown
or complete match simulation is claimed. A sixteen-KB untouched capture buffer
is checked byte by byte when the last material's owner is wrong.


Retained span queries return `NativeClassReference` / `NativeObjAllocReference`
by value, with zero bytes on failure. Class epoch zero is valid at startup;
the fixture checks an actual `hsdNew` record before the first amnesia call.
Explicit Save/Load buffer-overlap and malformed reference tests remain enabled.
`--benchmark` optionally times one million class and ordinary queries (including
Resolve), plus 100,000 controlled particle captures. Timing-loop assertions do
not contribute to coverage counts. This small controlled graph does not establish
whole-match or rollback latency; live eight-actor census provides separate data.

HUD coverage includes timer/countdown pointers and phase, retained score flags,
stock-indicator objects, stock counters and effect queues. The control fixture
restores their actual values and rejects malformed object references before
writing. Scene-history renewal retains live payload/cursor contents, clears
admission, and invalidates older spans even when backing is reused. These tests
do not replace the live match-end and sudden-death checks.
