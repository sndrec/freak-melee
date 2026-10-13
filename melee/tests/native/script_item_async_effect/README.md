# Owned Item async joint effects

Run `run.py --compiler <MSVC-target clang.exe> --output <fresh A: directory>`.
Every subprocess is hidden; no game, audio device or renderer runs.

The cohort uses actual source compilation, prepared common Item model selectors,
bytecode, VM, Item actor/storage, GObj pool and direct Article feedback backend.
It reuses the controlled imported-asset/spec-provider setup from
`script_article_feedback`; its source and native userdata construction remain
explicit fixture boundaries. No original fighter or Article policy wrapper runs.

Unchanged native async enqueue/process/flush/clear/init, eight actual synchronous
and alternate dispatcher cases (two Joint and six facing-helper cases), the facing helper and generator joint/refcount attachment are
compared with reference `4b32ba62a`. The queue allocator is a fixed sixteen-packet
reservation; generator assets/allocation and AppSRT allocation are controlled.
Tests compare native packets/results, scalar sampling, both queue timings, LIFO,
capacity failure, cancellation, source ownership guards and real Article/Item VM
calls. The full native argument classifier is compared with the source-derived
ABI inventory, including the three forwarded facing-helper cases.

Position modes 1 and 4 use controlled joint-position sampling and typed recipe
captures for effects 1230/1231. The actual native queue decides when to sample
the joint and copies mode 4's scalar. These checks cover immediate/deferred
processing, source-to-VM calls, nonfinite scalars, incompatible recipe families,
Fighter-only recipes, missing graphs, allocation failure and source retirement.
They do not execute the two recipes' native particle/model constructors or prove
HSD world-matrix calculations.

The queue borrows its joint without increasing its reference count; generation
guards around enqueue do not make arbitrary later graph replacement safe. Native
Item state/accessory flush and removal cancellation remain engine contracts.
Production ObjAlloc can grow backing memory, so this fixed reservation does not
establish allocation-free ticks. Particle commands, visual appearance, imported
DAT validity, full actor constructors and gameplay remain outside this proof.
Immediate resource-leaf mutation probes test publication guards; this native
particle path has no authored callback, and completed side effects are not undone.
