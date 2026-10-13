# Interpreter contract and migration status

Updated: 2026-10-13. The 2026-10-07 decision to use the native interpreter has
been integrated into ordinary package loading, engine scheduling and Workshop
source validation. This is the current contract, not the initial migration log.

## Backend decision

Ordinary authored fighter/article/item/effect behavior runs from `.fms` through
a typed frontend, bytecode lowering and fixed-pool interpreter. Authors and
recipients do not require a C toolchain, generated C or behavior DLLs.
The earlier generated-C path remains only in explicit behavioral reference
tests. Handwritten native extensions are an advanced facility, not a fallback
for unsupported ordinary source.

The evaluation compared real eight-actor behavior and found interpreter overhead
modest relative to the original snapshot/render costs. The user chose simpler
direct execution and a proper mutable-state system instead of preserving C
generation for performance. That decision does not itself prove the frame budget.
Further performance work is now deferred; retain the accepted runtime.

## Production contracts

- Validate complete source, symbols, types, binding signatures, resources and
  immutable bytecode structure at preparation. Reject unsupported operations
  explicitly. The program's prepared marker is published only after validation.
- Pin immutable programs/assets for every live actor or retained relationship.
  Per-instance variables/callbacks/clocks belong to the mutable match state.
- Publish candidate assets, parameters, program and family members coherently
  at a scene boundary. A failed candidate must not partially replace live state.
- Generic native bindings provide reusable engine primitives. Fighter-specific
  decisions remain in scripts; do not delegate a special to its stock handler.
- Preserve dynamic type/output checks and source/recipient lifetime barriers
  across native calls, callbacks, helper exports and interpreter reentry.
- Keep original-creator provenance, current damage owner, holder and revocable
  attachment distinct. Retirement or pool reuse must not mutate a replacement.
- Respect numerical behavior at observable boundaries and document intentional
  corrections to original undefined/bugged behavior. Native peers must agree.
- No per-tick heap allocation or hidden legacy compatibility interpreter.

## Integrated baseline

All 26 supported forms, excluding Kirby, prepare through normal source packages.
Single and alternate/companion families own their data, member programs and
parameters. Stock/source CPU, special-entry, paired-family, selected interaction
and result cohorts are recorded in [roster status](SCRIPT_PORT_CHECKPOINT.md).
These are bounded comparisons, not exhaustive whole-fighter acceptance.

Workshop uses the current source validator/tool and bytecode candidate path.
Per-move files, editable state declarations and named parameter overrides remain
project-owned. Frozen previews resolve the actual member/program lifetime rather
than using a native donor Definition as a package discriminator. See
[Workshop status](native_character_workshop_progress.md).

Production rollback now uses admitted mutable arenas and a confirmed checkpoint;
page faults and broad history are diagnostic only. Interpreter state, object
identity, allocation order, callback bindings and relationships restore together.
Hidden reconstruction preserves required CPU/effect state while omitting GPU
output. See [current rollback](native_rollback_current.md).

## Remaining work

The remaining backlog is Model/editor capability and usability, current
source-only sharing/release acceptance, broader targeted behavior/lifetime
coverage and later physical multi-PC testing. Do not restart the frontend,
whole-roster port or snapshot ownership migration from historical notes.
The 10 ms median / <=12 ms peak full reconstruction target remains unmet and
deferred, not silently removed from acceptance.

See the [project plan](../../PROJECT_PLAN.md), [scripting guide](scripting/index.md)
and [dated evidence index](checkpoint_index.md). Pre-migration C/DLL architecture
and early measured costs are preserved in the private source backup commit.
