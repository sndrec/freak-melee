# Samus grapple policy checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The composed authoring candidate now owns the Up/Down/Up/A code sequence.
It samples exactly one branch on each normal update in held/flight modes,
using the existing physical D-pad and attack-edge properties. The completed
sequence persists; the existing creation and primary-death hooks apply their
stock resets. No Samus-specific scripting primitive is introduced.

Air wall contact during AirCatch now enters the authored AirCatchHit motion,
switches the beam to wall-extension mode and dispatches 0x41C then 0x3F1 at
the endpoint through its resource GObj. It returns before the ground bounce
path, matching `fn_802B805C`. The extra ItemLink argument passed to retail
0x41C is unused by the effect dispatcher; the existing position-only primitive
is sufficient. These branches use existing generic engine primitives.

The modified wall-hang timer now uses generic `input.l_held`: stock
`fn_802B8D38` tests physical L (held mask 0x40), not R (0x20), Z (0x10) or the
combined logical shield bit (0x80000000). `input.l_held` / `input.r_held` are
ordinary read-only physical-button observations documented in
[Fighter](scripting/fighter.md#player-and-down-input-history), available to any
fighter. The authored hang helper runs before ground/attack exits and the usual
countdown; it adds one only for a modified beam while L is held.

## Evidence

Current receipt: `A:/FreakMelee-test-archives/samus-grapple-hang-oct10/receipt.json`.
The earlier input/wall checkpoint is retained at
`A:/FreakMelee-test-archives/samus-grapple-policies-oct10-verified/receipt.json`.
The full composed audit prepares **278 bodies**, with only the deliberate
missing independent-Fighter declaration. The candidate remains unregistered.

The production compiler, storage and interpreter execute the exact
`Tether_AdvanceCodeSequence` body against unchanged
`samus_grapple_state_sync`. **8,200 comparisons** cover all 256 native byte
states, all 16 low held-button combinations, attack pressed/unpressed, and an
eight-update sequence with both directions held simultaneously. The test uses
controlled input reads with the production bindings' physical masks.

The hang helper matches the unchanged pre-exit conditional/timer slice of
`fn_802B8D38` in **240 comparisons**: three modified-mode values, every L/R/Z/
logical-shield combination, and five starting timers. Its input reads are
controlled with the exact physical masks; this is not the full resource solver.

The combined batch passes **77,145 lifecycle/input checks**, including the
existing 128 load/death cases and 384 attribute reloads, plus **480 geometry
comparisons and 6 safety cases**. Both C fixtures pass strict warning checks;
source, native references, tools and headers are pinned before/after execution.
Seven affected native targets build and the focused script/parameter CTests
pass. No game was launched and no installed packages were changed.

The air wall-contact/effect branch currently has native
source comparison and full preparation proof, not live VM/resource/solver or
renderer acceptance. Modified grapple homing and capsule-hit behavior, throw
attachments, constructor graph order and full lifecycle/gameplay acceptance
remain open. This checkpoint does not promote Samus to complete.
