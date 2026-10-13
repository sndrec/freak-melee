# Unregistered Game & Watch normal entries, SDK59

This namespace composes accepted Chef52, specials53, contracts56 and normals57
inputs without modifying them. It adds all normal entries and 48 phase policies,
and resolves Manhole's previously pending first-child world-position wind branch.
It does not register a base, activate a default template or run a game.

```powershell
python tests/native/gamewatch_normal_entries59/run.py `
  --snapshot .local/baseline59 --output .local/gamewatch-normal-entries59-root
```

The runner verifies the complete immutable snapshot and original source manifest,
generates actual C with its content tool, then compiles with its Clang and SDK.
It never uses `script-build` and its embedded live header paths. Compilation uses
`vcvars_ver=14.44`; all binaries/logs/packages remain outside tracked inputs.

The matrix passes 4,851,141 checks over 49,960 source pairs:

| Group | Pairs | Coverage |
|---|---:|---|
| Normal phases | 20,666 | All four callbacks of motions 341..352, animation completion/throw cues, interrupt flags, rapid-jab state, collision outcomes and five prop masks; every DownTilt input stop and disabled-input control. |
| Entries | 4,618 | Six grounded/rapid entries, five aerial selections in both facings, three landing entries across all 512 live prop masks and command0/nonzero. |
| Prop setup | 24,576 | Six accessory setup helpers, all 64 normal-prop masks, constructor success/failure, saved held item, facing and preexisting/NULL damage callbacks. |
| Manhole world wind | 100 | Motions 340..349, present/absent current owner, commands 0/1/2/-1/16777217; exact ordered reveal, one Vec3 read, wind tuple, removal and cleanup. |

The original GameWatch function bodies are extracted unchanged from the supplied
snapshot. Their hashes still match the accepted SDK57 manifest. The active SDK59
common jab startup, rapid startup/update, landing entry/rate and aerial/landing
animation bodies also execute directly. The receipt distinguishes those inputs
through their source paths and hashes; this is not a claim that shared API bodies
have never changed since SDK57.

`fighter.fms` contains the readable entry/phase definitions. Five IASA callbacks
have no GameWatch decisions and bind ordinary common motion policies. Every
remaining phase and entry is script-owned; no GameWatch handler is delegated.
`compose.py` inserts these definitions before code and resolves the two pending
Manhole operations only in the new output. Accepted files remain byte-identical.

The fixture reuses the accepted normals57 boundary definitions, reset helpers
and comparators as pinned source. Its previous test loops/main are excluded;
new grouped cases live in `matrix.h`. `boundaries.h` provides explicitly
controlled input, collision, motion, world transform and constructor boundaries.
All source and generated calls reach those same boundaries in their actual order.
Unknown Host operations trap with their names. The actual
`native_callback_lifetime.c` creation-ID registry executes for generated guards.
Full-width fighter/item IDs remain 0xFEDCBA98 and 0x80000010+kind.

In particular, Manhole queries model 0/path "0", the actual primary first child
returned by `HSD_JObjGetChild`, then reads one Vec3 through `lb_8000B1CC`.
The controlled world coordinates differ from Item.pos. The wind call receives
that Vec3 and owned 120/3/.1/1.0471976 parameters. Geometry evaluation itself is a
boundary here; the SDK59 backend/primary/auxiliary/missing-output contracts have
their separate engine/compiler fixtures. This matrix assumes the stock child
exists and does not assert stock behavior for a malformed missing child.

Checks compare ordered common calls, commands, motion/frame/rate, facing,
interrupt/jab/rapid fields, held item and saved handles, prop flags/removal and
exact accessory/hitlag/damage callback decisions. Source NULL handles map to
ordinary authored -1 IDs. Intentional empty phase bodies remain authored empty
callbacks. Shared camera is unchanged metadata and not tested here.

Preserved source details include rapid setup after consumed pickup, rapid Loop/
End motion transitions without animation startup, Torch repeated allocation,
the ordered twelve DownTilt interruption checks, command-gated aerial landing,
Up Air choosing Back lag, and cleanup after landing animation/collision changes
the motion. The prior nine-prop cleanup and unusual Turtle/Sparky landing checks
are reused unchanged. No unrelated bug fix or RNG deviation is introduced.

Actual prop allocation/attachment/paper initialization, palette/outline/frame-map
rendering, whole OnLoad metadata, foreign/reflected prop owners, complete input/
collision/motion solvers, and drawn gameplay remain unaccepted. The constructor
boundary validates the authored kind/XYZ/zero velocity/facing/owner-origin/group/
creation-value/collision inputs and original raw joint part, but does not invoke
an allocator or pretend to implement paper rendering. Full fighter independence
and default activation must wait for those remaining boundaries.
