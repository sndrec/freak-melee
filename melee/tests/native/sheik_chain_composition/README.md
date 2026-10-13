# Sheik Chain source composition (unregistered candidate)

This namespace authors the entire Chain move and article policy. SDK64 console
comparison passes 58,153 grouped pairs and 24,077,957 checks, with maximum measured
float difference zero. This is a source policy gate: no registered Sheik base,
Workshop default, package installation, independent whole fighter, retail asset
import or rendered gameplay is accepted here. Accepted Needle/Vanish fixtures
remain unchanged. `compose.py` borrows the Needle component to preserve actual
Needle then Chain damage/death2 cleanup.

| Comparison unit | Pairs | Checks |
| --- | ---: | ---: |
| Fighter policies, two entries, all 24 phases and composed cleanup | 34,516 | 945,967 |
| Four article solvers and collision/history observations | 17,856 | 15,600,659 |
| Article animation/accessories/render/entries/constructor/cleanup | 5,781 | 7,531,331 |

The runtime count includes explicitly labeled safety controls, including 71
model-activation failure cases whose original constructor would dereference a
missing model. They are not claims of defined original behavior. The earlier
complete receipt is retained at
`A:/FreakMelee-test-archives/sheik-chain-complete64-r4/receipt.json`; the final
self-contained reproduction writes its own fresh receipt and source hashes.

## Reproduce

Run from the checkout with the immutable SDK64 and a fresh A: output directory:

```powershell
python tests/native/sheik_chain_composition/run.py `
  --root C:/gayms/freak-melee/melee --snapshot .local/baseline64 `
  --output A:/FreakMelee-test-archives/sheik-chain-complete64-final
```

The default ISO is the USA v1.02 image beside the checkout; `--iso` may override
it. The runner decodes actual PlSk.dat tuning, constructs the owned FMP, uses
frozen `script-c`, and compiles generated C plus actual original bodies with
frozen Clang/SDK and explicit MSVC 14.44. It does not use `script-build`'s embedded
live include paths. There is no game process or normal clone/import because a
copied Sheik catalog base is not yet registered. Typed DAT import is a controlled
fixture preparation boundary, not Workshop import acceptance.

`prepare.py` may be run separately with the same flags plus `--iso`; its receipt
reports PREPARED, never PASS. `run.py` hashes the untouched frozen originals,
corrected reference, candidate components, tools, generated module and executable
inputs. `freeze.json` SHA256 must be
`7301e8b4b16357d886bb305689a75e8ec1fe01f518e926c348db83cc714ab333`.

## Source coverage and ordering

`fighter.fms` supplies entries 349/352 and all animation, interrupt, physics and
collision phases 349–354. Startup ground/air conversion retains actual
`0x0c4c5088` flags and current frame. Loop ground loss enters ground end; loop air
landing enters air end, preserving those original choices. Article animation
observes leaving 349–354 and tears down resources; no invented exit callback is
added. Tests include source-derived spawn/retract/destroy tick boundaries,
circular input wrap/deadzone/magnitude, pose smoothing, sound cooldowns, motion
conversion, endpoint initialization, timed rehit, expiry/grace, constructor
failure, pause/resume and composed Needle-before-Chain cleanup.

`article.fms` supplies construction, all five animation states, picked-up and
interaction callbacks, five accessory behaviors, launch/retract, pause/resume,
owner notification and teardown. Resource 0 is original `x4` (head), resource
`count-1` is `x0` (tip), `next` decreases indices and `prev` increases them. The
four solvers are 802BBD64, 802BBED0, 802BC080 and 802BC94C with actual
`Item_RetractChain`. Initial active prefixes, edited counts 3/6/12/20/30, gravity,
history shifts, wall/floor contacts and sound stride are compared. Render follows
actual 802BCB88/802A6DC8 normalization, axis selection, cross products, matrices,
nine length multiplications and endpoint publication. TipAtHand affects the tip
resource matrix; primary-model hiding remains recursive. `ITEM_ANIM_UPDATE` is
actual flag 2. Constructor success and every resource allocation failure follow
the unchanged source initialization and release ordering.

Shared pose, stage contact, allocation/model activation, matrix/joint, audio,
notification and transition boundaries are controlled and recorded. The complete
actual `common_attribute` function is extracted, so named stick smoothing is
compared against actual `COMMON_DATA_F32[275]`/offset 0x44C. Pose choices and
independent blend weights remain script decisions. Actual pose descriptor/clip
validation and the swept-point backend have separate engine fixtures. This
namespace does not present synthetic graphs as retail costume/render acceptance.

Lifetime controls use actual native source registry/generation checks. Removal
and reuse during pose evaluation, motion change, Needle spawning, owner dispatch
or item transition stop subsequent cached accesses. Owner motion includes
348/349/354/355 and absent owner; retained creator remains distinct from mutable
current damage owner. Notification dispatch uses the explicit `chain_lost` export,
not foreign private state. Cleanup is idempotent after resources/owner clear.

## Native layout repair is a separate reference

Frozen64's `it_802BC080` reads `co.common.x1C` at native move-union offset 32,
which aliases Sheik's resume timer. The intended rehit timer `sk.specials.x1C`
is offset 28; `sk.specials.x20` is 32. `article_solver_test.c` asserts these
offsets and runs both old and corrected bodies. The old native failure remains
archived at `A:/FreakMelee-test-archives/sheik-chain-solvers64-r1`.

`corrected_control_actual.h` is the unchanged actual body from root's typed
source repair commit `5859967ad`, with complete original source SHA256. It is
the default corrected reference; an explicit `--corrected-chain-source` can
supply a full repaired source file. The runner proves that, ignoring comments,
the only difference from frozen64 is the typed timer read. Frozen64 is not edited
or silently rewritten. The authored named rehit timer matches the corrected
retail intent, not the native-width-shifted common-union alias.

## Ownership and excluded boundaries

Chain directly accesses original enum/raw part 26. Retail semantic L3rdNa maps
raw 27, so replacing this direct access with the semantic mapper would change
source behavior. Spawn and attachment therefore use named raw part 26. Pose
graphs are named slots 4/5 with path `"0"`, clips 305/308 and traversal raw start 1.
Seventy-five named values are owned after import; fighter state is 168 bytes and
Chain article state 308 bytes, with resource capacity 30 and 15 history entries.

Original constructor leaves previous rehit `x18` uninitialized; the script
initializes it to zero. Control's final `env_flags` is undefined if propagation
runs no subsequent resource; the script defines it as zero and excludes that
undefined original comparison. Original missing-Chain launch/retract dereferences
are excluded; typed signals safely reject missing handles. Counts below 3 or
above 30 and missing activated models receive defined rejection/cleanup instead
of undefined source memory access. These judgments are explicit; healthy state,
branches, calls, ordering and numeric observations remain compared. There is no
stack-spill imitation, RNG workaround, blanket ignored field or stock Chain
handler delegation.

Remaining activation work is owned rig/pose/article registration, normal import,
whole-fighter load/reset/Transform composition, independent callback census and
hidden/muted drawn gameplay. No additional generic Chain API gap is identified
at this source checkpoint.
