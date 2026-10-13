# Native Item owner fixture

Runs the actual native Item pool, occupied-record serializer, complete item.c
model bone-table builder and userdata destructor, and actual HSD GObj lifecycle.
The two item.c bodies are extracted unchanged and hashed alongside all source
headers. Resource cancellation and script detach are controlled boundaries that
assert argument/liveness/order; graph destructors trap unexpected calls.

The Item constructor allocation/publication sequence is exercised separately;
asset selection, draw registration, gameplay callbacks and full initialization
are covered by the connected old/new gameplay cohort, not this fixture.

Cases cover immediate/deferred/userdata-only removal, exact generation reuse,
zeroed reused slots, optional/100-entry/oversized bone tables, occupied payload
and satellite restoration, GObj restoration, corrupt candidates with no mutation,
scene epoch rejection, full 4096-slot capacity and deterministic free order.
Scheduler roots and other graphs still use the broad oracle after typed loads.
The shared stock/authored ItemLink pool is also exercised: its complete stock
userdata destructor is extracted from itlinkhookshot.c. Cases cover actual GObj
retirement/reuse, chain links, occupied byte/free-order restoration, candidate
root/neighbor validation, epochs and full capacity. Actual authored resource
callbacks/models are covered separately by script_item_resources.
No complete-match snapshot, performance, visuals or multi-PC claim is made.

Run with `python tests/native/native_item_pool/run.py --compiler <LLVM bin>
--output <fresh A: directory>` from the checkout with the MSVC environment set.
