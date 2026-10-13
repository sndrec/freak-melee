# Zelda/Sheik paired-form source composition

This is an **unregistered, inherited candidate**. It composes the accepted
Needle, Vanish, Chain, Nayru, Din and Farore components without changing them,
then adds both complete Transform policies, the owned pending-action CPU
decision, Zelda recovery and load-only Sheik handle initialization. It does not
activate a default template, register a family, install a package or run a game.

`compose.py` reads the accepted sibling components. The local FMS fragments
contain ordinary named parameters, state and callbacks; no stock special or
Transform handler is used by generated code. The Python composition helpers
are build-time source assembly, not runtime forwarding functions.

## Executed comparison

```powershell
python tests/native/paired_form_composition/run.py `
  --root C:/gayms/freak-melee/melee --snapshot .local/baseline68 `
  --output A:/FreakMelee-test-archives/paired-transform68-root
```

The runner verifies every frozen input before and after the run. It independently
decodes retail `PlSk.dat`/`PlZd.dat` tuning, produces owned FMP words, invokes the
frozen `script-c` compiler and compiles both generated modules with the frozen
SDK/Clang and VC tools 14.44. This is **retail parameter staging**, not normal
paired clone/import acceptance: the family package path is not registered yet.
The receipt records source, DAT, snapshot, generated-C, module and executable
hashes, commands and results. All outputs must be on A:.

Each member executes 8,960 original/generated pairs, all 16 Transform phases,
two ground/air entries, destination finish and accessory effect/transfer
callbacks. Retail tuning and an independently edited five-value profile cover
128 state/input combinations each. Both members additionally execute two
pending-action cases; Zelda executes 162 recovery queue comparisons, including
strict distance boundaries and every CPU level. Twelve destructive-source
cases per member exercise retirement/recycling at motion, animation, collision
conversion, finish and partner-activation boundaries using the real source
registry and inaccessible retired userdata.

The whole original Transform bodies come from the supplied snapshot's
`ftseakspeciallw.c` and `ftzeldaspeciallw.c`. Actual shared `ftCommon_Fall` and
`ftCommon_CalcSelfAccel_DeaccelAir` execute. Motion/animation setup, stage
contacts, joint-position output, rendering, wind, effects and partner transfer
are controlled boundaries with ordered event checks. Every unexecuted required
Host member aborts if unexpectedly called. The comparison is not a renderer,
full common motion transition or full transfer-engine test.

The `declared_phase_capacity` receipt field describes the composed base table
(Sheik 96, Zelda 72); it is not an execution count for earlier accepted slices.
This matrix executes the 16 newly authored Transform phases per member. Prior
component receipts retain their own source and boundary scopes.

## Policy and field mapping

| Original source decision | Authored policy |
| --- | --- |
| Sheik Lw361–364 / Zelda Lw355–358 | Four states, each animation/IASA/physics/collision; explicit empty IASA |
| `ftSk_SpecialLw_Enter` / `ftZd_SpecialLw_Enter`, air counterparts | Motion then animation, command0 reset, self/ground velocity division, world-origin wind, start accessory |
| Original Transform finish | Destination `on form_activated`, ground/air finish state and imported frame; no animation restart |
| Original transfer accessory | Clear accessory, `forms.activate_partner()` with no retail finish, return without dormant-source writes |
| Original effect accessors | Named raw part and effect parameters, once-only active flag, hitlag effect pause, accessory clear |
| Sheik imported floats96–112 / Zelda112–128 | Horizontal/vertical divisors, gravity, terminal speed and finish frame |
| Common constructor pointer zeroing | Load-only owned Needle/Chain handles use -1, count0; preserved across alternate activation |
| Raw pending action `cpu.xF8_b5` | Owned `cpu.pending_action`, run named script38, clear and return handled |
| `ftCo_800A949C` Zelda recovery | Ordered ordinary CPU commands; distance30, stick127, B40 and wait1 are named parameters |

Transform conversion uses the original `0x0c4c508e` motion flags. The fixture
normalizes only the expected stock finish callback argument versus authored
NULL in the controlled transfer event. Actual transfer ordering, relationship
preconditions and retained item safety have their separate production-source
fixture in `tests/native/partner_transfer/`.

## Shared CPU continuation and travel receipts

```powershell
python tests/native/cpu_callback_lifetime_source_test.py `
  --snapshot .local/baseline68 `
  --source .local/baseline68/sdk/src/melee/ft/ftcpuattack.c `
  --common .local/baseline68/sdk/src/melee/ft/kinds/ftCommon/ftCo_0A01.c `
  --output A:/FreakMelee-test-archives/cpu-pending68-root

python tests/native/paired_form_composition/travel_compare.py `
  --snapshot .local/baseline68 `
  --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso" `
  --output A:/FreakMelee-test-archives/ness-yoshi-travel68-root
```

The CPU source fixture accepts 1,550 assertions/141 cases. It executes the full
actual `ftCo_800ABA34` plus the exact startup prefix of `ftCo_800ACD5C` through
the pending decision, before its separate charge-decision branch. It tests raw
fallback, authored handled/unhandled/absent hook, priority and generation/kind/
Definition mutation. The ACD5C fragment is explicitly not a whole-function
claim. The printed 32 protected/reused cases describe its preexisting baseline
matrix; new pending cases add eight retirement/recycle and eight kind/Definition
cases within the 141 total. The rejected old source still faults at inaccessible
userdata, so the fixture does not silently waive the lifetime failure.

`travel_compare.py` normally clones/imports the **shipped** Ness and Yoshi
templates and compiles generated modules directly against the frozen SDK.
Ness accepts 116,585 checks/2,007 original decisions and 25,920 candidate pairs.
The 624 preexisting single rounded probability-threshold differences retain
their measured maximum `3.7252902984619141e-08`; queue/RNG effects are compared.
Yoshi accepts 9,297 exact original-C comparisons. The authored handled travel
callbacks increment `cpu.travel_frames` themselves. The shared handled branch
does not increment implicitly. Original Zelda Din handling never increments
that counter, so no adjustment is added to Zelda.

## Remaining closure gates

* Zelda `ftCo_800B9504` reads the live Din **primary JObj translation**, not
  `Item.pos`. A general validated article-reference model-translation Vec3
  query is still required. A cached script coordinate would not prove this
  original observation.
* The same body calls `ftCo_800A4E8C(fp, &position)`: nearest eligible opponent
  to that explicit projectile position, not the CPU's current target. The
  actual shared query excludes self, `inlineD0`, allies, `ftCo_800A1C44` and
  `inlineD1`, traverses the fighter list, and keeps the first equal-distance
  candidate. A generic synchronous query must preserve that eligibility/order
  and return a full creation ID without retaining a Fighter pointer.
* Until those observations exist, Zelda's authored attack/travel decision for
  Din states344–348 is not complete. No stock Din CPU helper is delegated.
* Atomic two-member registration, separate module/FMP/state/asset ownership,
  costume correspondence and normal family clone/import remain required.
  Alternate-form dormant members and simultaneously active companions have
  different semantics. Compilation here does not prove package switching.
* Load/reset, visibility, item/knockback presentation and dynamic resource
  damage/death policies are assembled from accepted components, but this new
  matrix does not re-accept all those runtime paths. Independent activation
  requires their final census and family metadata validation.
* Zelda's debug-ROM dress toggle is a deferred diagnostic path, not a versus
  gameplay decision silently delegated by this candidate.

No private-union reads, frame-count workaround or source-kind decision shortcut
is introduced. This checkpoint supplies complete authored Transform behavior
and identifies the remaining whole-pair boundaries rather than claiming full
Zelda/Sheik independence.
