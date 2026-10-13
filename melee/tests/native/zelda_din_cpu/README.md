# Zelda Din CPU steering, unregistered source component

This namespace adds both `cpu_attack_decision` and `cpu_travel_decision` to a
new assembled Zelda source output. Accepted paired SDK68 components remain
unchanged. The script owns projectile handle, motion range, release radius,
target height, stick magnitudes and wait duration. It uses the same-author
article's **primary model local translation**, then the nearest eligible
fighter's **current position**. The CPU's existing planner target is deliberately
different in the fixture. Neither query is substituted with a camera bone,
world transform or Item position.

The unchanged complete original `ftCo_800B9504`, original nearest body and
eligibility helpers come from frozen SDK68. The actual current same-author refs
and callback-lifetime units execute, alongside complete extracted shared nearest
and target-position bodies. Queue writers are unchanged original functions;
comparisons include their exact bytes, ordering, neutral stick writes, wait,
Done, cleared target and previous behavior. Zelda's handled travel decision
does **not** increment travel_frames; Ness/Yoshi's separate policies do.

```powershell
python tests/native/zelda_din_cpu/run.py --snapshot .local/baseline70 --reference .local/baseline68 --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso" --output A:/FreakMelee-test-archives/zelda-din-cpu70
```

The runner verifies both immutable manifests before/after, independently decodes
retail parameter imports, emits the complete assembled source, compiles its DLL
and actual callback fixture with the snapshot Clang/SDK (vcvars14.44), and
records commands, source hashes, parameter words and output hashes. It does not
use snapshot `script-build`, whose include paths are embedded at tool build.
Retail parameter staging is explicit: normal paired package registration/import
is still a separate open gate.

The matrix covers all five Din motions and their adjacent unhandled motions,
missing projectile/eligible enemy, 3D distance and list/tie/eligibility order,
high-bit and above-2^24 IDs, reflected current damage owner, exact-fitting command
buffers, editable tuning, and source removal/recycling after each query. Missing
or foreign article and a selected enemy disappearing before position read are
safe new boundaries, not comparisons against an original valid pointer.
Retired source userdata is PAGE_NOACCESS in destructive cases.

Original `15.0 + enemy.y - projectile.y` uses a double compound, whereas the
editable named Float parameters produce binary32 arithmetic. ULP-adjacent and
deliberately widened-stage large-cancellation probes identify any resulting
strict-release differences separately. Only that proven predicate delta is
permitted; all remaining command/state/observation order stays exact. Large
height probes are numeric contract controls, not claims that a retail arena
contains fighters at those heights. The receipt reports count and maximum error.

The frozen70 matrix has 3,254 policy pairs, 3 edited-tuning cases, 7 safe missing
identity cases and 12 source-lifetime cases. Its 20 ordinary threshold rows use
`enemy.y = 14.999999046325684`, `projectile.y = 15`: the original double distance
is below 15, while binary32 rounds it to 15. Error is exactly
`9.5367431640625e-7`. The 28 widened-stage cancellation rows contain 2 rows
at 2^24 and 26 at 2^28; their maximum error is 15. Both signs are probed, with
no negative-height branch differences in this matrix. Exact inputs, motion, callback, distances and
command lengths are retained in `result.threshold_cases`. These expected numeric
differences are not advertised as exact parity or ordinary arena trajectories.

This is console source acceptance only. Unexecuted component Hosts abort rather
than simulate their behavior. Shared game mode, stage bounds and special-state
observations are controlled. The full CPU scheduler, paired content registration,
all lifecycle closure, independent activation, rendering and gameplay remain
outside this gate. No defaults, accepted components or installed packages change.
