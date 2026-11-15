# Samus owned lifecycle and scaling checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

2026-10-10. This advances the unregistered Samus source; it does not activate
an independent fighter or claim gameplay acceptance.

The main template now owns load, primary death and special-attribute reload.
Load enables wall jumping and uses generic attribute scratch setup. Owned
handles use -1 for absence; the redundant lazy Charge Shot initialization flag
is removed. Primary death resets charge, missile serial and spin-effect state
and restores model group 0. The composed tether candidate also resets its
handle and code-sequence state in those existing hooks.

Ten values are computed from immutable named parameters and instance scale at
load/reload: morph jump speed, bomb spawn height, bomb hop and air-start speed,
and six morph collision extents. Every live move/roll use now reads the derived
owned field. There are no per-frame parameter mutations or repeated scale
accumulation. These forty state bytes participate in the existing script state
snapshot path; no new storage or fighter-specific API is added.

The obsolete `lifecycle.pending.fms` is replaced by `lifecycle.component.fms`.
Composition extends the existing FighterEvents hooks, with checked replacements
rather than duplicate event registrations. Its damage/death2 helper follows
the stock cleanup chain: remove shot, reset charge, clear spin effects, remove
tether, then clear death2/secondary-damage/damage callbacks. Earlier notes saying
those slots remain registered were wrong: `ftCo_800D9C98` explicitly clears them.

## Evidence

Receipt: `A:/FreakMelee-test-archives/samus-lifecycle-oct10-final/receipt.json`.
The combined host batch passes **34,660 lifecycle checks**, **128 cases** and
**384 reloads**, plus the existing **480 geometry comparisons / 6 safety cases**.
Both fixtures pass strict warning checks. Inputs, copied asset project, tools
and headers are pinned before and after execution.

The production compiler, storage and VM execute the exact composed load,
reload, death and scale helper bodies. Whole native `ftSs_Init_OnLoad`,
`ftSs_Init_LoadSpecialAttrs` and `ftSs_Init_OnDeath` bodies are extracted unchanged
for comparison. Cases cross eight scales and sixteen parameter sets, check
initial load, repeatedly reload and change scale, and seed nonzero fields before
primary death. All ten derived values match native scaled attributes exactly;
handle absence is compared semantically (-1 versus native NULL).

Setup copy, model scale, visual-group operations and native article registration
are controlled host boundaries. Native load's four article registrations are
observed; scripted article admission is checked in the full candidate audit,
not simulated as additional load-time registrations. No live actor, graph,
resource destruction or render behavior is claimed by this cohort. The composed
damage/death2 cleanup has preparation proof only, not VM cleanup/borrower proof.

The main WIP audit prepares all **176 bodies**; the composed candidate prepares
all **276**. Both still report the intentional independent-Fighter package gate.
Seven affected native targets build, and the two focused script/parameter
CTests pass. No game launch, installed-collection change or upstream push.

Remaining Samus work includes modified grapple behavior, throw-accessory
authoring, real constructor/control/solver comparisons, and complete lifecycle
and gameplay acceptance. Historical generated-C receipts are not reused as
acceptance of the edited source.
