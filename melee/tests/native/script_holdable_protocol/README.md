# Workshop holdable native comparison and real source execution

From the repository root:

```powershell
python tests/native/script_holdable_protocol/run.py --compiler A:/FreakMelee-test-archives/baseline72/compiler/clang.exe --output A:/FreakMelee-test-archives/script-holdable-protocol-review
```

Use a fresh A: output directory. Compiler and fixture consoles are hidden.
`--case healthy`, `held`, `fault` or `retirement` isolates a family; default `all`
requires the complete fixture to pass. Artifacts include logs, exact commands,
generated actual bodies and hashes of compiled/extracted inputs and headers.
Checks fail with a nonzero exit independently of NDEBUG. No game or global build
is run.

The production metadata/compiler prepares Plain, Custom and Unbounded recipe
specs plus an empty Article. Plain/Unbounded and the Article declare no state,
parameter, helper or hook; real zero-byte owner rows are required. Actual Item
actors/storage/VM, receiver/actor access, GObj pool/proc/deferred
removal and common state/destructor bodies remain linked. The custom thrown,
tick, hit and ground_collision bodies execute interpreted; they have no authored
C surrogate. No-hook comparisons select the actual extracted legacy state table.
Only local baseline function identifiers are renamed to avoid collisions.

Thirty-two parameter trials compare actual pickup/drop/throw and hitcaps, eight
age/physics ticks, hit/clank/reflect/shield behavior, and all16 contact masks.
Held/resting collision tests ensure no geometry/hook activation. Other cases
cover custom tick gravity suppression, hook Bool decisions, held neutral age,
unbounded lifetime and u16 wrap, fault rejection/output preservation, reentrant
hook/native-stage retirement, cleanup-spec availability and independent child
survival after caller retirement.

The complete review run passes5802 checks in18 cases, including1024 comparisons
to actual native bodies. The preparation regression verifies empty owner rows
and all absent hook IDs without adding dummy declarations.

`boundary.c` controls imported Article/model descriptors, graphics/audio leaves,
scene contact observation and ground response, and published program lookup.
Position writes trigger selected reentry/removal scenarios; all authored decisions
and owned state access still use the actual VM. Generic hitcaps initialization,
damage with no Fighter owner, gravity/spin, victim bounce, reflection, shield-vector
mirror and facing use actual native bodies. A Fighter damage-owner branch is
explicitly unavailable and aborts if reached. Unrelated operation families reject
calls rather than provide gameplay fallback. Retail assets/rendering, real scene
geometry, authored Fighter caller VM/factories and aggregate rollback are separate
acceptance work.
