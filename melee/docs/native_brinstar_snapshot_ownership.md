# Brinstar bubble snapshot ownership

The stage-hazard component now includes Brinstar's actual parameter pointer,
bubble cooldown, debris count, four base positions, and all twenty complete
bubble entries. `stage_hazard_state.c` owns the record. Brinstar functions use
those fields directly; the original numerical operations and initial values
are preserved. Bubble entry/state types move to the internal ownership header
without changing their native members or layout.

The expanded `NativeStageHazardSnapshot` is 2,800 bytes. Each potential actor
has GObj/Ground generation references. Each actor-backed optional bubble joint
has an ordinary class-piece backing reference and admitted class identity.
Stable backing references use a checked host cache; attachment changes refresh
through the actual allocator. The twenty-entry capacity is the engine's actual
bubble-array length, not an inferred allocator capacity.

## Dormant aliases and live graph roots

`grZebes_801DB088` can transfer a donor bubble's joint and actor to another
entry, then clear the donor's active state and actor while retaining its joint
pointer. That dormant pointer can subsequently dangle. New bubble creation
(`grZebes_801DAE70`) resets the pointer before reuse. Selection and updates gate
access through the bubble/actor lifecycle.

The snapshot preserves dormant pointer bits exactly but supplies no live class
claim for a joint whose entry has no actor. It does not dereference such a
pointer, require its backing to survive, clear it, or alter merging behavior.
An actor-backed joint is captured and validated as a live class borrower.
This distinction follows the actual callers; it is not a general permission to
ignore arbitrary dangling references elsewhere in the game.

Class identities are admitted from the initialized Brinstar stage's named
joints and actor-backed bubble joints. No foreign candidate class-info pointer
is followed. Candidate validation checks scene identity, backing spans, class
admission and generation-reference geometry without requiring present actor
occupation. GObj, Ground and class companions must restore before Load, which
checks all live roots before any hazard writes. Ordinary class spans do not
prove occupied-object generations or candidate graph membership; aggregate
prevalidation still has to establish those relationships.

Snapshot buffers and canonical unused union/reference bytes are explicitly
zeroed/checked. Save/Load do not allocate, call game callbacks or draw. Inactive
stages retain their existing empty-payload behavior, and dormant other-stage
records are not restored. The borrowed parameter payload remains outside this
component's ownership proof.

## Evidence boundaries

The connected owner fixture uses actual Ground/class owners, generic GObj
backing, controlled model initialization, parameter identity and an idle
scheduler. It exercises live and dormant joint paths, changed attachments,
freed actor/class payloads, companion ordering, malformed candidates and
repeated controlled evolution. It does not run actual stage animation or
whole-game resimulation. The separate live stage cohort captures and validates
at idle boundaries while comparing real rendered gameplay; it does not Load.

Other stage-private records, variable data (including allocated block arrays),
mutable borrowed attributes, other world/GM/HUD/effect/dynamics ownership,
multi-heap/resident closure and aggregate restore/history remain open.
Production rollback still uses the broad page oracle. This component's size
and timing are not complete-match rollback performance acceptance.


Final receipts under `A:/FreakMelee-test-archives/`:

- `brinstar-state-oct11-owner-b`: 76 additional Brinstar checks and ten malformed
  candidates, alongside the existing 178 hazard checks/25 malformed candidates.
  Repeated controlled ticks, slot-19 late failure, attachment replacement,
  a deliberately unmapped dormant pointer, freed dormant joint preservation,
  actor/ground/class companion restoration and buffer/activity/epoch refusal.
- `brinstar-state-oct11-stages-final`: six old/current rendered CPU scenarios,
  4,800 samples each (Brinstar, Corneria, Venom, Green Greens, Brinstar Depths,
  Final Destination). Populated gameplay traces match exactly and all forty
  shared/private capture/validation boundaries pass in each. Brinstar's observed
  hazard Save mean is 4.745 us; this is one component, not aggregate timing.
- `brinstar-state-oct11-eight`: stock and independent interpreted eight-actor
  scenarios pass all 28 components at all ten boundaries. Previous/current and
  diagnostic-on/off populated gameplay traces match exactly. No live Load.
- `brinstar-state-oct11-gm-a`: the connected control/player/camera-target restore
  fixture passes; no actual Brinstar model/startup graph is constructed there.
- `brinstar-state-oct11-independent-verification.json`: owner/frozen/live input
  pins, actual simulation versus host sections, all 56 donor Brinstar bodies
  and previous/current eight-actor traces independently verified. Four build
  targets and the focused GObj-consumer CTest pass.
