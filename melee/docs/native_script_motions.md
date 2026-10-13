# Interpreted fighter motion scheduling

The complete compiler accepts an explicit `NativeScriptMoveLayout`, copied at
load from the installed owned fighter definition. It contains common-state
bounds, the extended table range, four pairs of named special states and their
transition flags. Unknown/absent targets, competing aliases and layout mismatches
fail before a program is activated. Extended layouts must match the independently
owned definition actually installed for the chosen kind; raw guessed state IDs
are not sufficient.

## Source forms

```text
move Projectile:
    on special_n:
        pass
    on special_air_n:
        pass
    on ground_animation:
        pass
    on air_animation:
        pass
    on ground_physics:
        physics.ground_friction()
    on neutral_special_cue:
        pass

action Idle:
    on state_14_accessory:
        state.counter = state.counter + 1
```

These are scheduling examples, not a complete projectile implementation. Entry
handlers must author their behavior. Body operations and metadata must all be
supported; unsupported ordinary behavior never calls a stock special as a fallback.

`state_N_animation`, `state_N_interrupt`, `state_N_physics`,
`state_N_collision` and `state_N_accessory` target actual absolute state IDs.
Named ground/air phases target the corresponding canonical special states from
the layout. `neutral_special_cue` / `accessory` bind both canonical states and
require both to exist. Additional states can declare explicit per-state accessory
callbacks. One named move cannot declare two different special entry families.

`NeutralSpecial`, `SideSpecial`, `UpSpecial`, `DownSpecial` and their explicit
`Air...` forms resolve to source-positioned I32 literals. Air callbacks resolve
the unprefixed form to that air state; ordinary helpers use the ground form.
These values contain no native pointers. This does not by itself implement
`state.enter`, plane-transition helpers or all existing move/body operations;
those remaining binding contracts still reject unsupported calls.

## Actual scheduler

The program keeps a sorted fixed table of motion/phase function IDs. The actual
`Fighter_ChangeMotionState` callback-assignment site selects those IDs and clears
only native pointer slots with authored overrides. Invocation happens in the
existing animation, input, physics, collision and accessory paths. Shared native
assignments supersede an instance override, matching the existing callback
ownership rules. No per-authored C function, trampoline or native body fallback
is created. The original motion animation/subaction data and stage ordering stay
in place. All eight actual special input routes now dispatch a selected source
entry, retaining input priority, facing and ground-velocity preparation. Retirement
stops before accessing the old fighter's fields.

Absent hooks/phases retain native behavior in the offline diagnostic during this
partial migration. This is not full independent fighter or Workshop acceptance.
Production source-only package loading must supply its validated owned layout;
`--script-kind/--script-source` startup diagnostics expose verified common-state
phases only, since owned package definitions are not loaded at that early point.

## Evidence

The combined actual actor fixture passed 156 assertions. It compiles direct FMS,
checks owned-definition agreement, named ground/air literals, duplicate actual
phase targets, all eight extracted original special input routes, native override,
source retirement and selected phase execution. Native definition/asset and
presentation boundaries are controlled. Fixture:
`tests/native/script_fighter/`.

`live_motion.fms` adds only an idle accessory counter to the prior diagnostic.
Four Popos completed 2,000 ticks/late updates; their per-state call counts were
92,154,54,101. All earlier event counters matched the prior run, and all eight-Int
state hashes matched independent reconstructions. Census errors were zero.
Receipt: `A:/FreakMelee-test-archives/live-interpreted-motions-no-scratch/receipt.json`.
The same source reproduced all 64 broad-oracle rollback endpoints with drawing
retained. Mean 168.31 ms, all 64 deadline misses; this is state/scheduler correctness,
not a performance acceptance claim. Receipt:
`A:/FreakMelee-test-archives/live-interpreted-motion-rollback/receipt.json`.
