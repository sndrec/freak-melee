# Remaining owned-pair contracts (source census; not API activation)

No production edits are proposed as part of this namespace. All references below
are pinned by `stock_census.json`; additional shared references are pinned by
`source_manifest.json`. Early follower CPU/history/control work belongs to the
separate family foundation, not this phase translation.

## Exact load decisions

* `ftPp_Init_OnLoad` sets x2222_b5, copies attributes, publishes x40=attrs.x0,
  then registers three items0/1/2. `ftNn_Init_OnLoad` sets x2222_b4, copies the
  same attribute layout, then publishes x40=attrs.xC4; it does not register items.
* `spawn.horizontal_offset` already owns x40. Article descriptors should be owned
  by each package member (below), replacing reliance on Popo's global registration.
* Candidate generic Bool `family.retire_companion_on_sleep` would publish existing
  x2222_b5. `ftCo_800BFD9C`: first shared Sleep/invisibility/camera setup, then if
  flag and Player entity1 exists call ftCo_800D4F24(peer,1), then gm_80167320.
  This is retirement, not an ordinary special cleanup notification. Preserve its
  ordering and existing lifecycle guards when exposed; no new scripted dispatch.
* Candidate generic Bool `items.reject_healing_pickup` would publish existing
  x2222_b4. Both ftpickupitem_80094150 and ftpickupitem_800942A0 test
  `!flag || !it_8026B47C(item)`. The latter returns actual healing amounts for
  Heart, Tomato, Foods, Lucky Egg and WhispyHealApple; default0. This rejects
  NONZERO healing, not every item of those kinds. Existing items.block_ground_pickup
  writes x2223_b4 and cannot substitute for this filter.
* Nana on_death publishes attrs.xC8 armor before model groups and resource clears;
  represented directly. Both deaths reset ice spawn height (u.pp.x2250), represented
  by ordinary owned state. u.pp.x224C has only reset/diagnostic consumers and is
  not assigned invented gameplay meaning.

## Surface friction

`ft_GetGroundFrictionMultiplier` returns1 for raw Popo/Nana, otherwise calls
mpColl_8004CA6C. ftCommon_SetSelfMovementFromGroundedMovement multiplies ground
acceleration only when result<1, then projects ground acceleration/velocity onto
floor normal. ftCommon_800804A0 also applies that multiplier before xE8 publication.
`floor.friction` is the surface query, not the fighter's policy. Per-special
NoFriction calls already map to generic physics.ground_to_self_movement; this does
not close common walking/slope decisions. A generic immutable surface-friction
policy (use stage vs ignore stage) would reproduce this without callback retirement
hazards. No field/API chosen or written here.

## SFX ID remapping and KO cue

This is same-bank ID translation, NOT voice-bank selection. ft_80087D0C bank13
uses raw Popo/Nana +costume>=2 predicate ftCommon_80080144:
1. input0x1FBFD..0x1FC62 and alternate=true -> add0x66;
2. resulting0x1FC63..0x1FCC8 and alternate=false -> subtract0x66.
Ordered checks/range endpoints matter. ftLib_8008746C returns0x1FBD1 for alternate,
otherwise owned ft_data.x4C_sfx.x34. Seven-channel mute metadata is unrelated.
A bounded immutable costume-indexed ID-range translation and KO-cue override can
own both direct consumers without dynamic audio callbacks. Primary/secondary
Ice/Blizzard cue IDs are already named script parameters; staging sets Nana's
130090/0x1FC0F/0x1FBEE values directly as the accepted component importer does.

## Owned animation fallback

`ftData_80085FD4` returns the WHOLE primary wait descriptor when raw Nana,
non-Demo player, and own x24[ms].x14 isNULL; otherwise own entry. Primary comes
from global Popo bank today. `ftData_80086060` separately obtains live Player
entity0 for the nonnative archive-copy optimization; it is not required for
native bank resolution.

Retail PlNn has only eight named entries of321 (IDs313..320), so common motions
and primary special motions need fallback broadly. `animation_holes.json` records
serialized name absence, not runtime pointer presence. Raw serialized x14 zeros
must not be mistaken for imported FigaTree absence.

Consumers include ftAnim setup/evaluation, ftData native clip lookup, ftCo S3/S4
availability, Appeal facing, DownSpot, SquatWait, dead/camera tests and pltrick.
A clip-only runtime fallback would be incomplete. Prefer physically materializing
Nana's own descriptor/animation bank at package creation, eliminating runtime donor
resolution. See `physical_animation_plan.md`:260 missing entries have primary clips,
with199 distinct reusable blobs plus eight Nana clips. Preserve Nana command graphs,
blend bytes and own flags; copy animation name/offset/size/blob availability. All
loop bits agree, so shared lookup consumers retain their original loop decisions.
Normal construction imports the complete resulting bank; no live peer is required.

## Squall retained model branch

Complete ftPp_SpecialS_8011F720 body is compiled into the collision fixture. Its
nonNULL branch selects root.child.next, shows it only in343/344, sets root Z to
-atan2f(floorNormal.x,y) when command3 and ground marker, then binds effect hitlag
callbacks. Invalid pointer/flag branches clear pointer +hitlag callbacks.

Current complete C census has no nonNULL assignment to mv.pp.specials.x8, only
entry/resetNULLs. Exact retail343..346 graphs for both forms, including call/jump
closure, contain no gfx opcode10 (`squall_retail_commands.json`). Recipe4ED exists
in efSync but stock ftCo_8009F834 animation gfx routing has no4ED case. There is
no source-backed construction/timing to activate a named model policy. Preserve
this unreachable branch evidence; do not invent a spawn. The healthy comparison
pins NULL transport; abort stubs reject accidental traversal.

## CPU/common decisions and ownership boundary

Existing generic on_cpu_capture_decision +cpu.grab_attempts/level/random/run_script
can own Popo's x94==0,level>4,Randf<.05*level -> script59 branch in ftcpuattack.
Nana drop wait1/down-127/wait5/neutral/wait1/Done is representable by cpu_drop_command.
Nana spacing multipliers1 and initial1 (versus common4/3.5), history30, mode6,
follow distance15 and fixed stick step64/clamp127 require the separate early
CPU/control foundation. No CPU callbacks are claimed by this composition.

Family coordination is already ordinary creation IDs +typed recipient exports;
no retained native pointers or private union writes. Family registration/member
publication/early initialization is still pending architecture acceptance.
Kirby donor/copy dynamics are explicitly outside this requested scope. Normal
landing clears unused u.pp.x224C only; no player-visible owned variable is invented.

## Normal project pipeline and metadata

There are no Popo/Nana copied-base registration files or corresponding stock article
slot entries today. No clone-stock/import success is claimed. `prepare.py` extracts
real per-member retail DAT attributes and builds inherited test Definitions only.

`ownership_census.json` contains machine-readable owned metadata proposals:
40 exact semantic bindings,49 runtime parts,zero omissions,four verified costumes
per form; roots ftDataPopo/Nana, PlPpAJ/PlNnAJ,321 animations,14 demos,EFbank14,
sound mask0x44. Article layout mask0x7,graph mask0,ClimberString typed decoder;
Ice0,Blizzard1,GumStrings2 with no dynamic kind offset. Rope attribute graph fields
24/28 are inside typed slot2, not separate top-level graph-mask slots. Rope count40.
Demo Result/Intro/Ending names are form-specific; both ViWait names are Popo.

Root must register both owned bases/assets and choose the immutable companion family
package presentation, preserve Nana entry asymmetry and physically materialize Nana's primary fallback animation data.
Only after remaining load/common/CPU policies and normal fresh clone/import/generated
module acceptance can behavior-independent be declared. This candidate retains its
stock source_kind and contains no promotion to default templates.
