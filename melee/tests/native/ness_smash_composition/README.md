# Unregistered Ness smash composition

`candidate.fms` is an unregistered source candidate, not a default template or an independent Ness
module. It composes all seven smash states (341–347), their 28 gameplay phase
callbacks, three complete entries, charge/release transitions, Bat construction
and animations, and YoYo construction/animation/entries/four physics solvers.
It deliberately remains separate from `examples/ness_wip.fms` and the accepted
Thunder candidates.

`compose.py` assembles the candidate from this namespace's `fighter.fms` and
three accepted immutable component inputs. `component_inputs.json` records the
input hashes. No accepted input is edited. The resulting source is under 48 KiB;
55 named defaults plus 31 fighter/article imports produce 86 owned parameters,
within the 256-value capacity. Article-qualified `attrs.It_Kind_Ness_Yoyo.*` reads are ordinary owned
immutable parameters after normal import, never runtime DAT or union reads.

## Available compiler foundation

Frozen43 supplied the entry/phase contracts used by the original source candidate;
the cleanup correction below additionally requires frozen44 or a later reviewed
snapshot with the generic-enable bookkeeping fix:

- Complete `on forward_smash_enter`, `on up_smash_enter`, and
  `on down_smash_enter` replacement hooks. The existing post-entry forward
  presentation hook cannot implement these entries.
- Float-local outputs for the existing `bones.world_position`; existing state
  outputs remain useful, but no persistent coordinate scratch is required here.
- `vectors.rotate`, `controls.interrupt_allowed` (Bool bitfield),
  `input.attack_held`, and `damage.song_immune`.
- Fighter floor/ceiling contact-mask constants alongside wall constants.

Common charge storage, fixed-box sweep, raw hitbox state, victim clearing,
standard enabling, callback predicates, article signal/lifetime protection and
resource operations are already generic engine contracts. The candidate uses
`hitbox.clear_victims` followed by `hitbox.set_enabled`; it does not use the
removed misleading detection-reset operation.

The original full imported fighter/article module compiled directly with frozen43 Clang.
Do not run snapshot `script-build`, whose embedded includes can point at the live
tree; this fixture uses explicitly hashed frozen tool/SDK inputs instead.

## Fighter phase mapping

| Motion | Original prefix | Authored animation / interrupt / physics / collision |
| --- | --- | --- |
| 341 | `ftNs_AttackS4` | BatAnimation / BatInterrupt / BatPhysics / standard ground collision |
| 342 | `ftNs_AttackHi4` | YoyoAnimation(up,start) / YoyoInterrupt(start) / YoyoPhysics(start,up) / YoyoCollision |
| 343 | `ftNs_AttackHi4Charge` | YoyoChargeAnimation(up) / YoyoChargeInterrupt(up) / ground friction / YoyoCollision |
| 344 | `ftNs_AttackHi4Release` | YoyoAnimation(up,release) / YoyoInterrupt(release) / YoyoPhysics(release,up) / YoyoCollision |
| 345 | `ftNs_AttackLw4` | YoyoAnimation(down,start) / YoyoInterrupt(start) / YoyoPhysics(start,down) / YoyoCollision |
| 346 | `ftNs_AttackLw4Charge` | YoyoChargeAnimation(down) / YoyoChargeInterrupt(down) / ground friction / YoyoCollision |
| 347 | `ftNs_AttackLw4Release` | YoyoAnimation(down,release) / YoyoInterrupt(release) / YoyoPhysics(release,down) / YoyoCollision |

The raw attachment joint is 42, slope-corrected endpoint joint is 61, and the
original direct `parts[FtPart_XRotN]` ordinal is 2. These are editable authored
parameters; numeric direct ordinals are distinct from semantic remapping.

The ordinary state fields replace the private smash union: frame counter, rehit
timer, charge-disable and slope-correction flags. Hit XYZ and spin rate remain
owned persistent fields shared with the authored article. Common charge frames
are advanced by the ordinary engine update; no duplicate authored increment is
added. The independent authored YoYo frame counter is still needed by its source
animation decisions and is reset to 0 on charge and 14 on release.

## Ordering and composition boundaries

YoYo creation samples raw joint 42 into stack coordinates without overwriting
the remembered hit position. It publishes the auxiliary item, preserves existing
damage/death callbacks when non-NULL, and binds hitlag callbacks even after
construction failure. Bat creation overwrites damage/death2 callbacks only after
success, as stock does. All signal and constructor results remain full-width
integer handles with the `-1` sentinel.

Charge sets the authored frame counter to 0, releases the article, selects common
mode4, saves the current animation rate (already paused), then writes frames,
limit, shake index/period and color before color/rumble/sound presentation.
Release changes motion first, initializes animation, restores held YoYo state,
sets spin/sound, applies u32-truncated damage, and rebinds immunity/deal/accessory.
Victim history is cleared before enabling; retained victim timers are not cleared.

The released-vector signal carries only fighter facing. The article reads its
owned release XY and performs the original ordered assignment/absolute-X/item
facing transform before changing state. This avoids a multi-signal coordinate
protocol; the native third argument to `it_802BFE5C` is unused.

The collision probe retains two short-circuited sweeps and authored flag
remapping. The first checks the fighter ECB midpoint to remembered hit position;
only with no floor/wall/ceiling contact does the vertical second probe test floor.
Slope rotation uses the actual generic engine vector rotation, not trigonometric
substitution. Release blending remains multiplication/addition even when the
fraction clamps to 1; it is distinct from the final direct-copy branch.

`SmashResourceDamage` composes only the two resources present in this namespace:
YoYo, then Bat. Final whole-Ness composition must preserve the accepted complete
YoYo → Flash → Thunder → Bat order. This isolated load callback initializes test
handles and is not a replacement for complete Ness OnLoad/OnDeath. It cannot be
promoted while Flash/Thunder/jump/CPU/lifecycle closure is incomplete.

## Accepted source comparison

`run.py` compiles unchanged `ftnessattacks4.c`, `ftnessattackhi4.c`,
`ftnessattacklw4.c`, and `ft_0DF0.c` from the pinned frozen source root. It compares
all 28 generated phase callbacks and complete entries with actual source calls. Controlled
transition, animation, sound/color/rumble and article boundaries must record
ordering/arguments and apply the relevant field mutations. These boundaries
cannot be presented as actual resource rendering or full stage geometry proof.

The original frozen43 matrix covers constructor failure/success, preserved callbacks, commands
0/1/2, spawn/rotate/retract/despawn frames, both A-held states, interrupt gates,
all live hitbox states, retained victim timers, release truncation, edited named
parameters, actual common charge mode4 updates before animation, both slope/blend
branches, exact callback bindings and auxiliary publication. Stage checks compare
full previous/current XYZ, six box coordinates, category, ordering, all four
contact-mask combinations, and short circuit behavior. The stage sweep implementation
is controlled; actual stage geometry needs separate acceptance.

The historical independently reproduced `.local/ness-smash-composition43-root/receipt.json` records **207,045
checks, zero failures** and **6,528 unchanged-source pairs**:

- 3,584 phase pairs: all seven states and four callbacks, 64 fixtures, two tuning sets.
- 896 complete forward/up/down, charge and release entry pairs.
- 1,792 resource/charge/accessory/victim-history and obstruction helper pairs.
- 256 actual common charge updates followed by the original/generated animation phase.

The second tuning set changes imported charge duration, damage multiplier, rehit
rate, article spin rates, and rotate/retract frames without recompiling. The tested
float field/event values agree without a relaxed numeric bound. This is a bounded
source result, not a claim about all floating-point inputs or gameplay pixels.

Ten authored-only lifetime cases separately remove or recycle the fighter at a
motion transition, Bat/YoYo constructor, removal signal, or charge-release signal.
Full generation validation stops subsequent result/state/callback/presentation
writes. Healthy original bodies remain unchanged; the fixture does not execute
their undefined stale-source continuations.

The full article candidate is compiled, while this comparison executes its
fighter slice through controlled constructor/signal boundaries. It composes the
accepted immutable resource components; it does not re-prove their entire runtime
or resource geometry here. Bat constructor normalization, article release and
model animation remain the corresponding accepted component fixtures' scope.

## Delayed-hitbox cleanup correction

The original `ftNs_AttackHi4_YoyoCheckTimedRehit` decrements the rehit timer and,
at zero, calls `lbColl_80008440` to clear victims followed by `lbColl_80008434` to
enable the hitbox. Neither call sets fighter `x2219_b3`. The ordinary
`Fighter_ChangeMotionState` cleanup block checks that flag before calling
`ftColl_8007AFF8`, so a delayed hitbox with no prior cleanup latch can remain
enabled after a smash ends or is interrupted. The same unlatched hitbox can
survive a later airborne transition and landing or ledge transition.

The generic `hitbox.set_enabled(index,true)` correction preserves activation and
also records ordinary motion-cleanup intent for valid indices. It needs no
YoYo-specific API or private-union decision. The script comments explain why
victim clearing and enabling remain separate ordered operations. Existing source
pairs explicitly assert this intended flag difference only when the original
delayed refresh occurred; remaining fields, ordering and callbacks stay checked.

`cleanup_cases.h` adds 30 bounded regression cases. Its common transition fragment
is mechanically copied verbatim from `Fighter_ChangeMotionState`, and its clear
operations are actual extracted `ftColl_8007AFF8`/`lbColl_80008428` bodies. The
fixture exercises timer/command controls, ordinary up/down completion and
down-smash interruption, both previously latched and unlatched hitboxes, an
airborne transition retaining `Ft_MF_SkipHit`, then landing/ledge cleanup. Explicit
SkipHit exits retain hitboxes and the cleanup latch; ordinary exits consume the
latch and disable them. The same cases also enter an ordinary airborne attack,
activate a new hitbox through actual `ftColl_8007B064`, and verify its existing
cleanup behavior on landing/ledge. Damage and per-victim bookkeeping remain intact.

This is a source cleanup regression with controlled Wait/IASA transition selection,
not the complete `Fighter_ChangeMotionState`, actual input arbitration, rendering,
or a game reproduction of the visual glitch. The unchanged original unlatched
branch is retained as the negative control, rather than being rewritten to match
the corrected script.

Frozen44 accepted `.local/ness-smash-composition44-final/receipt.json` records
**214,186 checks, zero failures**, the unchanged **6,528 source pairs**, ten
source-lifetime cases, and **30 cleanup regression cases**. The original-pair
comparison counts **180 explicitly classified cleanup-latch differences**; its
other field/event/order assertions remain active. This intentional engine
bookkeeping correction is not reported as exact stock parity.

The namespace is still unregistered. The shipped Flash-only Ness draft inherits
stock YoYo handlers, whose direct native enable calls are unchanged. It receives
this scripted correction only when the authored smash composition is integrated;
full Ness lifecycle/default activation remains a separate gate.

## Reproduce

From the repository directory, using a fresh output path:

```powershell
python tests/native/ness_smash_composition/run.py --snapshot .local/baseline44 --output .local/ness-smash-composition44-rerun --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso'
```

The runner verifies the snapshot, unchanged original source and accepted component
hashes, creates a fresh normal Ness asset project, imports the candidate to an owned
FMP, generates full and fighter-only C, compiles the full candidate and comparison
with explicitly selected MSVC14.44 environment/frozen Clang, then runs only a console
comparison executable. The receipt contains input/module/import hashes, scope and
result gates. No package or binary output belongs in this tracked namespace.
