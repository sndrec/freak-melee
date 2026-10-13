# Zelda Side-B and teleport composition (SDK58)

Unregistered source-only composition. The accepted Zelda57 candidate is read
unchanged and checked against SHA256
`e1830780e0438e2ee5f5c3e8a2ebc359996f35b5d5cf6509fb55327f29f5cf7a`.
`travel.fms` contributes owned declarations, helper bodies and callbacks; the
fixture inserts those sections without changing the accepted source. No default,
clone base, shared engine, transformation or package presentation is modified.

Reproduce from the repository root:

```powershell
python tests/native/zelda_travel58_composition/run.py --root . --baseline .local/baseline58 --output .local/zelda-travel58-root-repro
```

The output directory must be new. The runner extracts retail PlZd.dat, constructs
the staged FMP, generates actual C with the frozen content tool, compiles a real
module DLL and the console comparison, then runs the console program. All
commands, source/DAT/SDK/compiler input hashes, generated artifact hashes and
counts are retained in result.json. The module callback table executes directly
in the console fixture; dynamic OS module loading is not tested.

Final worker receipt: `.local/zelda-travel58-verified/result.json`.
The coherent suite passes **10,993 original/generated pairs, 655,214 checks and
20 separate authored safety cases**. This consists of **10,172 new travel pairs**
and all **821 accepted Zelda57 pairs**. No game, global build or install is run.

## Translation and coverage

- Side-B343..348: two entries and every animation, interrupt, physics and collision
  callback. Four signed timers, release-button gating, raw89 construction point,
  offsets/facing, failed-allocation hand cue on raw76, jumps, completion reset,
  root rotation, retained-ID owner changes and ground/air flags retain stock order.
- Teleport349..354: two entries and all four phases per state. Startup divisors,
  wind/accessory, ground threshold equality versus strict air threshold, input
  direction, soft-platform gate, duration, intangible/invisible/pickup flags,
  zero-rate travel, ground movement, recovery momentum/clamp/freefall and ledge
  routing are authored. Collision age increments before landing permission;
  ceiling, left wall and right wall are independent ordered tests, using the
  changed velocity after any preceding recovery entry. A focused case asserts
  three recovery entries within one callback, plus each wall independently.
- Bound startup/end effects preserve the otherwise unused raw4 world-position
  read, conditional joint/position constructor, hitlag bindings and accessory
  clearing. Ground loss uses D60C (consume all jumps) where stock does; this is
  distinct from the controlled ordinary ground-to-air transition.
- Start/hold/release/end/death sequences run through actual bound cleanup and the
  accepted detach signal. A changed Din owner is cleared without entering the
  absent-item completion branch during that same callback.
- Retained IDs/source IDs exercise high bits. Integer timers include +/-16,777,217
  and INT32_MAX and are compared as Int. Input samples include exact minimum,
  neighboring float, magnitude one/clamping, both directions and tilted floor
  normals. Finite float results and ordered events compare exact bits.
- All12 interrupt phases remain real generated callbacks; table assertions
  verify phases343..354 are owned and transformation355..358 stays untouched.

## Parameters and retail facts

Independent big-endian DAT reads supply51 named parameters (34fighter and17article).
Six ordinary owned source constants supply raw cue/creation joints and wind
arguments. Thus the real generated module accepts57 parameter values. Inline
constants are clearly marked in provenance.json and are not represented as DAT
imports. Special attributes cover offsets16..108; source offsets/types and
serialized values are recorded independently. The original29 parameter component
is reused without replacement tuning.

The existing Zelda rig audit establishes118 physical parts; raw89/raw76 remain
literal stock traversal indices. Semantic Hip4 resolves to raw4 and TransN1 to
raw1 in this controlled retail rig. Actual imported rig/package acceptance is a
separate integration requirement, not established by this fixture.

No newly required generic API was identified: SDK58 full-width fighter.creation_id,
articles.ref(...).current_owner, named functions, generic state/physics/collision,
raw-part particles and wind suffice. Fighter decisions call no ftZd handler.
Only the *original reference side* contains mechanically extracted stock bodies.

## Real bodies and controlled boundaries

fixture.py freezes complete ftzeldaspecials.c and ftzeldaspecialhi.c from immutable
SDK58 with includes removed, plus accepted neutral/item/lifecycle bodies. Actual
Fall, Deaccel, DeaccelAir, FallBasic, ClampSelfVelX, facing/threshold, D60C,
ground-to-self movement and XY-angle implementations execute. Actual
native_article_refs.c and native_callback_lifetime.c are separately compiled.
Definition lookup is an explicit immutable one-package registry control.

Animation availability/motion publication/start/rate, common wait/fall/freefall
and landing entries, stage/cliff/platform probes, ordinary ground/air conversion,
ground-friction multiplier (0.8), particles/wind rendering and item allocation
are explicit dependency controls. The Din constructor boundary records its
requested position, facing, outcome and typed generic creation arguments; the
complete Item allocator, owner-center/model construction and successful
constructor-to-on_created scheduling are not claimed. The unchanged57 suite
separately compares real full Din/Explosion initializer decisions. No surrogate
fighter tick or alternate collision solver is used.

14generated source/initializer invalidation cases include the accepted6 plus
four travel entries with removal/recycling. Six additional retained-query safety
cases cover absent/poisoned item registry, changed item generation, wrong kind,
wrong logic, retiring article and stale/poisoned owner. These are authored safety
contracts, not stock valid-pointer comparisons. They retain same-tick owner-clear
ordering. The original unsafe body is never run on poisoned data.

Normal registered Zelda clone/import/package pipeline, whole fighter independence,
CPU, debug dress toggle, transformation355..358 and paired CSS/package coupling
remain pending. This receipt does not activate any partial override and does not
claim gameplay or installed-package parity.
