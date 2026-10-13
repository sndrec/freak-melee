# Samus tether fighter-phase composition

This namespace is an unregistered source fixture. It reads the accepted
`examples/samus_wip.fms` unchanged and composes its four specials, three combat
articles and CPU policies with new AirCatch and common Catch fighter decisions.
The Grapple Beam article is **an explicitly controlled test boundary**. It is
not a completed or usable Grapple Beam implementation, and the candidate must
not be installed or promoted as independent Samus.

The final SDK52 receipt is
`.local/samus-tether-composition52-first/acceptance.json`: **442,325 assertions,
8,338 source comparisons, zero maximum float difference**. The normal cloned
project, imported parameters, generated C, standalone DLL, console executable,
extracted original bodies, commands and diagnostics are retained in that fresh
output. Frozen SDK/compiler/tool hashes are checked before and after execution.
No game process, broad build or package installation is involved.

The compile logs retain22 generated-C warnings about discarded statement-
expression results and extra equality parentheses. They are not suppressed or
counted as gameplay evidence; the console executable exits successfully.

## Reproduce

From the repository root, use a fresh output directory:

```powershell
python -B tests/native/samus_tether_composition/source_compare.py --snapshot .local/baseline52 --output .local/samus-tether-composition52-repro
python -B tests/native/samus_tether_composition/audit.py --snapshot .local/baseline52 --project .local/samus-tether-composition52-repro/samus --output .local/samus-tether-assets52-repro
```

`--iso` can select an explicit input image. `audit.py` also accepts `--common`
for the original extracted `PlCo.dat`; its default is the preserved
`.local/mewtwo_common_stock.dat`. Outputs cannot overwrite the snapshot or this
source namespace. No generated C, DLL, executable, logs, DAT or FMP files belong
in the tracked namespace.

## Compared behavior

`aircatch_test.c` directly invokes actual generated functions, while a separate
ordinary DLL compilation checks the full composed module. Every original body
is extracted from an actual function-definition prefix in the frozen source;
call sites are not treated as definitions. The original standing/dash launch
velocity constant is also extracted verbatim.

- AirCatch motion357: input eligibility, entry, constructor timing and failure,
  obstruction, extension, retraction, removal, animation completion, physics,
  fastfall and collision/landing decisions.
- AirCatchHit motion358: entry and drift clamp, timers, displacement physics,
  collision responses and basic landing. Sixteen ordered normal input â†’ frame
  animation â†’ physics â†’ collision timelines retain the same article identity.
- Standing Catch212 and dash Catch214: 3,960 frame configurations cover the
  imported schedule, both facings, scales0.5/1/1.75, constructor failure and
  standing launch obstruction. Endpoint spark positions retain three RNG draws
  in the original order; the controlled RNG uses dyadic values. This is not a
  general claim of bit-exact equivalence for every floating-point RNG value.
- 145 actual common dispatch cases cover all three Catch input routes with
  held-item throw priority, start/registration, animation continuation, exit,
  CatchPull anchor/ready/follow and CatchWait pummel OR directional-throw release.
  The real `fn_800DA190` body is executed and proves that the native Samus follow
  callback has no effect. The authored policy leaves that callback NULL.

The matrix invokes these actual common bodies:

| Source | Bodies |
| --- | --- |
| `ftCommon/ftCo_AirCatch.c` | `ftCo_800C3B10`, `ftCo_800C3BE8`, `ftCo_800C3CC0`, AirCatch and AirCatchHit animation/input/physics, AirCatch collision |
| `ftCommon/ftCo_FallSpecial.c` / `ft_081B.c` | `ftCo_800968C8`, `ftCo_AirCatchHit_Coll` |
| `ftCommon/ftCo_0D95.c` | `fn_800D9558`, `fn_800D9930`, `fn_800D9C64` |
| `ftCommon/ftCo_Catch.c` | `ftCo_Catch_CheckInput`, `ftCo_800D8A38`, `ftCo_800D8AE0`, `ftCo_800D8C54`, Catch/CatchDash animation, `fn_800D8E30`, `fn_800D8E94` |
| `ftCommon/ftCo_0D8E.c` | `fn_800D952C` |
| `ftCommon/ftCo_CatchPull.c` | `fn_800D9CE8`, `ftCo_CatchPull_Anim`, `fn_800DA190` |
| `ftCommon/ftCo_CatchWait.c` | `ftCo_CatchWait_IASA` |
| `itsamusgrapple.c` | `it_802BAC3C` fighter teardown decision |

Production `native_callback_lifetime.c` and parameter parsing are linked. Live
fixture fighters have high-bit generation IDs. Unused Host entries abort if
reached; they do not silently succeed. The synthetic module base only supplies
the necessary motion rows and special entries. Its purpose is executing the
generated policies, **not copied-base metadata, full package ownership or
independent activation validation**.

## Controlled boundaries and limits

Article creation, reference lookup, signal effects and removal are controlled
boundaries shared by both sides. They provide matching item identities,
completion/anchor state, endpoint positions and ordered transition events.
The Grapple article boundary declaration supplies typed fields and real DAT
scalar imports solely for compilation. Its callbacks are not executed by the
matrix. There is no native solver behind an authored script call.

Other controlled boundaries are motion/freefall/landing entry; raw part51 and
endpoint world matrices; particle dispatch; random samples; ordinary physics
and drift/clamp helpers; segment and ECB collision responses; held-item throw
priority; capture callback registration; transition into CatchWait; pummel and
directional throw outcomes; and active Definition lookup. Native Link-only
policy branches are excluded by explicit Samus checks. Actual victim
CaptureWait transition bodies are not executed in this slice.

The original frame handlers dereference a missing raw grapple pointer after
the creation interval, and native CatchPull entry dereferences a missing
endpoint. Those undefined original paths are excluded from strict comparison.
The authored absent-resource ready result is tested as defined safe behavior.
Source retirement/recycling during synchronous transitions is covered by the
separate accepted engine lifetime fixtures; this matrix keeps retained fighter
generations live rather than pretending its controlled transitions establish
those engine side effects.

Existing specials/combat articles compile in the same module but are not
retested here. Grapple construction, nine article modes, model animation,
rendering, resource release, throw accessory and composed lifecycle require
their own actual-source acceptance. No full grapple, full lifecycle, renderer,
gameplay or independent Samus claim is made.

## Asset and pending lifecycle receipts

`.local/samus-tether-assets52/inventory.json` reads the normal cloned DAT and
records exact relocation/external symbols and hashes. The selected default
costume has60 physical joints and60 runtime parts. Raw attachment51 has no
semantic mapping; endpoint139 is an explicit external reservation, not a
physical joint. Grapple roots occupy special-attribute offsets0x64..0x70;
animation components0x74..0xAC are pointer arrays. Several material/shape
components are unresolved external symbols in this native input, which must
remain distinguishable from local graphs. Fighter item slot4 is a structured
throw-accessory descriptor, not a plain joint or article.

The old lifecycle.pending.fms is replaced by lifecycle.component.fms and
joined into the real interpreter candidate in samus_grapple_constructor.
Load/reset/scaling now have an actual VM/native host comparison; composed
damage/death2 cleanup has preparation proof. These edited components are not
covered by the older SDK52 receipts above. See the
[owned lifecycle checkpoint](../../../docs/native_source_samus_lifecycle_checkpoint.md).
