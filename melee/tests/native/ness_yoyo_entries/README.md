# Ness YoYo entries and Bat article phases — unregistered source fixture

This namespace owns a separate candidate. It does not alter the shipped Ness
example, the accepted initialization/solver fixtures, or the Thunder candidate.
It provides source acceptance for the remaining article entry, material and
animation decisions. Ness smash phases and complete default activation remain
unimplemented.

Run from the repository root on Windows:

```powershell
python tests/native/ness_yoyo_entries/run.py --snapshot .local/baseline39 --output .local/ness-yoyo-entries39
```

The runner verifies every supplied snapshot SDK/compiler input and content tool
against `freeze.json`, verifies the original source files against
`original_inputs.json`, extracts unchanged function bodies, generates candidate
C with snapshot `script-c`, and directly compiles against only that SDK with its
Clang. It selects MSVC 14.44 and disables FP contraction. It never calls
snapshot `script-build` (which embeds live SDK paths), starts a game, or modifies
the immutable snapshot. Outputs and binaries belong only in the supplied output
directory. The candidate's parameter declarations are lowered into the supplied
FMP/default-word table before generation; the harness installs those words
through the actual generated parameter setter/schema check.

## Compared source and authored mapping

| Original | Candidate |
|---|---|
| `it_802C0010` | `Held` signal/helper: tip attachment matrix translation, tip position, current/last collision history, pin/draw state, input velocity, state 1/flags 2 |
| `it_802BFE5C` | `Released`: complete input velocity first, X absolute value times facing, state 2/flags 2 |
| `it_802BFEC4` | `Retract`: head attachment matrix translation, XY distance, named retract fraction, state 3/flags 2 |
| `it_802BE5D8` | `Material`: remove animation, add material-only Spin, rate 1, request supplied frame, advance |
| `itNessyoyo_UnkMotion3_Anim` plus its two inline helpers | All four animation callbacks: saved creator spin rotates existing tip child before checking current damage-owner motion 342–347 |
| `it_802BE958_inline` | Expiry requests current owner's exported `yoyo_forget`, clears current owner, releases nodes in reverse order |
| `ftNs_AttackHi4_YoyoItemSetFlag/SetUnk2`, `it_802BE5B8` | `yoyo_forget` resumes the still-published handle before clearing it; the actual handle is inspected at the resume boundary |
| `itNessbat_UnkMotion0_{Anim,Phys,Coll}` | Bat smash mode: command-controlled child visibility; creator motion 341 and live named Bat handle gate; forget only when creator is current owner, then detach |
| `itNessbat_UnkMotion1_{Anim,Phys,Coll}`, `copy_jobj_scale`, `ftCo_800BF228` | Bat presentation mode: Sleep/DeadUpFallHitCameraIce gate and creator **root-joint** Y propagated to all three item-root scale components |
| `ftNs_AttackS4_CheckNessBatRemove/ItemNessBatSetNULL` | Named creator Bat handle/motion check and `bat_forget` helper |
| `it_2725_Logic59_EvtUnk`, `itNessBat_Logic58_EvtUnk` | Same item interaction boundary; YoYo's original volatile no-op comparison remains only in the unchanged reference |

Held uses the resource's **attachment** matrix, not its drawn-model matrix.
Its older `CollData.prev_pos` remains untouched. Released/retract preserve node
position/history/model matrices; only the source's velocity or retract scalar
and state transition change. `pinned[]` is ordinary authored state; draw
registration is synchronized through the generic resource API.

The initial six-argument resource creation is reused to build valid models and
nodes for this comparison. Initialization and four physics solvers already have
separate accepted fixtures; this run does not claim another solver acceptance.

## Coverage and numeric boundary

The gated matrix contains **2,604 source pairs**:

- 1,078 entries: 54 held/released/retract cases at 2/8/90 nodes, both facings and
  negative/zero/positive input X, plus 1,024 deterministic retraction inputs
  spanning coincident/small/large XY distances.
- 12 material requests: zero, one, fractional and negative frame values.
- 1,152 YoYo animation cases: every phase, every valid owner motion 342–347 and
  both outside boundaries 341/348, creator/current-other-owner/NULL current
  owner, published/absent handles and existing/missing tip child.
- 360 Bat pairs: both states, smash/non-smash and both incapacitated motions,
  published/absent handle, creator/other/NULL current owner, three command
  values and present/absent saved creator. Inert physics and collision are also
  invoked and compared for both states.
- Two item interactions.

The original retraction retains its three double-precision Newton refinement
steps over the native `__frsqrte` definition. The authored candidate uses ordinary
`sqrt`. Both final retract values are measured and checked using a 2e-6 relative
bound; the receipt records the maximum absolute sqrt difference separately.
No stack-spill/undefined-state behavior is imitated. Zero observed difference
in this matrix is not a universal bit-identity claim or GameCube estimate claim.
No RNG is consumed by the tested callbacks or the input generator in gameplay;
the deterministic integer generator belongs only to the numeric fixture.

## Actual production units and controlled engine boundaries

`native_article_resource.c` is included directly from the frozen SDK and
`native_model_joint.c` and `native_fighter_target.c` are compiled directly from it. The original resource
construction helpers and matrix bodies are mechanically extracted unchanged.
The retained Dolphin math fragment has independent source/fragment hash receipts.

Controlled boundaries are HSD allocation/model loading/animation evaluation,
GX registration/free notifications, subtree hidden-flag engine helpers, item
state transition and interaction, and live creator/recipient
identity; current-owner motion uses the actual production registry path. Signals and exports resolve immutable generated metadata and invoke
its actual callbacks with full high-bit creation IDs; these controlled
resolvers do not substitute for the separate accepted generic runtime resolver
fixtures. Allocation/rendering and real collision geometry are not accepted by
this matrix. No stage sweep is requested by these entry/animation functions.

Every cleanup event observes operation/node order. A reverse free sequence,
resume while the real named/source handle is still published, owner clearing,
creator/current-owner separation, final pin/history/matrix state and Bat dirty
flags are asserted. Model/animation boundaries are explicit; pixels and real
material progression are not compared.

## Remaining boundaries

`holder.motion` now lowers through the actual `nativeFighterTarget_CurrentOwner`
and `nativeFighterTarget_Motion` live registry operations. Both are compiled from
the supplied snapshot; `ftLib_8008741C/80087460` registry bodies are extracted
unchanged and source-hashed. The original 2,604 healthy source pairs are retained.
Sixteen additional authored-only cases cover all four animation phases with
NULL owners, removed owners, removed owners with poisoned userdata and live
replacement generations. Original raw-pointer callbacks are deliberately not
invoked on undefined stale-owner inputs. Missing motion returns -1 and expires
the chain safely; a live replacement in a valid motion continues.

A retained integer ID rejects a replaced generation. In contrast,
`item.current_owner` samples the current live object at the raw owner address;
it cannot recover the historical identity of an address that has been recycled.
The fixture explicitly checks this distinction instead of claiming otherwise.
Frozen39 passes 1,370,954 checks with zero failures, retaining all 2,604 source
pairs and adding these 16 authored-only safety cases. The measured numeric
maximum remains zero. Receipt: `.local/ness-yoyo-entries39/receipt.json`.
Historical frozen36 acceptance is preserved separately.

YoYo rotation's unchanged original also requires a valid saved creator whenever
the tip child exists. Bat's absent creator path is defined and covered. Full
smash entry/phases, charge fields/hitbox reset/ECB query, aerial jump, CPU policy,
ordinary item notifications, versus presentation and final four-resource
lifecycle activation remain separate work. This candidate is never registered
as an example and does not declare independent behavior.
