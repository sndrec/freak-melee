# Peach Float and Side-B source composition

Local source comparison passes against frozen SDK50. This namespace preserves
accepted full composition48 unchanged and adds Float/FloatFall/FloatAttack's
32 phases plus Side-B's 24 phases. The inherited candidate has 90 authored
phase declarations, 18 ordinary fighter fields (60 payload / 69 allocated bytes),
43 helpers, 124 owned parameters and all five authored article tables. No pool
capacity increased. State356 stays null. The 24 Parasol travel/common phases
361..364 and369..370 remain inherited; independence and gameplay are pending.

`float.fms` and `sideb.fms` contain readable policies. `assemble.py` combines them
with the immutable accepted input identified in `components.json`. Normal
clone-stock Peach / import-parameters / script-c / matching frozen Clang/SDK
DLL compilation is exercised. No live-header script-build, installed package,
default replacement or stock fighter callback delegation is used.

## Reproduction

```powershell
python tests/native/peach_float_sideb_composition/assemble.py --sdk .local/baseline50/sdk
python tests/native/peach_float_sideb_composition/run.py --root . --baseline .local/baseline50 --output .local/peach-float-sideb50-root-repro
```

Use a fresh output directory. The runner retains logs, generated source,
retail DAT, imported FMP, DLL, console executable, extracted bodies, source
snapshot and SHA receipt. Final handoff:
`.local/peach-float-sideb50-final-v2/result.json`. Root review is separate.

The matrix executes 16,424 original/generated pairs, 159,541 checks and 17
authored safety cases. A further 256 cases check each manual/automatic entry
hook against explicit conditions and grounded availability reset; these are
not counted as original-body pairs or common-caller ordering evidence. Root
owns the separate common engine policy fixture.

## Exact source policy

The real forward.h/motion table maps Side-B as354 ground start,355 ground end,
356 null,357 air start,358 air end,359 rebound end,360 air jump. Draft numeric
mistakes were caught by compiled original motion rows and corrected. Named
transition masks remain Int.

Float entry omits animation.start, optionally resets its timer, consumes
availability, zeroes Y and uses selector0 async attached particle creation.
Direction uses common.x78 and clip18/19 lengths already owned by accepted load.
Float attack retains one gun-selector call or two ordinary attack-selector
calls, held use-category filtering, reverse cue consumption, release latch,
and retained timer when returning to Float. FloatFall physics is drift followed
by adjusted animation Y. Landing preserves strict velocity.y > threshold
Wait/basic-Landing selection. Own FloatAttack344..348 follows LandingAir's
nonstandard-motion basic-landing fallback, including command0 set.

Side-B binds reset before motion entry and preserves command/velocity/animation
delta clearing and smash-age/thrown-stat order. AccelToVel uses ordinary ground
fields; the LStick-scaled ground_accel method differs. Its negated comparison
is explicitly `not (...)` to retain C operator precedence. Wall/ceiling tests
continue after transitions. Explosion creation at raw joint4 precedes rebound
velocity publication. Both allocation outcomes and facing callbacks are tested.

## Real bodies and controlled boundaries

The extractor preserves complete original Float, FloatFall, FloatAttack, Side-B,
Explode and earlier pluck/smash files plus identified shared bodies. New
comparisons execute all56 added phase slots, entries/reset, selection, landing,
facing and hurtbox callbacks. Earlier pluck/counter matrices are reused evidence,
not counted as rerun coverage.

Actual shared bodies execute C-stick edges/attack selection, attack entry,
LandingAir fallback, outer82C74 landing, AccelToVel, deceleration, gravity,
grounded movement, signed ABS facing, article creator/constructor and Explode
initialization/lifetime/scaling. The real native item-owner backend, callback
generation registry and contact rebound dispatcher are linked. Original union
variables are projected against named script state; motion flags, ordered call
records, position/velocity/acceleration/commands, selector counts, facing,
interruption/effects/stat fields and callback presence remain checked. No
diagnostic digest fields are masked.

Animation sampling, drift/animation Y, input-entry outcomes, floor probes,
ground friction, joint positions, allocation/attachment, rendering, sound and
debug are controlled. 831CC's internal contact/wall-jump/ledge solver is
controlled; named landing continuation and common predicate identity are checked.
Selector angle boundaries use the exact stock atan2f formula with call counting.
Unexecuted inherited Host methods fail if called. The motion boundary executes
the actual state-changed callback point; whole Fighter_ChangeMotionState engine
coverage is not claimed.

Safety cases separately check removed/recycled source or contact recipient,
changed kind/Definition/contact pointer, rejected recipient and null contact.
Changed identities stop rebound tails. Stock's null-contact dereference is
outside defined retail parity. Generation invalidation at entry stops later
particle/start/timer work. No gameplay or deterministic replay is claimed.

## Provenance and retained diagnostics

All124 parameters are imported through the real project flow;59 declarations
are independently checked against big-endian retail DAT words. Float duration
is150; Side-B age limit3, acceleration0.025 and target speed0.7. Clip18/19 lengths
are controlled23/31 here and matched in named state; accepted load sampling is
separate evidence.

Historical import-only48 receipt:
`.local/peach-float-sideb-tuning48/result.json`. Earlier50 failures remain in
`.local/peach-float-sideb50*/`: missing adjacent FMP, unsupported draft inline/
deep syntax, motion mapping (v5/comparison/run1.log), and unparenthesized
acceleration (v6/comparison/run.log). These were draft/preparation errors, not
accepted stock deviations. v8 passed the expanded matrix before final freeze.
