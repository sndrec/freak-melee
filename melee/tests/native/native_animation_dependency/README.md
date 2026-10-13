# Native animation owner and dependency fixture

```powershell
python tests/native/native_animation_dependency/run.py --compiler .local/baseline72/compiler --output A:/FreakMelee-test-archives/native-animation-dependency-proof
```

The runner requires a fresh A: directory, uses CREATE_NO_WINDOW for compilation
and execution, bounds each subprocess to120 seconds, and records source/header/
compiler hashes, extracted function hashes, exact commands and complete logs.
No client or renderer is launched.
`extract(root,out)` is importable by the CMake fixture. It generates exact
production heap/preload bodies, private packed-header layout and Hermite body;
standalone receipts verify the extracted source hashes before compilation.

The historical receipt
`A:/FreakMelee-test-archives/native-animation-dependency-opcodes/receipt.json`
passes64 cases/947 checks, with69 allocations/frees and32 target reference releases.
It compiles the actual complete FObj decoder, native RNG and lbAnim FigaTree
constructor sources. It extracts the actual AObj construction, interpret,
request, flags, timing, callback and removal bodies plus Hermite interpolation
body without rewriting their logic.

Five FLOAT packet streams (CONSTANT, LINEAR, SPL0, SPL, KEY) live on a Windows
PAGE_READONLY page. Loop/nonloop playback at rates0.5/1/2 snapshots after three
frames and replays eight actual interpreter frames. Complete AObj/FObj/target/
RNG/end-counter state replays exactly in all30 combinations. Frame-only restore
diverges. A separate negative control restores everything except the consumed
FObj and demonstrates a different next joint position, isolating decoder state
from RNG/callback bookkeeping. NO_UPDATE still consumes decoder state without
calling the mutation callback. Both real FigaTree installation paths preserve
channel order, replace/free the prior graph and filter interpolation translation.

That historical fixture used controlled fixed allocation. ID lookup and target
unref remain controlled reference accounting. Update/end callbacks are fixture
callbacks that mutate the real native JObj struct and invoke actual HSD_Rand.
JObj flag propagation is controlled and is not graph/renderer acceptance.
The private AObj endcallback list is injected by the fixture; current production
source only clears it, so this is a protocol dependency proof, not a claim that
the stock workload registers these callbacks.

The current fixture links actual full AObj/FObj/lbAnim/RNG sources and the actual
native typed owner/pool/snapshot/global-region units.
`A:/FreakMelee-test-archives/native-animation-pool-ctest-extraction/receipt.json`
passes90 cases/1079 checks (64 decoder,26 owner). Decoder replay now restores
through the production packed owner API. Owner cases cover actual construction,
free/reuse/resurrection and exact allocation order, fallback-JObj reference
ownership, wrong-pool/duplicate/cycle/cursor/counter/truncation rejection, both
families prevalidated before either loads, complete backing/metadata/global bytes
unchanged on invalid load, empty/full snapshots, actual4096/16384 exhaustion,
double/wrong-family free, active callback rejection and PE .mstate coverage.

The fixture runs actual ObjInit/CreateMainHeap bodies with controlled class,
other-family initialization and OS heap hooks. A reentrant reset rejects before
class state changes; normal reset retires occupied storage without destructors
and invalidates prior epochs. Actual lbDvd_80018254's unchanged-cache branch
leaves live animation ownership intact and reaches none of the trapped later
asset/heap stages. The fixture protects source streams read-only and verifies
that counter reset and list forget remain at their original source sites.

Empty packed owner bytes are163992; both families full pack1376408. These are
measured component bytes, not a complete match snapshot or timing result.
External JObj/refcounts, rendering/IK/constraints, archive residence and other
graph owners remain separate. Slope-only/quantized packet formats and live
event-heavy scene acceptance remain outside this console matrix. See
`docs/native_animation_snapshot_ownership.md` for scope and capacity evidence.
