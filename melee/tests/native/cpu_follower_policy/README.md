# Remaining authored follower CPU: SDK71 design receipt

Read-only implementation proposal, not an implemented CPU policy or gameplay
acceptance. Accepted `cpu_early_policy` and all production sources are unchanged.
The proposed smallest complete policy surface and ordered FMS translation are
in [API.md](API.md). This namespace extracts actual definitions from immutable
SDK71, including the complete mode switch, recording, final overlay, destination,
planner and CPU-coupled capture bodies. No compiler, C fixture or game ran.

## Reproduction and inputs

```powershell
python tests/native/cpu_follower_policy/census.py --snapshot .local/baseline71 --output A:/FreakMelee-test-archives/cpu-follower-design71-fresh
```

Output must be a new directory. The runner verifies each input against the
snapshot freeze manifest before extraction. It records full-file and actual
function-body SHA256, source line references, lexical call/field inventories,
all raw Nana/Popo references in the audited CPU units, exact motion enum values,
and each top-level mode case. Lexical calls are not a transitive semantic call
graph. Extraction requires a definition prefix, not a callsite match.

Final audit output: `A:/FreakMelee-test-archives/cpu-follower-design71-receipt/`.
Frozen manifest SHA256:
`820f4d64f0809c1d9cb412d54d84797b689434b753f714216179397e3c274b60`.
The checked-in `source_inventory.json` contains41 extracted definitions and13
verified source/header/compiler inputs; large extracted body texts stay in the
A: output. `source_manifest.json` hashes this namespace's immutable handoff files
and the extraction output. Historical preliminary audit outputs remain separate.

## Exact ownership gaps

Source paths below are relative to `sdk/` in the frozen snapshot. Line numbers
are for the frozen source, with exact body hashes in the machine inventory.

| Source | Remaining decisions | Required closure |
| --- | --- | --- |
| `src/melee/ft/kinds/ftCommon/ftCo_0A01.c:2933` A589C | First same-player non-self actor; sleeping first match returnsNULL | Explicit script-selected full-width family member ID after publication; safe live reads |
| Same:7643 B0918/inlineM0 | Advance two30-record cursors; physical controller recording | Substantive generic bounded record operation, no inferred partner |
| Same:7683 B0AF4/inlineJ0 | Final delayed output, Lw synchronization, motion-gated position/facing smoothing | Output replacement hook and authored own special helper |
| Same:7728 B0CA8 | Common motion/held-item/relative motion eligibility | Ordinary script comparisons and a live position-delta read |
| Same:7803 B0E98 | Grounded, recovery-lock/motion/behavior, velocity-distance and radius checks | Authored ordinary state and common destination-latch operation |
| Same:7881 B101C | Flags, target, recording, level, copy/recovery/stop decisions | Whole mode replacement hook; no B101C delegate |
| Same:3948 A7AAC | Floor/actual island/edge/ledge destination decisions | Authored solver using typed geometry queries and actual common destination setter |
| Same:7611 B0760/inlineI3 | Common combat setup mixed with implicit partner destination | Split actual common preparation; script chooses destination; actual common planner |
| Same:8924 B3900 | Continues pipeline and increments x7C after potentially authored calls | Direct generation/userdata/kind/data/Definition continuation checks |
| `src/melee/ft/ftcpuattack.c:1027` B683C | Popo initial special chance, then common pummel/throws | Existing capture/pummel hooks plus real continuation/victim safety |

The new policy needs no private Popo/Nana move-variable union or C follow
solver. Owned motion IDs come from named recipe parameters: common count341,
ground/air Lw357/358, and specialHi_0..5=361..366. Literal361..366 in inlineK0
really matches that latter six-state range, not all ten earlier UpB startup/
throw states347..356. Common CliffCatch/CliffWait are252/253. Motion metadata
x2225_b3 is common row x9_b0, not a follower-owned persistent bit.

## Raw stock CPU exception inventory and accepted coverage

The census contains every literal Nana/Popo reference in ftCo_0A01.c and
ftcpuattack.c. They divide as follows:

| Frozen source lines | Exception | Status |
| --- | --- | --- |
| 481 | Nana rebirth/drop wait schedule | SDK71 on_cpu_drop_command already closes it |
| 571 | Nana periodic spacing1*rand | Existing on_cpu_spacing_update; authored recipe needed |
| 721 | Nana initialization mode6/distance15/initial candidate flag | SDK71 on_cpu_prepare already closes it |
| 852 | Initial30 pose/facing records from raw partner | SDK71 validated history hint/returnedID already closes it |
| 908 | Initial Nana approach spacing1 | Existing on_cpu_initialize; authored recipe needed |
| 4977 | Nana stickstep64/limit127 | SDK71 two scalar hooks and five outer continuations already close it |
| 7674,7690 | Match partner Lw and directly enter stock Popo Lw | New owned output policy and direct own scripted helper needed |
| 7705..7706 | Exclude own Lw from history-facing overwrite | Named owned motion comparisons in output policy |
| 7842..7843 | Own six-state UpB recovery-lock predicate | Named owned range in mode policy |
| ftcpuattack1125 | Popo capture special chance runs shared CPU script59 | Existing capture_decision/pummel_special hooks need authored recipe and continuation guards |

The raw same-player lookup also appears without kind spelling in A7AAC,
B0AF4 and B101C, so a literal kind census alone is insufficient. The proposed
mode/output hooks close all three for the owned follower. The CPU-coupled
capture special uses the common imported command table; verify script59's
owned bytes as part of pair asset closure. It is not license to call a stock
Popo fighter handler. All other Popo fighter coordination, rendering, specials
and common player role transfer are outside this bounded CPU proposal.

## Complete default mode inventory

No callback changes the following switch when absent or when returningfalse.
The follower's mode6 callback must returntrue on every owned path, including
missing/sleeping explicit target. No callback falls through to B101C merely
because a target or history record is unavailable.

| Mode | Actual default update |
| --- | --- |
| 0 | Clear common target modes/flags, behavior latch, filtered floor below current position, set destination if found, ADE48 |
| 1 | Same setup but floor below initial cpu.x98; missing floor falls back to current position; ADE48 |
| 2 | B04DC |
| 3 | Same current-position floor setup as0; ADE48 |
| 4,11,12 | B24B8 |
| 5,16,other unrecognized | No explicit case; default B24B8 |
| 6 | B101C original follower |
| 7 | AF290 |
| 8 | AECF0 |
| 9 | B00F8 |
| 10 | AFC40 |
| 13 | B126C |
| 14 | B1478 |
| 15 | Set behavior0, return; no mode planner |
| 17 | B17D0 |
| 18 | AF78C |
| 19,20,21,22 | AFE3C with respective argument0,1,2,3 |
| 23,25 | CpuInitEnemyTarget, mutating behavior/destination latch, A75DC destination toward common enemy if eligible, ADE48 |
| 24 | B1DA0_noinline2 |
| 26 | CpuInitEnemyTarget, behavior latch, filtered floor below current position, set destination if found, ADE48 |
| 27 | B1EF0 |
| 28 | B21C8 |
| 29 | B1AB8 |

The per-frame B2790 behavior switch and B3E04 interpreter still run after mode
planning, then the final output overlay. These are distinct from the mode
switch; replacing mode6 must not accidentally skip normal queued commands.
Missing partner's original AEFB8 runs common recovery/target/no-target planning,
not just neutral inputs. Copy-disable clearing preserves cstick. The source
stores the same physical trigger Float into both byte channels; no255 scaling.

## Precision and undefined-source policy

Negative inlineM0 Float-to-u8 conversion is undefined native decompiled C.
Signed quantization is the recommended deterministic behavior choice, accepted
as a UB fix by the user; no claim of a retail console bug without PPC evidence.
Defined positive sticks and normalized trigger conversion remain strict source
comparisons. Smoothing uses original double0.95/0.05, changes only x/y and
conditionally facing, retains z. Level uses percent/20 truncation and only an
uppercap9; undefined nonfinite/out-of-Int narrowing is separately classified.

This is source-audit evidence, not a source-phase comparison matrix. Future
implementation must execute generated callbacks inside actual full CPU bodies,
compare ordered common planner/RNG/geometry boundaries, and test retirement/
recycling before asserting acceptance. Proposed exact APIs, invalid contracts
and a coherent matrix are in API.md. No game, build, package or default changed.
