# Current direct-source companion runtime audit

This audit separates current runtime helpers, source preparation and remaining
activation work. Earlier claims
that source CPU hook slots were absent are obsolete. Those hooks and scheduler
boundaries are now implemented; this document identifies remaining runtime work,
separately from source preparation and historical generated-C evidence.

## Existing ownership to reuse

`native_fighter_runtime.c` already gives each `NativeFighterSlot` a family,
immutable member index, source package and native kind. Two-member preparation,
leases and atomic publication exist for alternate forms. `bind_family` registers
both kinds and derives the alternate flag from the package mode. No second
actor-family registry or wrapper layer is needed.

`prepare_family` still rejects SimultaneousCompanion before either member prepares
(`native_fighter_runtime.c::prepare_family`); `can_bind` accepts Single/AlternateForm.
AlternateForm already prepares both sources, requires both
`form_activated` events and publishes both banks/tables/leases. The statement
that all paired families reject is obsolete. Companion admission still needs
constructor history/member integration and retained native continuations;
removing the checks alone would not establish complete Popo/Nana behavior.

`bind_family` already registers both kinds with the actual mode's alternate flag.
`Player_80031AD0` creates the primary with `b0=false`, publishes it, then creates
the secondary with `b0=true` and `has_transformation=false` for companions.
`Player_8003248C` already classifies that nontransform secondary as CPU for human
or CPU ports. No new spawn API, species handler or second family registry is
required. Family authoring/build/packet transport accepts both two-member modes;
each member prepares through its own Single source-owner input. That internal
owner discriminator is not the family-mode blocker.

## Immutable member queries

`nativeFighterRuntime_MemberId` uses the existing active slot's immutable member
index and family. It requires a published interpreted caller, then searches both
Player roles for the requested member's registered kind and exact selected
package/program/storage and physical identity. Role swaps and dormant alternate
actors do not change member ordinals. The published program is a copy of the
prepared package program: selection matches its compiler bank and identity,
rather than requiring both program addresses to equal. Caller, selected member
and family/Player relationships are retained; absent members return zero.

Constructor history selection is a separate boundary. `Player_80031AD0` assigns
each `player_entity[]` only after `Fighter_Create` returns. Fighter load/reset/
death/CPU preparation occurs inside that constructor, before the caller has a
published player role. Current `Fighter_Create` attaches the actor before its
reset/death/CPU preparation path; attachment itself creates storage before Load.
`CpuHistorySource` nevertheless admits the selected physical caller without
requiring actor storage, and retains that storage whenever present. Only
simultaneous member one can select already-published member zero in the same
Player, selected character and family. Primary, Single, AlternateForm and absent
donors return zero. The ordinary query still requires a published caller. This
also supplies the secondary's later reset hint, without a constructor registry.

## Source CPU hooks exist

`native_script_fighter_hooks.c` and `ftCo_0A01.c` now connect prepare, initialize,
spacing, drop, stick step/limit, mode and output hooks through typed interpreter
dispatch. The engine retains `NativeFighterLifetime` across these callbacks.
Reuse this implementation and the `script_cpu_hook_abi` / `script_cpu_scheduler`
fixtures; do not rebuild hook registration based on the old audit.

The interpreter now implements CPU mode/follow distance, candidate gates,
destination preparation/assignment and approach spacing; record/apply/read-pose
history; combat/recovery/attack/no-target planners; and typed controller outputs.
The actual `native_cpu_history.c` uses the existing inline 30-cell ring.
`script_cpu_control` connects these operations to actual actor/storage/VM
execution, with deeper planner leaves explicitly controlled. Their implementation
does not complete paired scheduling. The runtime now supplies the separate
constructor history hint, while companion admission remains closed. The legacy
exporter/Host implementations and generated DLL
comparisons remain behavior references, not interpreter activation evidence.

The actual mode/output Bool contract matters: absent or false mode handling can
reach native mode 6 (`ftCo_800B101C`), and false output handling can reach stock
peer/Blizzard decisions. Complete Nana must own those decisions in scripts;
recognizing the hooks alone does not establish acceptance.

## Native callback continuation still needs care

`ftcolanim.c::ftCo_800BFD9C` caches its Fighter, enters Sleep, optionally retires
the role-one peer, then uses the cached Fighter to update match status. The
Sleep entry also continues after `Fighter_ChangeMotionState`. Authored motion
and peer-death callbacks can invalidate those cached objects. The peer path
`ftCo_800D4F24` also continues into its inline Sleep work after the void
`ftCo_800D331C` death routine returns; that routine's internal early return does
not stop its caller. Enabling companion retirement requires exact caller/peer
retention across these real boundaries, rather than relying only on the outer
binding's final guard. This audit makes no engine changes and does not claim the
existing continuation is safe.

## Smallest next integration cohort

Immutable lookup and constructor hint are implemented, with connected source VM,
physical actor, native Player role-swap and complete CPU initializer proofs using
explicitly controlled active-family/package inputs. Those proofs do not activate
a simultaneous package or execute the native constructor. The remaining cohort:

1. Retain source/peer across Sleep, peer death and their native continuations.
   Allow ordinary dormancy and expected peer retirement, but stop cached reads
   after source fault, generation/userdata/schema change or physical reuse.
2. Admit SimultaneousCompanion through the existing two-member preparation,
   activation preflight, lease unwind and Player registration transaction once
   those paths have connected proof. Test primary/secondary construction failure
   explicitly: the current Player constructor publishes each return separately
   and does not make pair creation atomic. Do not infer construction success from
   package publication.

This is a bounded runtime/member/constructor cohort, not a rewrite of CPU policy
or full linked-special/rope gameplay. The physical Fighter pool and interpreter
actor pool both have capacity 64; capacity alone does not establish four-port /
eight-actor scheduling, teardown or rollback acceptance.

An adjacent preview issue is separate: `nativeHome_WorkshopInitialMember` and the
Workshop actor selector still require non-NULL callback-bearing Definitions.
Ordinary source registration returns NULL Definitions, so these old checks do
not establish source alternate-member preview selection. Use selected family /
source identity when that preview slice is assigned; it is not required to
replace the native simultaneous constructor.

## Verification boundary

Extend existing `source_family_runtime` preparation/publication tests, physical
`script_family_queries`, alternate role-swap coverage in `source_form_transfer`,
and current CPU ABI/scheduler fixtures. Historical family fixtures using a
controlled legacy importer are references, not current package acceptance.

Then exercise actual two-member construction and four ports/eight actors:
follower separation/rejoin, input history, linked specials and rope cleanup,
independent death, Sleep, respawn and scene changes. Generic collision copying
is implemented separately; it does not provide pairing or velocity policy.
Whole paired Item/resource lifetimes and visual behavior are not established by
the isolated animation/resource binding cohorts.

Rollback must restore both members, player roles, script storage, history,
links and owned Items/resources together. Existing broad Regions/Pages snapshots
remain the oracle; compact whole-match save/load is not complete.
