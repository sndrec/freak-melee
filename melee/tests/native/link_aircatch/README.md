# AirCatch360/361 source slice

This unregistered test namespace compares original Link/Young Link fighter
decisions with actual generated callbacks from the full tether drafts. It does
not activate a fighter, execute tether article solvers or claim gameplay parity.
The accepted held and deployed namespaces remain unchanged.

```powershell
python -B tests/native/link_aircatch/source_compare.py --snapshot .local/baseline32 --output .local/link-aircatch32-final
```

Use a fresh output directory. Optional `--link`, `--young` and `--iso` overrides
are recorded in the receipt. The runner verifies immutable SDK/compiler/tool
hashes before and after, performs normal clone/import of owned parameters, emits
the complete generated modules and compiles ordinary DLLs. Each console executable
includes its actual generated C directly, allowing exact authored helper entry
calls without fixture exports, wrappers or stock-handler delegates.

The frozen32 receipt is `.local/link-aircatch32-final/acceptance.json`. Each
fighter passed **243,738 assertions / 4,365 source phases**, maximum float
difference **0**. Combined: **487,476 assertions / 8,730 phases**. Coverage includes
256 input configurations, 16 hit-entry configurations, every original animation
timer from -1 through removal+2 with create-failure/obstruction/end-of-animation
and both facings at three scales, normal/fastfall gravity branches, hit velocity
copying, both collision callbacks and 16 ordered input/animation/physics/collision
timelines. Per-fighter event counts include 64 creates, 28 launches, 52 retractions,
80 removals, 1,540 freefall calls, 114 fastfalls, 306 falls and four explicit
landings. Both hit bounce and basic landing branches execute. These counts are
fixture coverage, not real gameplay frame counts.

The original bodies are extracted from frozen
`ft/kinds/ftCommon/ftCo_AirCatch.c`: `ftCo_800C3B10`, `ftCo_800C3BE8`,
`ftCo_800C3CC0`, all six AirCatch/AirCatchHit animation/interrupt/physics
callbacks and `ftCo_AirCatch_Coll`. The source freefall forwarding body
`ftCo_800968C8` is extracted from `ftCo_FallSpecial.c`; the entire shared
`ftCo_AirCatchHit_Coll` body comes from `ft_081B.c`. The latter is also the actual
generic Host operation called by the generated state361 collision callback.
The production callback-lifetime source is linked, using a controlled live list
with high-bit fighter generations and article handles.

Two source-backed corrections appear only in these new test candidates:

- `Tether_EnterAir` uses `state.enter_at(360, 0, 0)`. The earlier draft's
  `state.enter(360)` generated start frame1; the original uses frame0.
- Failed construction calls freefall and then continues the animation-finished
  check. The earlier draft returned immediately. The original defined branch
  can make a second freefall call when construction failure and animation end
  coincide. The actual extracted forwarding body and retained-source transition
  boundary exercise that call order. Generated lifetime guards still stop
  continuation when a source is invalidated; this fixture does not simulate
  removal during a transition, which has separate generic lifetime acceptance.

The controlled boundaries are motion/freefall/landing transitions, root article
creation/reference lookup/signals/removal, thumb semantic resolution and world
position/matrix setup, common fastfall/fall/drift/clamp operations, ECB responses
and the grounded-hit auxiliary update. Launch signals model the destination
launch-vector effect for trace comparison; they do not execute an article solver.
Creation's post-pickup signal is explicitly within the constructor boundary.
Article callback installation is compared by slot presence, with exact generated
function identities used for its own removal/unbinding. Unused required Host
operations trap. Active Definition lookup returns no authored hook on the stock
side; the actual generated input hook executes on the script side. The engine
hook dispatcher itself has separate accepted coverage.

Imported timing/sound/launch parameters come from the owned FMP files. Common
attributes, motion/ECB state, creation failures and stage responses are controlled
fixture inputs. Source tuning pointers are fixture-owned and used only by the
original bodies; generated callbacks read owned parameters and ordinary state.
The source's null-tether dereference after an inconsistent successful-creation
timeline is excluded. Full resource construction, spool animation, article
states1/2/3/6/7/8, reel/capture decisions, ground Catch/CatchPull/CatchWait and
whole fighter damage/death composition remain unaccepted. No new generic API
was required for this bounded fighter slice. No game or global build was run.
