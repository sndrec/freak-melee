# Versus stage startup sweep (2026-09-20)

The Versus stage-select table in `src/melee/mn/mnstagesel.static.h` has 29
selectable stage IDs. The test stage (1), Akaneia (21), and IceTop (26) are
absent from that table and are outside this sweep.

Each stage was launched directly into a one-stock four-fighter match with
Ganondorf (human), Zelda, Ice Climbers, and Mr. Game & Watch (CPUs), using the
supplied USA 1.02 ISO and the native client without a debugger. For stages
without a committed importer specification, a temporary diagnostic fallback
used a 0x200-byte yakumono span with no indexed animation layout. That fallback
was removed after the sweep: 0x200 is not the actual per-stage archive schema,
so the results below are triage evidence, not proof that those archives are
validly imported. A first pass watched for Versus scene entry; a second pass
kept every apparent survivor running for ten more seconds. An entry marker
alone was insufficient: Kongo Jungle panicked immediately afterward, while
Corneria and Brinstar crashed within ten seconds.

| ID | Versus stage | Result |
| ---: | --- | --- |
| 2 | Fountain of Dreams | Typed 0x54-byte import plus native auxiliary symbols; both moving-platform controllers completed movement, submersion, hidden-wait, and restoration cycles |
| 3 | Pokemon Stadium | Typed base and four transformation-archive imports; accelerated full live intervals observed for fire, grass, water, and rock |
| 4 | Princess Peach's Castle | Typed 0x144-byte import and native dynamics roots; accelerated satellite, hazard, and pressure-platform cycles observed |
| 5 | Kongo Jungle | Typed 0xBC-byte import; pendulum mesh, swinging barrel, occupant, material, and all three scheduled event classes exercised |
| 6 | Brinstar | Typed 0x190-byte mixed-width import; acid movement, tendril trigger, and breakable-platform state transition observed |
| 7 | Corneria | Typed 0x8C-byte import, four indexed animation layouts, and native Arwing-laser attributes; all thirteen flight types, both attack groups, and repeated cannon cycles observed in a deterministic 165,103-sample run |
| 8 | Yoshi's Story | Typed 0x24-byte archive import; complete Randall circuit and all six Shy Guy spawn patterns observed |
| 9 | Onett | Typed 0x68-byte archive import; accelerated one-stock match reached results |
| 10 | Mute City | Typed 0x50-byte import, indexed map-29 animation layout, native car state, and native car animation record indexing; track, collision, damage, and destruction paths observed |
| 11 | Rainbow Cruise | Typed 0x48-byte archive and ship-flag dynamics import; accelerated route observed through platform, cannon, and ship-return sections |
| 12 | Jungle Japes | Typed 0x20-byte archive import; Cranky, Klaptrap, water current, and one-stock results exercised |
| 13 | Great Bay | Typed 0xA4-byte mixed-width archive import; native Tingle collision state; accelerated Tingle, turtle, and moon-event traces |
| 14 | Hyrule Temple | Typed four-byte archive parameter import; accelerated one-stock match reached results |
| 15 | Brinstar Depths | Typed 0x34-byte parameter and indexed animation import; full Kraid attack and bidirectional rotation cycles observed |
| 16 | Yoshi's Island | Typed parameter and indexed material-animation import; all nine destructible elements completed break, collision-removal, recovery, and restoration cycles |
| 17 | Green Greens | Typed 0x7C-byte import and native block state; Whispy wind/apple cycles, block interactions, and one-stock results observed |
| 18 | Fourside | Typed 0x4C-byte mixed-width parameter import; crane, UFO, and helicopter cycles plus one-stock results observed |
| 19 | Mushroom Kingdom | Typed 0x54-byte mixed-width parameter import; block and balance-platform cycles plus one-stock results observed |
| 20 | Mushroom Kingdom II | Typed 0x4C-byte parameter and indexed material-animation import; repeated Birdo lifetimes and egg volleys observed in a deterministic 17,704-sample run |
| 22 | Venom | Typed 0x3C-byte import, complete indexed Arwing animation layouts, and native controller state; all eleven flight types and both attack groups observed in a deterministic 103,310-sample run |
| 23 | Poke Floats | Typed four-byte archive parameter import; accelerated full controller cycle and moving collision vertices observed |
| 24 | Big Blue | Typed 0x144-byte archive import; accelerated one-stock match reached results |
| 25 | Icicle Mountain | Typed 0x13C-byte parameter and linked-array import; accelerated one-stock match reached results |
| 27 | Flat Zone | Typed 0x40-byte archive import; native tool-machine state repair; complete machine cycle and repeated falling-tool spawns observed |
| 28 | Dream Land | Typed mixed-width archive import; repeated Whispy wind cycles, both wind directions, bird events, and a 4,062-sample deterministic replay observed |
| 29 | Yoshi's Island (64) | Typed 0x1C-byte mixed-width archive and 13-joint indexed animation import; repeated cloud vanish/restore cycles and background guest events observed in a deterministic 11,585-sample run |
| 30 | Kongo Jungle (64) | Typed 0x70-byte mixed-width archive and three-joint barrel animation import; barrel motion, fighter launch, and repeated bird cycles observed in a deterministic 28,288-sample run |
| 31 | Battlefield | Already supported by typed archive import and full match smoke |
| 32 | Final Destination | Already supported by typed archive import and full match smoke |

All 29 selectable stages now reach gameplay. The temporary fallback used during
the initial sweep may itself have caused some access violations by importing
past a stage's parameter object. Green Greens, Jungle Japes, Yoshi's Story,
Flat Zone, Yoshi's Island (64), Kongo Jungle (64), and Great Bay were promoted
only after their source-defined parameter sizes passed the real-ISO importer
test and a live native run. The Jungle Japes run reached the results screen.

The same archive audit found that Dream Land's first four yakumono fields are
16-bit values. The retail archive stores 3000, 4000, 30, and 0 in that order;
the old all-32-bit copy read them as 4000, 3000, 0, and 30 on Windows. The
importer now reads the declared 16-bit fields individually, and the real-ISO
test compares every converted field with its source value. The same check now
covers the mixed-width fields of both N64 stages. A fresh Dream Land match
remained live for 35 seconds after the fix. This corrects imported data; it
does not change the original stage logic.

Yoshi's Island (64) stores each of map 2's three animation groups as thirteen
contiguous joint descriptors. The generic graph import preserved the hierarchy
root but not that indexable source array. Cloud 1 therefore selected unrelated
native material-animation data at joint index 5; its 96-frame disappearance
animation acquired a loop flag and the cloud remained forever in state 1. The
stage import now declares the 13/13 indexed layout, and the real-ISO test checks
the final joint entry in both animation arrays. A fresh accelerated trace
observed Cloud 1 repeatedly traverse loaded, disappearance, hidden cooldown,
reappearance, and restored states while the long-period background guests also
selected and completed animations. With diagnostics removed, all 11,585
controller samples replayed without RNG or fighter-state divergence.

Kongo Jungle (64)'s barrel controller addresses joint 2 in all three animation
groups of map 1. As with the Yoshi's Island clouds, the generic graph import
preserved each hierarchy but not the contiguous array required by direct joint
indexing. The stage now declares the retail three-joint layout, and the ISO
preflight checks joint 2 in all three groups. The barrel-release cleanup also
reads the typed `HSD_GObj::p_link` field rather than assuming the GameCube byte
offset; the equivalent stale offset access in Kongo Jungle was corrected at
the same time. A fresh accelerated trace observed repeated rotation cycles, a
complete horizontal movement cycle, four bird animation start/completion
cycles, and two fighters naturally entering the barrel. The first occupancy
progressed through wait, launch, release, and idle. After diagnostics were
removed, all 28,288 recorded controller samples replayed without RNG or
fighter-state divergence.

A later controller audit traced Whispy repeatedly advancing through its idle,
wind-start, wind-active, and return phases from both sides, plus multiple bird
patterns with one- and three-bird groups. During the longer diagnostic replay,
Popo's up-special exposed an unrelated native arithmetic mismatch: the
non-PowerPC `__frsqrte` placeholder returned a square root even though the
original instruction returns a reciprocal-square-root estimate. The original
three Newton iterations therefore diverged to infinity for a normal squared
distance of about 1,785. The native definition now supplies reciprocal-square-
root semantics; the Ice Climbers code and attributes are unchanged. A focused
test covers the intrinsic contract, and a fresh corrected-build Dream Land
recording replayed all 4,062 controller samples with matching RNG and fighter
state and no nonfinite-state report.

Green Greens exposed a native type mismatch in the shared stage-animation
helpers. The decompilation declared animation and joint indices as `bool`;
MetroWerks' `bool` is an integer typedef, but native C's `_Bool` converts every
nonzero index to one. Consequently, stages requesting animation 2 or higher
silently loaded animation 1, and completion tests for joint 2 or higher watched
the wrong object. The helpers now accept integer indices, preserving the values
the original code passes. Accelerated traces then observed Whispy's full wind
sequence through phases 0-6, multiple apple-spawn sequences, normal and bomb
block destruction, and a one-stock match reaching results.

The same pass gave Green Greens' block positions and block grid explicit native
ownership. Its destroy callback now frees those typed pointers directly instead
of reaching them through an unrelated stage-union view that happened to share
GameCube offsets. Block-hit callbacks retain their source object as a pointer,
and the internal break/compaction functions now have purpose-based names and
accurate return types. These representation changes preserve the original block
behavior while removing 32-bit pointer truncation and layout assumptions.

Fountain of Dreams exposed a separate native layout problem. Its first crash
was a null GameCube archive lookup for a reflection image. The native path now
imports that image and the stage's star joint directly from their DAT public
roots. The next crash was a write through a hand-built GameCube offset-0x18
view of `Ground` user data: on x86-64 it overwrote a callback pointer. The
stage code now writes `Ground.x18` directly. A no-debugger run then rendered
the reflection and fighters for 35 seconds without a crash. A later controller
audit found that platform creation wrote the 25-unit resting height through the
reflection-stage union view, then the platform update read it through a
different view. Those views shared offset 0xDC on GameCube, but widened native
pointers moved the fields apart. The platform now owns explicit, purpose-named
state, timer, collision, joint, height, target, model-height, and resting-height
fields, and creation writes the same typed state the update reads. Temporary
tracing over roughly 17,000 accelerated simulation frames captured both side
platforms choosing multiple higher and lower targets, updating their moving
collision, fully submerging and hiding, waiting, reappearing, and returning to
the correct 25-unit resting height. The trace code was removed after the run.

Rainbow Cruise's ship flag uses six packed 0x3C-byte dynamics records in the
stage DAT. The native importer now reads those records and the descriptor's
position from the original archive. Its startup also exposed two x86-64 layout
assumptions: a runtime array had been allocated using the smaller source
description array's size, and distinct ground-state views had aliased at
GameCube pointer offsets. The runtime arrays now use their actual entry sizes,
and the stage reads the same typed view that wrote each pointer. A later
99-stock accelerated run remained responsive through the rainbow/checker-wall
ascent, cannon and moving-platform geometry, and return to the ship. Captures
showed changing collision geometry and fighters on every section. This covers
one route loop; exact transition timing has not been compared with retail.

The route controller's temporary transition object was still stored in a
32-bit integer and cast back to `HSD_GObj*` at every use. It is now a native
pointer, and the completion path only dereferences it when the optional object
exists. A fresh accelerated one-stock match traversed the route and reached
the four-player results screen. This removes pointer truncation from the route
state; it does not add a replacement controller or change the transition
schedule.

Great Bay's source parameter structure includes short timing values and
item-weight pairs among its floating-point fields. The native importer now
converts all 30 declared halfwords individually. A real-ISO test compares each
converted value with its source. Tingle's stage-collision callback previously
wrote collision flags, counts, and force through the turtle state view. Those
views share offsets on GameCube, but the turtle view's native pointer widens
and shifts the later fields on x86-64. The callback and update routine now use
one explicit Tingle state structure. Temporary accelerated traces observed
Tingle progress through states 1-7 with collision force reaching the update,
repeated complete turtle state cycles, and the moon event starting. The moon's
non-looping animation reached its 4,690-frame endpoint and stopped. The retail
animation descriptor has no loop flag and its scene loop-override byte is zero,
while the original reset branch specifically waits for a loop rewind. That
branch is therefore unreachable with the shipped asset rather than an
unexercised native path; the importer test now locks the retail flags and end
frame.

Hyrule Temple's source stage logic stores the yakumono parameter pointer but
does not read fields from it. The retail DAT has exactly four bytes between
that root and the next referenced object. The native preflight asserts that
span and imports the stage graph; a no-debugger four-fighter match stayed live
for 35 seconds with the stage and fighters visible. A later accelerated
one-stock match completed and displayed the correct four-player results screen
(`screenshots/melee-1789890820-0000.bmp`), covering the match-end path as well
as stage startup and live simulation.

Brinstar Depths uses a 0x34-byte yakumono parameter and contiguous animation
descriptors for at least 19 joints in scene 3. Its stage code addresses joint
18 in two animation groups; importing each group as a single descriptor caused
an access violation when Kraid's hand animation ran. A real-ISO preflight now
imports those indexed descriptors. The no-replay `--fast` run reached the
one-stock results screen without a crash, with four fighters visible during
the match. This tests the early Kraid sequence and match-end path, not every
stage event.

The archive's joint-map table also supplies the four Versus spawn points. The
native importer previously left each table key as a raw 32-bit DAT offset,
which could never equal the imported scene joint pointer used by the original
stage setup. It now imports the joint references and keeps the pair table in
its native layout. Real-ISO preflights check that Battlefield and Brinstar
Depths map joints match scene roots and that points 0-3 exist. A Battlefield
startup capture shows the four entry effects at separate stage positions,
followed by fighters on the stage. This is evidence for these two stages, not
an audit of all 29 spawn layouts.

Fourside's stage parameters contain three 16-bit fields at offsets 0x44-0x48.
The real-ISO test verifies the 0x4C-byte span, all three converted halfwords,
geometry, and four spawn-point mappings. An accelerated one-stock match reached
the results screen after the helicopter, UFO, moon, and fighters were visible
in screenshots. A later 11,566-frame accelerated trace observed repeated
helicopter state cycles through approach, hover, departure, and wait; three UFO
cycles through approach, landing, stay, departure, and hidden cooldown across
two buildings; and repeated crane cycles through every state 0-7, including
both upward and downward target selection and damped stopping motion.

Mushroom Kingdom's stage parameter has six 16-bit fields at offsets
0x14-0x1E. The real-ISO preflight validates the 0x54-byte parameter span,
converted halfwords, geometry, and all four spawn mappings. An accelerated
no-debugger match stayed live for 21 seconds with the pipe, moving platform,
blocks, and fighters visible. A later 10,800-frame accelerated trace with items
enabled observed normal and question blocks being struck, hidden, respawned,
flickered back in, and returned to their steady state. The balance-platform
controller traversed states 0-1-2-3-0, including the platform fall, wait,
collision restoration, and return to equilibrium. A clean one-stock run with
the same item setting then reached the correct four-player results screen
(`screenshots/melee-1789891007-0000.bmp`).

Poke Floats stores but does not read its four-byte yakumono parameter. The
real-ISO preflight asserts that exact span and imports the stage graph. An
accelerated one-stock match reached results, with the floats and fighters
visible during play. A later 99-stock accelerated run sampled the animated
controller hierarchy, child visibility, and collision vertices every 600
frames. The visibility masks progressed through the float sequence, controller
nodes 0 and 24 crossed the stage, and collision vertices 20, 284, and 513 moved
with their corresponding models. The controller returned to its initial state
at approximately frame 12,600 and began the same sequence again. The coarse
samples observed 23 of the 25 child visibility channels; this establishes a
complete advancing and looping controller path, but not frame-exact comparison
of every float against the GameCube version.

Icicle Mountain's parameter root spans 0x13C bytes and contains three linked
halfword arrays plus 32 spawn records. The original source's parameter layout
described only one spawn record and gave the field at offset 0x4 the wrong
type; the shared source/import definition now matches the retail DAT. The
real-ISO preflight checks the array lengths and values, converted halfwords,
spawn records, geometry, and all four spawn mappings. An accelerated live
one-stock match reached results. A recorded run initially crashed on results
because the imported common fighter assets had been allocated in a scene heap
and reused after that heap reset. Keeping those cached assets alive across
scenes let the exact 3,435-sample replay exit cleanly. The replay still logs a
nonfinite Ice Climbers state at sample 1,417. That September 14 recording is no
longer a current-build reproducer: subsequent gameplay and stage fixes cause it
to diverge consistently at RNG sample 983. A fresh current-build capture ran
for 30,455 samples without a nonfinite report, and replayed all 30,455 samples
without RNG or fighter-state divergence. The old symptom therefore is not a
confirmed current defect, although the change that removed it was not isolated.

The moving-section selector also advanced a `Ground*` by two bytes to iterate
the twelve halfword cooldowns in the original packed layout. It now indexes
named section and row cooldown arrays directly, preserving the same selection
and decrement rules without depending on the native `Ground` layout. The same
controller exposed a PowerPC return-register dependency: its translation
routine left the requested delta in `f1`, and the caller passed that register
straight to the section update even though the C body had no return statement.
The native routine now returns the input delta explicitly. A fresh accelerated
one-stock run reached results after the fix. One earlier run asserted in the
nametag projection after a player position left its valid range; the concrete
undefined return is fixed. The current-build record/replay check above no longer
reproduces the nonfinite Ice Climbers state.

Mushroom Kingdom II's parameter has twelve 16-bit fields in its 0x4C-byte
root. Its map 2 uses indexed material-animation descriptors through joint 7
in three groups; importing only one descriptor per group caused the startup
access violation. The real-ISO preflight checks the span, converted halfwords,
all four spawn mappings, and the indexed material groups. A later match crash
came from a GameCube-sized pointer and following animation flags sharing a
union view; the native pointer's upper half overwrote those flags. The stage
object now stores its link as a pointer in the same typed view as its flags.
A no-debugger accelerated one-stock match showed the stage and fighters, then
reached results. A later 99-stock trace observed repeated complete Birdo
lifetimes from both stage sides, including movement into the arena, egg volleys,
normal exits, destruction callbacks, and the timer rearming the next spawn. A
17,704-sample current-build recording containing those cycles replayed to its
endpoint without RNG or fighter-state divergence. The first unrecorded attempt
at this validation hit the generic nonfinite-ECB assertion in
`mpCollInterpolateECB`; a debugger run and the deterministic record/replay did
not reproduce it, so that isolated collision failure remains unassigned rather
than being attributed to the Birdo controller.

Yoshi's Island uses 0x20 bytes of source-defined parameters, followed by four
zero bytes before the next referenced DAT object. Its map 1 material-animation
groups address joints through index 26. The real-ISO preflight checks the
parameter span, zero tail, geometry, four spawn mappings, and indexed groups.
The first live run reached the destructible-platform callback but crashed when
that callback cast a 64-bit `Ground*` through a 32-bit integer. Keeping the
pointer directly preserves the original no-op and its native address. A fresh
accelerated one-stock match displayed the stage, fighters, and blocks, then
reached results. A subsequent 12,662-frame accelerated trace exercised all nine
destructible elements through force accumulation, the break delay, collision
removal, animation, the original 323-frame recovery interval, and
collision/material restoration. Several elements also entered the obstructed
restoration state before returning to their intact state.

Onett's yakumono parameter is a contiguous 0x68-byte block of 26 floats. The
real-ISO preflight checks that span, geometry, and all four spawn mappings. Its
startup hazard model exposed another GameCube record-stride assumption: the
original code selected scene descriptor 1 by adding 0x34 bytes to the scene
array, but native scene records contain 64-bit pointers. Selecting array entry
1 directly preserves the same descriptor on both layouts. A fresh accelerated
one-stock match displayed the stage and reached a correct four-player results
screen.

A later native-state pass replaced the traffic controller's packed offsets and
padding with two explicit directional streams, each with its own car index,
state, timer, progress, and speed. The awning physics and destructible-building
controllers now use purpose-based fields and state names, and unused duplicate
joint slots carried over from the GameCube layout were removed. Temporary live
tracing in a 99-stock accelerated match observed repeated left-to-right and
right-to-left car activation and completion, including the rank-delayed path,
as well as both the awning and building collision callbacks. The trace code was
removed after the run; exact event timing and visual motion have not yet been
compared frame by frame with retail.

Pokemon Stadium's base yakumono parameter occupies 0x54 bytes, with five
16-bit fields at offsets 0x48 through 0x50. Its base archive also refers to
joint, animation, material-animation, light, and deferred-object symbols that
live in the four transformation archives. Retail initialization leaves those
externals null until their owning archive is loaded, so the typed importer
preserves that deferred state instead of rejecting the base archive. The four
`GrPs1.dat` through `GrPs4.dat` files are stage fragments: each contains its
own map, collision, item data, and 0x54-byte parameter object while relying on
the base archive for lighting and quake data. The fragment importer now makes
that ownership explicit and real-ISO coverage converts all four archives.

The delayed-load controller no longer sends a transformation buffer through
the legacy in-place DAT relocator. It imports the completed asynchronous read
into native assets and releases the previous typed archive before the next
load. That pass also replaced the controller's offset-named buffer, archive,
timer, type-history, and ground-object fields with purpose-based native fields.
This exposed a remaining GameCube union alias: transformation state 2 cleared
the timer through the jumbotron's offset-0xD8 pointer field. It now clears the
controller timer directly. A 99-stock accelerated run remained responsive for
38,539 simulation frames while the original random scheduler selected every
transformation. Temporary transition tracing captured each of fire, grass,
water, and rock progressing through asynchronous import, announcement,
collapse, incoming-stage growth, activation, its complete live interval, and
the full return to the normal stadium. The transformed geometry, fighters,
collision, jumbotron feed, and variant motion remained active throughout. The
trace code was removed after the run; the real-ISO importer test continues to
cover all four fragment archives.

Big Blue's yakumono parameter is a contiguous 0x144-byte block of 32-bit
values. Its car manager exposed three native-layout assumptions together: a
thirty-pointer array was allocated as 120 bytes, scene entry 2 was selected by
adding two 0x34-byte GameCube records, and the per-car simulation read packed
PowerPC state through absolute `Ground` byte offsets. The native path now sizes
the pointer array from its element type, indexes the typed scene array, and
stores car state, direction, collision slot, and motion data as explicit native
fields. The car update, spawn, collision, and separation paths use those fields
directly. The real-ISO preflight checks the 0x144-byte parameter span,
geometry, spawn mappings, and at least three scene descriptors. All supported
stage preflights also pass with the runtime stage arena reduced from 16 MiB to
4 MiB; their measured high-water mark was below 1.74 MiB. A release-build
accelerated one-stock match rendered Big Blue and reached the correct
four-player results screen. A later native-layout pass separated the platform
manager, flying platform, route platform, moving road, and four car lanes into
their owning native records. It removed the remaining 32-bit item and joint
pointer stores, GameCube byte-offset controller access, and a collision query
that advanced a `Ground*` as though it were a 0x40-byte car-lane record. A
99-stock accelerated observation remained live while three captures taken five
seconds apart showed different F-Zero machines, road segments, and flying
platform positions. The related Big Blue Route car state now keeps its sound
flags separate from the controller pointer that formerly shared the same
four-byte union field. That route update still followed the model sibling list
through a helper that returned the native `HSD_JObj*` as a 32-bit `int`. The
native path now walks the typed `next` link directly. A fresh 99-stock
accelerated run remained live to 87/95/95/96 stocks and captured the moving
road and vehicle platforms after exercising that traversal. The complete event
sequence has not been compared frame by frame with retail.

Venom's Arwing scheduler and controllers previously treated the stage joint
table, three object pointers, three type values, group values, position
offsets, and animation tables as one contiguous `s32` array rooted at the
original GameCube address. Pointer widening changed those offsets on Windows,
and several paths truncated live object pointers back to 32 bits. The native
path now gives each table and runtime field a named type, separates flying
Arwing motion from the controller's linked objects, and deletes the obsolete
packed union view. This let the intended Arwing controller run, exposing that
the archive importer still represented maps 3 and 6 as graph roots even though
the stage indexes their contiguous joint-animation descriptors. The importer
now declares the retail dimensions: six nine-joint groups for map 3 and six
sixteen-joint groups for map 6. The real-ISO test checks the exact indexed
animation and material-animation entries used by the stage. Repeated 99-stock
accelerated runs remained live through the formerly crashing Arwing startup;
captures show the moving Great Fox, changing background view, fighters, and
entry platforms. A later natural scheduler trace covered all eleven flight
types, both the near and far attack groups, all four near attack animations,
both alternating laser mounts, flight-state transitions 0-3, and clean object
retirement. After diagnostics were removed, all 103,310 recorded controller
samples replayed without RNG or fighter-state divergence. This establishes the
complete set of scheduler choices and their active controller paths; exact
motion, attack timing, and presentation have not been compared frame by frame
with retail.

Corneria uses the same joint-indexed archive convention in four scenes. Its
Arwing, background Arwing, smash-taunt, and carrier animation states contain
5, 9, 10, and 16 contiguous joint descriptors respectively. The native import
now declares those source dimensions instead of treating each state as one
graph root. A deterministic replay that formerly crashed while indexing scene
2 completes all 685 samples after the change. The longer replay then exposed a
separate four-byte archive pointer in the Arwing laser's special attributes;
that block now expands its nested `ItemAttr` into a native pointer and decodes
the following floats into a typed structure. The 753-sample laser replay exits
cleanly, and a fresh accelerated one-stock match ran for 46 seconds and reached
results (`screenshots/melee-1789887078-0000.bmp`). The real-ISO preflight
dereferences the exact scene-2 joint and the native laser attributes.

A later natural scheduler trace covered all thirteen Arwing flight types,
groups 1 and 4, all five near attack animations, straight and aimed laser
creation from every mount, flight-state transitions, and clean object
retirement. The Great Fox cannon repeatedly completed its normal states 0-3
with both single shots and bursts of eight through eleven shots. After
diagnostics were removed, all 165,103 recorded controller samples replayed
without RNG or fighter-state divergence. No fighter naturally damaged the
cannon during this run. A debugger-only injection then set the manager to
state 4; execution reached the function-end breakpoint after the untouched
destruction branch hid the intact model, revealed the destroyed model, removed
collision joint 4, and assigned state 5 without a fault. This covers the tail
of the controller, not a natural threshold crossing or the attack callback
that accumulates the retail 200 damage.

An archive-wide scan found the same pointer-bearing special-attribute pattern
in several other stage articles, including Green Greens, Great Bay, Kongo
Jungle, Mushroom Kingdom II, and Icicle Mountain. Their pointee types differ,
so the native importer now decodes each format explicitly: Yoshi's Story Shy
Guys, Green Greens apples, Great Bay's Tingle, Kongo Jungle's Klaptrap,
Mushroom Kingdom II's Birdo and eggs, and Icicle Mountain's Polar Bear. The
real-ISO preflight dereferences each expanded common-attribute pointer and
checks representative mixed-width values from every record. Accelerated live
runs remained active on all six stages, with one-stock results reached on
Yoshi's Story and Mushroom Kingdom II. These runs establish native-safe article
loading; they do not prove every article state or collision outcome.

Icicle Mountain then exposed two independent 64-bit layout assumptions while
cycling its scrolling sections. The generic cleanup routine advanced through
the joint-map table with the GameCube's 12-byte records even though the native
records contain two widened pointers. It now traverses typed map records and
typed joint pairs, and the importer rejects joint destinations outside the
261-entry runtime table. A stage callback also initialized collision-animation
state through the literal GameCube address `Ground + 0x100`; on native that
overwrote four cached joint pointers with the halfwords `0x25`, `0x109`, and
`0x27E`. It now addresses the declared Icicle state directly. A 99-stock
accelerated run survived a full minute of repeated section creation and
destruction after both fixes; the previous access violations occurred within
seconds. The changes preserve the original section and collision logic.

Princess Peach's Castle, Kongo Jungle, Corneria, Venom, and Brinstar now use
typed parameter imports for their source-defined spans. Parameter pointers to
color scripts and environmental-hit records are expanded into native pointers
instead of retaining four-byte GameCube fields. Castle also imports its three
flag dynamics roots, Kongo Jungle updates its two pendulum pairs explicitly
instead of walking `Ground` with a GameCube byte stride, and Venom indexes its
callback table and seven platform joints as native arrays. No-debugger runs
rendered fighters on all five stages. These runs establish scene entry and
continued simulation; they do not establish every stage event.

Kongo Jungle's barrel controller had another three-way GameCube union alias.
The material animation frame was initialized through one view, advanced through
a second, and submitted through a third; all three fields shared source offset
0xE8, but widened pointers separated them on native. A diagnostic run measured
the intended initialized value as 0.5, the advancing value as 1.0, and the
submitted value as 0.0. The controller now owns one named material-animation
field and uses it for initialization, both directions of its fade cycle, and
animation submission. A subsequent accelerated trace kept the value at 0.5 in
the steady state, decreased it to 0.0, waited, and increased it back to 0.5.
The same run repeatedly exercised every barrel swing state and naturally moved
an occupant through capture, hold, launch, and release. The stage scheduler
selected all three event classes, including the Klaptrap carrier, item-box
carrier, and animated stage entries 1, 2, and 7. The temporary trace code was
removed after validation.

Yoshi's Story needed no equivalent layout repair. An accelerated controller
trace followed Randall through repeated complete circuits: across the lower
track, up the right side, back across the upper track, and down the left side.
The moving-collision update remained active throughout. The Shy Guy scheduler
selected all six configured height patterns from both sides of the arena and
created both single and multi-character groups. Since the original scheduler
will not create a new group while any Shy Guy remains, the repeated spawns also
establish that earlier groups completed their routes and left the active item
list. The callback labels, state types, and controller function names now
describe their actual roles; the temporary trace code was removed after
validation.

Jungle Japes likewise retained its original controller data without a native
layout repair. An accelerated trace observed Cranky repeatedly complete states
0 through 3 and return to idle. The Klaptrap controller completed repeated
active passes from varied horizontal positions, returned its article to the
idle state, and selected new randomized delays. Fighters repeatedly crossed
the water surface, exercising the splash path and the stage's `-3.0`
horizontal current. The stage already had a complete one-stock results run.
The parameter fields, controller functions, and runtime state now use
purpose-based names; the temporary trace was removed, and a subsequent
accelerated run remained live after that cleanup.

Brinstar Depths' Kraid controller also completed its original sequence without
a mechanics repair. Accelerated traces repeatedly covered actor states 0
through 4, all three attack animations, several of the six spawn positions,
both hand-strike timing windows, and rotations in both directions. Each rotation
applied the selected delay and per-frame deceleration, updated the stage's
moving collision, and returned the controller to idle. The actor-animation
helper had still described the actor and companion object's first two bytes
through the unrelated rotation-state union member. Those accesses now use
named actor state directly, while rotation timing, angle math, and attack
selection remain unchanged. The temporary trace was removed, and an
accelerated run remained live after the ownership cleanup.

Princess Peach's Castle received a native-layout mechanics pass after its
stage controllers proved to be sharing GameCube union offsets. On GameCube,
the satellite pointers, hazard scheduler, recent-hazard history, and dynamics
records occupied one packed 32-bit layout. Widening only the pointers caused a
later timer write to corrupt the third satellite pointer. The manager now owns
an explicit `grCastle_ManagerState`, and the satellite, hazard, and dynamics
paths all access that typed state directly. The two moving-platform records are
also indexed as a typed array instead of advancing a `Ground*` by the original
record stride. A debugger run completed a one-stock four-player match and
reached results. After restoring the original target lookup, a second
accelerated debugger run simulated for 40 seconds without an access violation,
assertion, or panic. This establishes native-safe controller state and a full
match path. A later trace covered 16,609 accelerated frames: all three satellite
slots advanced through their launch, attack, return, and idle states; four
hazards spawned as entries 7, 5, 2, and 3 while the three-entry recent-history
ring advanced; and both pressure platforms reacted to collision force, entered
their raise and lower states, and moved from their 205-unit resting height as
high as 280 units. This establishes that the scheduler, satellite state
machines, recent-hazard exclusion, collision callbacks, and platform movement
all execute on native. Their exact presentation and timing have not been
compared frame by frame with retail.

Brinstar received a separate mechanics check after its visible objects were
reported as static. Its level-start callback had written state `1` through an
old four-byte union alias; pointer alignment moved that alias on x86-64, leaving
the acid controller in state `0` forever. It now writes the typed level state.
Two native screenshots six seconds apart show the acid moving from below the
platforms to filling most of the arena. A debugger breakpoint confirmed that
the changing acid level reached the original tendril controller and selected
animation group 1. Both breakable-platform hit callbacks now update their typed
damage and velocity fields. A normal CPU attack reached the upper callback with
damage 4; a debugger-only high-damage injection then exercised the original
threshold and material-disable transition. The injection verifies the remainder
of that state path, not the retail time required to break the platform through
ordinary attacks.

Flat Zone exposed another post-pointer native-layout mismatch. The tool-machine
controller owns a dynamic-attribute pointer followed by its state and timer. Its
initializer and update switch used the widened native structure, while the
event trigger and one later transition still wrote those fields through packed
GameCube union views. A pre-fix trace captured the trigger writing state `0` and
timer `130` into the packed view while the native switch instead read state
`130` and timer `0`, so the selected machine event could not begin. All paths
now use a named `grFlatzone_ToolState`; this changes only native storage
ownership and leaves the original state machine intact. An accelerated run then
completed states `0`, `1`, `2`, `3`, and `4`, returned to idle, and began a
second cycle from the opposite side. The scheduler also selected its other
event, which repeatedly spawned falling tools across all five tool kinds and
varied horizontal positions. A clean run with diagnostics removed remained
live. Healing-stage collision state that had reused the same anonymous union
view now has its own typed member rather than retaining a compatibility alias.

This exposed a broader acceptance gap: earlier stable screenshots prove that a
stage rendered and simulated without crashing, but they do not prove its moving
platforms, hazards, destructibles, or scripted transitions. Those mechanics
remain unvalidated unless called out above. Native-layout audits are continuing
for stage-local state that stores pointers or walks fields by GameCube offsets.

Mute City's final startup fault came from the same generic indexed-animation
layout issue seen on other stages. Its map 29 startup applies animation group 1
to joint 6, while the graph-only import exposed just one entry. Importing the
seven joint descriptors in each group lets the original animation call run. A
normal one-stock match remained live for 27 seconds and saved a framebuffer
screenshot with the road section and fighters visible. The car controller also
treated the separate car-order and car-state globals as a single contiguous
GameCube allocation. Its native update now addresses the typed car array
directly. Car particle creation had relied on the PowerPC return register
retaining a nonzero value; the native branch explicitly records whether the
generator and its transform were created instead of returning zero after every
successful call. A later 99-stock accelerated run remained responsive for more
than a minute while the stage advanced through several visibly different road
sections. A subsequent 47,000-frame trace observed repeated car-to-car impacts,
cars entering and leaving their live collision-material window, and player
attacks naturally raising two cars' damage counters. The retail destruction
threshold is 16 damage; a debugger-only injection raised one already-damaged
car from 2 to 17, after which the untouched controller swapped its model,
removed its collision material, decelerated it to a stop, and repeatedly
created and retired the destroyed-car effect as it crossed the camera bounds.
The trace distinguishes the naturally exercised collision and damage paths
from the injected threshold crossing.
The car-child setup had one remaining GameCube-layout dependency: it multiplied
the map index by the original 0x34-byte descriptor stride before reading car
animations. Native descriptors contain widened pointers, so the setup now
indexes the imported descriptor array by type. A subsequent accelerated
99-stock run remained stable across the moving city, tunnel, and outdoor track
sections with all four fighters active.

Pokémon Stadium later exposed a fixed-size allocation left behind in its
jumbotron setup. One `ImageDescWrapper` constructor allocated the original
28-byte GameCube record even though `HSD_ImageDesc` now contains widened native
pointers; initializing the wrapper overran that block and damaged the following
heap cell. The sibling constructor already allocated the typed size, and both
now use `sizeof(ImageDescWrapper)`. A Ganondorf/Zelda control reached gameplay
after the repair. A subsequent very-high-item run with Samus, Mewtwo, Luigi and
Marth recorded 30,162 controller samples while the stage remained live; its
first 10,000 samples replay to their endpoint without divergence or panic. A
post-repair debugger pass over that exact replay reached the asynchronous
transformation callback, imported the completed fragment, and constructed the
incoming ground object. A framebuffer capture requested when the new ground
reached full scale (`screenshots/melee-1789908704-0000.bmp`) shows the fire
transformation with its geometry, smoke, fighters, and active HUD rendered.
