# Interpreted capture frame and throw setup checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-10. Ordinary fighter programs now prepare and execute two existing
generic event contracts without generated C: `catch_pull_frame` (Float, no
arguments) and `throw_victim_setup` (Void, no arguments).

## Runtime changes

The standing Catch-to-CatchPull scheduler runs the holder's frame callback
before anchor selection and the shared motion change. Its Float return is the
next motion's start frame; fractional values are preserved. Dash-catch does not
invoke this hook. Missing declarations retain the native frame policy.

The shared thrown-victim scheduler runs `throw_victim_setup` on the holder
after changing the victim's motion, before common statistics, timer clearing,
capture masking and animation initialization. Missing declarations retain the
native policy. Both sites require successful VM execution and retained actors
and capture relations before continuing. Neither event dispatch calls a stock
fighter-specific special handler.

The existing shared motion, anchor, attachment and throw routines remain in
control. This is callback migration, not a replacement capture implementation.

## Verification

The connected capture cohort passes **8,142 checks / 813 cases**. Its 35 new
cases exercise Float frame publication, standing/dash branch selection,
stock and interpreted victims, missing-hook defaults, callback faults,
actor/schema retirement, peer retirement and changed capture relations.
The fixture extracts the actual two production scheduler bodies and executes
the actual native victim body-status setter through the interpreted binding.

Receipt: `A:/FreakMelee-test-archives/script-capture-hooks-oct10-final/receipt.json`

SHA-256: `50875b6295ffcb3f9c0c3a1c2418947ed01c0dbcdfee5bab834ec46a2e1a9871`.
All 1,746 pinned inputs were independently rehashed unchanged.

Five native targets build: the client, content tool, family/package runtime
targets and capture enclosure target. The nine-source retained-body cohort
keeps five preparations passing; Yoshi still stops at `cpu.travel_frames`
on line 1574 (corrected from the earlier ranged-query label). No source body was removed to bypass that diagnostic.

Source receipt:
`A:/FreakMelee-test-archives/source-capture-hooks-cohort-oct10a/receipt.json`.
SHA-256: `5d6f4d954a8a7a558bf5bea925bc24c114582b4a73d316cad936fb2f858d105c`.

## Boundaries

Motion dispatch, animation startup, graph constraints, statistics and overlay
application are traced controlled fixture leaves. The new cases prove scheduler
ordering, ownership and typed VM dispatch; they do not compare a fully rendered
stock versus scripted Yoshi grab/throw. The earlier pinned caught-shape native
comparisons remain part of the same cohort. Owned articles, remaining CPU
policy bindings and full-character gameplay acceptance remain open.

No game or Dolphin process launched, no installed content changed, and no
remote push occurred. Compact whole-match rollback remains unfinished.
