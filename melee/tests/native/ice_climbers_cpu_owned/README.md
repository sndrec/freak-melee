# Authored follower CPU composition

This unregistered source fixture translates Nana's mode-6 decisions, ongoing
controller recording/application, destination selection and Blizzard entry into
ordinary fighter script. Popo's CPU capture choice and common pummel routing are
separate primary-member hooks. It never starts Melee or installs a package.

Reproduce from the repository root with a fresh output directory:

```powershell
python tests/native/ice_climbers_cpu_owned/run.py --snapshot .local/baseline72 --reference .local/baseline71 --output A:/FreakMelee-test-archives/ice-climbers-cpu72-reproduction
```

Both generated routes passed at
`A:/FreakMelee-test-archives/ice-climbers-cpu72-eleventh/receipt.json`:

| Evidence per route | Count |
| --- | ---: |
| Whole-phase original/generated pairs | 506,488 |
| Copy eligibility pairs | 551,300 |
| Start-copy threshold pairs | 504 |
| Assertions | 21,580,064 |
| Authored safety cases | 17 |
| Overlay grammar/context/type checks | 15 |
| Maximum measured float error | 0 |

The driver subsequently gained complete name-keyed body provenance, accepted
component hashes, an exact tracked-parameter check and structured result counts.
Those reporting changes do not change the compared C or script inputs. The
parent's fresh reproduction will provide the final receipt for this driver.

## Actual bodies and controlled boundaries

The reference is immutable SDK71. The runner extracts actual complete definitions
of `ftCo_800B101C`, `ftCo_800B0AF4`, `ftCo_800A7AAC`, `ftCo_800B0CA8`,
`ftCo_800B0E98`, `ftCo_CpuUpdateNanaFollow`, their local helper definitions and
the actual destination setter `ftCo_800A1F3C`. It also extracts the original
`ftCo_800B683C` and `ftPp_SpecialLw_Enter`. Definition-prefix matching prevents
extracting call sites. The receipt records every extracted body by exact name.

The current side extracts SDK72's complete `ftCo_800B683C`,
`ftCo_CpuPrepareCombatPlan`, `ftCo_CpuPrepareDestinationUpdate` and ten actual
retained-target getters. It links the production `native_cpu_history.c`.
Generated callbacks, helper DAGs, declared state metadata and external tuning
binding are actual compiler output for both action and motion routes.

Controlled engine boundaries are live registry/family-member lookup, lifetime
snapshot/alive checks, common enemy/item target planners, attack planning and
recovery destination children, destination stage adjustment, stage floor/island
and hazard queries, RNG samples, common CPU command programs, motion change and
animation start. Each records ordered calls or returns the same controlled
geometry to both sides. The actual Blizzard accessory is bound on the generated
side, but its later article creation/rendering is not executed. The fixture
does not prove full asset import, shared outer dispatcher execution, rendering,
gameplay, or real family publication; those have separate evidence.

Unused required Host slots trap. No native Nana/Popo move or follower handler is
called by the generated policy. Old full bodies run only on the reference side.

## Coverage and numerical choices

Whole-mode coverage varies caller/target motions, copying/recovery/grounded
flags, planner latches and floor/island geometry. Output covers all 370 tested
caller motions, Blizzard synchronization and control history. Eligibility covers
all 370-by-370 motion combinations and all four disallowed held containers plus
a normal held item. Start-copy cases exercise relative speed around 1.5 and
distance around 25 with recovery and behavior variants. Capture covers ten CPU
levels, three attempt counts and twenty fixed RNG samples.

Safety cases exercise missing/sleeping targets, invalid history cursors and
callback retirement with PAGE_NOACCESS source storage or same-address/new-ID
reuse. The target-query argument helper verifies one evaluation and stops before
the query or later writes when its nested callback retires the source.

Position smoothing is exact here: generated unsuffixed `0.95`/`0.05` literals
preserve the original double-promoted expression before Float storage. Capture
uses explicit Float locals for the original `0.05f * level` probability. The
earlier direct-literal candidate disagreed at a threshold and is preserved in
the ninth failure output; the final Float-local version matches the supplied
RNG samples. No new double-cast API or RNG substitution was added.

Strict original/history comparisons use nonnegative physical stick samples.
The original native decompiled C's negative Float-to-u8 conversion is outside
the defined-C parity claim. The accepted generic history backend uses the
documented deterministic signed-stick policy. This is not a claim about a
retail console defect.

## Immutable SDK and parser overlays

Every consumed frozen input is checked against its snapshot manifest. The
fixture overlays two direct mapping entries in a private copied SDK72 compiler
unit, leaving the snapshots untouched:

- RO Float `ground.mid_walk_point` maps to `fighter->co_attrs.mid_walk_point`.
- `targets.is_fighter(Int)` maps to the existing `target_is_fighter` Host query.

The parent approved and added these same mappings to production separately.
The generator rejects writes to the RO field, Float/Bool target identities,
Float-to-Int assignments and unsupported article/model-effect contexts.

## Full-pair merge contract

`follower.fms` belongs only to the secondary member. Place its top-level state
and helpers beside the accepted fighter helpers, then append the contents after
`move FollowerPolicy:` to the existing `OwnedLifecycle` move. It provides
prepare, initialize, spacing, stick-step, stick-limit, drop-command, mode-update
and output hooks. The declared `copy_controls`, `recovery_lock` and
`history_target` are ordinary script-owned state.

`capture.fms` belongs only to the primary member. Append its hooks after
`move PrimaryCapturePolicy:` to that member's existing move. It provides capture
decision and pummel-special hooks; returning false keeps the common pummel
command path.

`parameters.fms` names the exact frozen Common/Popo motion enum values and the
explicit follower target member 0. It is suitable for inclusion in both members;
these are policy/enum constants, not replacements for retail DAT tunings.
Existing fighter/Ice/Blizzard attribute recipes must still be imported through
their real DAT declarations by the final assembly.

The follower depends on the real accepted `blizzard_cue` helper and its ordinary
`blizzard_countdown`/`blizzard_emitting` state. Its own
`follower_start_blizzard` contains the actual entry work and binds that helper.
The fixture reuses `tests/native/ice_climbers_composition/{fighter,ice,blizzard}.fms`
read-only. The final pair should use the already accepted full member assembly,
not this fixture's synthetic base or uninvoked literal tuning defaults.

`query_fixture.fms`, `generate.c`, and `compare.c` are test-only. Do not include
their probes or synthetic boundaries in a playable member. There are no shared
core/default-template changes in this namespace.
