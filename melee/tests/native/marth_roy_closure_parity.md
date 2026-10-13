# Marth/Roy closure runtime suite

`marth_roy_closure_parity.py` creates fresh owned packages and runs the shared
stock/script parity comparator with matching `--tool` and `--exe`. Its default
acceptance mode requires `behavior independent` in the shipped templates.
`--candidate` explicitly permits adding that declaration to isolated imported
source while the templates remain unpromoted. It never changes the examples.

Run from the repository root after the coordinated runtime slot is granted:

```powershell
python -B tests/native/marth_roy_closure_parity.py `
  --iso '..\Super Smash Bros. Melee (USA) (v1.02).iso' `
  --tool .local/baseline18/bin/native_content_tool.exe `
  --exe .local/baseline18/bin/melee_native.exe `
  --out-dir .local/marth-roy-closure-candidate18-runtime `
  --candidate --include-lifecycle
```

Use `--plan-only` to copy/hash inputs without invoking a tool or game, or
`--prepare-only` to compile, bundle, verify and accept packages without games.
The inferred SDK/compiler paths are the snapshot's `sdk` and
`compiler/clang.exe`; explicit `--sdk`/`--clang` overrides are available.
Compilation uses the tool's `script-c` output and those frozen headers directly.
The tool's `script-build` command embeds live include paths and is unsuitable
for this snapshot test. Package verification supplies the actual copied base;
the standalone `module-probe` command passes a null base and cannot validate
these generated fighter modules.

The default focused suite selects 24 cases per fighter:

| Coverage | Cases per fighter | Required evidence |
| --- | ---: | --- |
| Neutral special | 3 | Full/partial/short phases and intended entry facing |
| Up special | 3 | Ground/air phases and actual contact damage |
| Side special | 8 | All high/neutral/low phases, ground/air, rejected early input, attack continuation |
| Counter | 3 | Ground/air laser response and real melee damage |
| Common behavior | 4 | Marth jab restart/Roy single jab, shield, powershield, actual global wind |
| Cross-fighter contact | 1 | Peach body contact during the counter window |
| Owned CPU policy | 2 | Level9 active inputs and opponent damage on FD/Battlefield |

Exact recordings come from the five accepted `sword-*-acceptance.json` receipts
named in the runner. Fighter-specific corrected recordings are selected from
their receipt log names; the latest owned CPU receipt supersedes older CPU
evidence. `--suite all-accepted` selects all 45 accepted recordings per fighter.
Repeated `--fighter` and `--case` selectors support a bounded rerun.

`--include-lifecycle` adds two new normal-input trajectories per fighter; these
are coverage experiments until actual traces meet their gates. The landing
case requires fresh/consumed/fresh/consumed air boosts, no landing within either
same-air pair, and a common landing between pairs. The death case consumes an
offstage boost, requires real death then rebirth, and requests the next air
special directly from RebirthWait before a landing can reset it. The original
`ftCo_RebirthWait_IASA` (`src/melee/ft/ft_0D4D.c`) checks air-special input directly.
No position, damage, motion or stock-loss hooks are injected. A failure to reach
these states is a trajectory coverage failure; it cannot count as acceptance
even if stock/script traces match. Results/demo construction remains separate
source census evidence until a real results scene is exercised.

Games use the shared runner's hidden, muted, fast defaults with drawing enabled.
The ordinary fighter/action/hitbox/hurtbox/RNG comparison stays enabled; extra
coverage gates run on both stock and script traces. `plan.json` records input
hashes, source receipts and expectations; `packages.json` records owned source,
parameter, CPU and DLL hashes; `commands.json` contains exact invocation arrays;
`results.json` records each comparison and its coverage. Prepared commands still
need the runner's `coverage()` checks after the shared comparator succeeds.

Console preparation on 2026-10-02 succeeded for both baseline18 candidates at
`.local/marth-roy-closure-candidate18-prepared2`, producing 52 commands and no
game launches. `--help` succeeded. A separate read-only check applied the new
focused gates to 96 historical stock/script logs and passed all 96; this is
recorded in `.local/marth-roy-closure-runtime-plan/historical_gates.json` and is
not new runtime acceptance. The initial preparation failure from null-base
`module-probe` remains preserved at
`.local/marth-roy-closure-candidate18-prepared`.

## Independent runtime acceptance - 2026-10-02

Root's baseline18 batch accepts all 48 focused comparisons plus four corrected
lifecycle comparisons: 30,000 controller samples and 60,000 fighter states.
The original lifecycle trials all matched stock but three failed coverage:
one requested the offstage boost while grounded, and Roy's landing trial
landed during side-special rather than common Landing. Those trials do not
count toward acceptance. Current inputs hold a full jump, delay the double
jump, and request the offstage boost at frame480, then post-rebirth boosts at
800/831. Every lifecycle gate now passes on stock and independent traces.

Both shipped templates were then promoted. Eight final shipped-package pairs
(melee counter, level9 FD CPU, landing and death per fighter) pass, comparing
8,800 samples and 17,600 states. Generated behavior C matches the accepted
candidate exactly after removing only the exported source-hash value;
parameter and CPU files are byte-identical. The final shipped-source console
batch passes all 3,094 comparisons as well.

Evidence: `.local/marth-roy-independent-acceptance18.json`, with full matrix,
final package subset, canonical/raw generated hashes, parameters and CPU hashes.
The source checkpoint is `.local/marth-roy-independent-source18/acceptance.json`.
Results/demo scene and exact pixels remain outside this acceptance.
