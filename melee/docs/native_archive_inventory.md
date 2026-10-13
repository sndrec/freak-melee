# Versus archive preflight

The compiler cannot discover missing DAT objects: `PlCo.dat` is a serialized
pointer table, and the runtime only touches each target when a corresponding
gameplay path runs. Use `tools/audit_native_dat.py` against the supplied ISO to
list every immediate target and its nested pointer count before converting an
archive. This records the archive schema in one pass; a live match is an
integration check after the group has a tested native importer.

For the retail USA 1.02 ISO:

```powershell
python tools/audit_native_dat.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --archive PlCo.dat --symbol ftLoadCommonData --pointers 23
python tools/audit_native_dat.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso' --archive LbBf.dat --symbol lbBgFlashColAnimData --pointers 16 --stride 8
python tools/audit_native_color_scripts.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso'
python tools/audit_native_action_scripts.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso'
python tools/audit_native_item_scripts.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso'
python tools/audit_native_stage_item_scripts.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso'
python tools/audit_native_cpu_data.py --iso 'C:\gayms\freak-melee\Super Smash Bros. Melee (USA) (v1.02).iso'
```

`Fighter_LoadCommonData` binds all 23 entries in the original path. The table
below compares those entries to the native branch after the 2026-09-14 typed
imports. `root[i]` means the pointer in `ftLoadCommonData` at index `i`; a raw
span is only a boundary clue, not a decoded type.

| Root | Game use | Native status |
| --- | --- | --- |
| 0 | Common gameplay constants | Imported as typed `ftCommonData` |
| 1 | Item-throw attributes, 78 floats | Imported |
| 2 | Swing multipliers, six groups of five floats | Imported |
| 3 | Stale-move penalties, nine floats | Imported |
| 4 | Per-fighter joint/part tables | Imported |
| 5 | Per-fighter part omissions | Imported |
| 6 | 123 fighter color-overlay entries and their scripts | Imported as native four-byte commands; complete callback behavior still needs gameplay validation |
| 7 | Six special color-overlay entries and their scripts | Imported as native four-byte commands; complete callback behavior still needs gameplay validation |
| 8 | Respawn platform joint and animation | Imported |
| 9 | Three damage-shift vector sequences (4, 4, 5) | Imported |
| 10 | Grab-mash shake offsets and count | Imported |
| 11 | Smash-charge shake offsets and count | Imported |
| 12–15 | Scale, bunny, metal and Zako modifiers | Imported |
| 16 | Fighter entry platform | Imported |
| 17–19 | Three color tables | Imported |
| 20 | Metal fighter model | Imported |
| 21 | Crowd sound configuration | Imported |
| 22 | CPU behavior tables and command scripts | Imported and bound as native CPU data; ground, air, ranged and held-weapon selections have executed in a live match |

The CPU root preflight now checks all ten live pointer fields in its 48-byte
descriptor, including two trailing zero words. It finds 61 byte-command
scripts (1,158 bytes, 652 commands), seven tables of 32 fighter-specific
attack arrays (1,136 attack records), 32 distance thresholds and six reach
values. Each command script terminates within its source span and each attack
array has a terminator within 32 records. `ftCpuData_Import` now copies the byte
scripts, decodes all attack records and scalars, and binds the typed descriptor
in `Fighter_LoadCommonData`. The real-ISO native test matches the complete
counts, compares an attack entry with the source archive, and runs the existing
CPU command copier against an imported script. The same test now independently
compares all 1,158 imported script bytes, all 652 command boundaries, every
field of all 1,136 attack records and their 224 terminators, all 32 distance
thresholds and all six weapon-reach values directly with the big-endian retail
source. A temporary runtime trace in a level-nine Samus, Ness and Mewtwo match
then observed successful selections from the ground, air, ranged/projectile and
held-weapon families. Smash, special-item and edge-guard selection were not
observed in that run, so this establishes live use of four families rather than
complete CPU behavior parity.

The numeric, respawn and color import is covered by the real-ISO
`native_fighter_common_assets` test, including dimensions, nested pointers and
truncated input. The entry-animation color script (entry 117) runs to its
terminator, and a callback command's native fields are checked against its
source word. All 15 retail `LbBf.dat` flash scripts also run to termination in
the native test. This does not establish that a match is playable. The last live
run entered the first Versus simulation frame and stopped in
`ftCo_800BFFD0`: root 6 was unset when the entry animation selected color
overlay 117. That root is now supplied. The action-script import has since been
converted in a batch, as have common and fighter item scripts. Other known
gaps remain before another live integration run.

The script work was larger than filling two table pointers. The retail color
tables contain 129 slots (127 non-null), and scripts include loops, branches,
timers, colors and fighter callbacks. `ftCharacterAssets_Import` now imports
fighter action commands into one shared four-byte native buffer per archive.
It validates the reachable instruction graph, repacks PowerPC bitfields,
converts multiword payloads, and relocates subroutine/goto operands relative
to the native command word. The color runner casts `ColorOverlay*` to
`CommandInfo*` for commands 0–9; its current native layout is exercised by the
flash scripts, while fighter callbacks require further execution tests. A
coherent script port must cover fighter, item and stage callers before gameplay
acceptance.

The color-script preflight now walks all 127 non-null `PlCo.dat` entries plus
15 `LbBf.dat` entries, following subroutine/goto relocations and checking
opcode widths, payload overlap and target bounds. It found 1,315 distinct
instruction starts with opcodes 0, 3–7, 10–19 and 21–23. This establishes
what the color importer must handle; the dynamic tests above cover only the
specified scripts and branches.

The same preflight now covers `ItCo.usd` root 5: seven color entries, six
non-null scripts and 55 instruction starts. The item-common importer converts
the complete table and its 77-word backing command span with the shared color
script importer. The ISO test checks the public table binding and metadata;
match-time item color changes remain to be verified.

Fighter archives contain 21 article special-attribute blocks with serialized
references. Typed imports already covered the Link/Young Link projectiles,
Ice Climbers string, Ness yo-yo, Sheik chain, and ten Game & Watch articles.
The remaining block was Samus's grapple beam: its four segment-model joints
and fifteen animation pointer slots had previously been copied as 32-bit raw
words. A CPU Samus dash grab reproduced the resulting invalid joint pointer in
`HSD_JObjLoadJoint`. The importer now decodes the complete 0xB0-byte block,
including pointer-to-pointer animation roots. The original 1,592-sample crash
recording reaches its endpoint and exits normally after the correction.

`tools/audit_native_action_scripts.py` now checks all 33 fighter character
archives. It finds 10,658 wait states, 9,994 non-null action scripts and
64,174 distinct instruction starts. Opcodes and multiword payload boundaries
validate against the existing `ftAction_803C0870` width table. One Pichu script
has a null, unrelocated goto, which the native branch representation reserves
`INT32_MIN` for. `native_fighter_character_assets` imports all 33 archives and
independently matches those entry and instruction totals, checks every wait
entry's native command pointer and first opcode, and probes the exceptional
hitbox lookahead bit mapping. The original executable confirms the hitbox
handler reads byte 1 of the *following* word; the native handler preserves
that behavior even when the following opcode has a different field layout.
Opcode 55's sound and graphics reinterpretations likewise use decoded fields.
The test now also follows the independently decoded action graph and compares
all fields of all 6,655 reachable fighter hitbox commands directly with their
five big-endian retail source words. This covers hitbox IDs, groups, bones,
damage, size, signed offsets, knockback, interaction flags, elements, shield
damage, sound and ground/air masks rather than sampling a few commands.

A focused native match then executed Falco's up tilt, Luigi's down smash and
Sheik's up smash through virtual controller input. Falco created both up-tilt
hitboxes. Luigi created both front and rear down-smash pairs, and Sheik created
the early and late up-smash sets. Temporary traces of their world-space
capsules stayed near the owning fighters and advanced continuously with the
animated bones. This directly retests the three moves that previously exposed
missing or long-range hitboxes. It does not prove that every move is correct or
that the earlier intermittent long-range symptom cannot recur elsewhere; a
captured replay remains necessary if another occurrence is observed.

The native stage loader now accepts `GrNBa.dat` (Battlefield) and `GrNLa.dat`
(Final Destination). Final Destination's contiguous animation descriptors are
imported with the joint counts used by its stage callbacks; its background
scene has advanced through several animation states in a live match. The
remaining stage archive families still need typed imports and corpus tests.
Successful top-level DAT import alone does not establish stage behavior.

`tools/audit_native_item_scripts.py` now walks `ItCo.usd` and all 33 fighter
archives before import. The common bank has 98 articles, 226 states and 152
local scripts. The fighter banks have 116 local scripts; structural candidate
article counts include a few non-article slots, so the native article masks
remain authoritative for the 87 actually imported fighter articles. Across
the two sources, 1,046 distinct item command starts validate, including branch
targets and multiword payload boundaries. Opcode 16 uses its three-word form
with subopcode 0 or 2 in this retail corpus. `itCommandData_Import` now
converts those reachable commands into one shared four-byte native buffer per
archive, repacks the fields read by item handlers, preserves the byte payloads
used by hitboxes and extended commands, and relocates branch operands. The
common importer supplies all 152 script pointers and 744 instruction starts;
the 33 fighter importers supply another 116 pointers and 302 starts. The native
tests match those totals against the independent ISO audit and check retail
GFX, hitbox, branch, and extended-command examples. The common importer also
rejects any known item whose special-attribute span still contains an untyped
relocation. Typed imports now cover Food, both Mushrooms, Warp Star, the
`it_2E5A` three-tier item, the four stage monsters, and both Unown articles.
This matters on 64-bit hosts: copying the original four-byte graph pointers
into native structures made adjacent Unown pointers combine into invalid
addresses. The ISO test checks all 26 model variants in each Unown article,
the three `it_2E5A` model/animation tiers, and the nested monster-common
attributes including mixed-width big-endian fields.

Runtime coverage remains narrower than the corpus checks. A deterministic
666-sample Green Greens replay that previously failed in `JObjLoadJoint` when
a Poké Ball summoned Unown now reaches replay end, and a fresh item-heavy match
ran for two minutes beyond that point. This verifies that failure and a useful
stress interval; it does not establish every item's behavior. Stage-supplied
item scripts and dynamically assigned stage item data have a separate native
import path described below.

The common `ItemAttr` prefix also contains two bytes of PowerPC bitfields.
Copying those bytes into MSVC bitfields reversed their semantic positions: a
Proximity Mine reported action kind 2 (swingable) instead of 0 (throwable), so
Pichu reached the original swing routine with an illegal item kind. The typed
importer now decodes all nine fields from their serialized masks. The real-ISO
test compares those fields with all 97 non-null common, stage/character and
Pokémon article attributes and pins known throwable, swingable and shootable
items. A fresh high-item-rate Yoshi's Story run with Falco, Pichu, Roy and
Kirby remained live for 40,106 recorded controller samples. Its first 2,000
samples replay to the endpoint with matching RNG and fighter state, passing
the original sample-927 crash window. The pre-fix recording diverges earlier,
as expected when corrected item semantics change the simulation.

Yoshi exposed a separate host-layout assumption during fighter creation. The
original shield setup treated two adjacent four-byte `TempS` records as an
anonymous packed structure. A native `TempS` is wider because its index pointer
is eight bytes, so that reinterpretation read pointer padding as the second
variant's count and index address. The Yoshi code now reads the second variant
through the existing `FtPartsVisLookup` and `TempS` types, and its four shield
material helpers have names that describe their behavior. The real-ISO fighter
test verifies both visibility groups for every Yoshi costume. A fresh
Sheik/Pikachu/Yoshi/Jigglypuff Battlefield run remained live for 4,478
controller samples, and a 2,000-sample slice replays to its endpoint without
divergence.

That same lineup with items enabled then reproduced a Wobbuffet hit of exactly
`2^32` damage at sample 6,812. Wobbuffet deliberately lets its floating damage
accumulator decay below zero and casts it to unsigned; the original Metrowerks
runtime saturates negative values to zero and values at or above `2^32` to
`UINT32_MAX`, while an unchecked Clang/MSVC conversion produced
`UINT32_MAX` for the negative value. The original executable has 93 calls to
that saturating conversion across gameplay, effects, GX and audio. The native
game target now enables Clang's defined saturating float-cast semantics for the
whole decompiled source set instead of changing Wobbuffet's logic. A focused
test pins float and double boundary behavior. The exact 6,812-sample failure
recording now reaches its endpoint, and a fresh 20,129-sample recording,
including the reset transition, replays without divergence or panic. All 45
native tests pass.

The stage item preflight walks all 71 `Gr*.dat` archives on the retail disc.
It finds 53 stage item articles, 62 local item-state script entries and 82
`ALDYakuAll` entries, reaching 269 distinct item command starts. The first
`ALDYakuAll` slot is deliberately null: Battlefield has a live command script
at slot 1. The stage importer now converts `itemdata` records and their article
graphs, plus the `ALDYakuAll` scripts, using the shared native item command
importer. The real-ISO stage test confirms Battlefield's script pointer and
hitbox field, and Green Greens' two item articles plus seven script entries.
The audit supplies the batch coverage target for extending stage imports; it
does not claim all 71 stages can run natively.

This inventory deliberately does not label an unconverted pointer as
"imported" just because its bytes are accessible. Native code needs its
actual typed object graph or bytecode semantics before using it.

Kirby's copy-ability family now has a typed native import path. The importer
normalizes all 25 retail `PlKbCp*.dat` roots named by `ftKb_Init_803CA9D0`
instead of treating their ten byte sizes and 22 relocation signatures as one
fixed five-pointer structure. It imports the hat or merged part model,
visibility and texture tables, dynamics, joints, Yoshi capture animations,
Game & Watch colors, and 28 item articles. The item articles contain 49 script
entries and 113 decoded instruction starts. Link's arrow attributes, Game &
Watch Chef visibility data, and Yoshi's capture-egg joint are typed according
to their actual referenced data rather than copied as host pointers.

`native_kirby_copy_data_inventory` retains the independent root-layout guard;
`native_kirby_copy_assets` imports the complete retail corpus and locks the
article/script/instruction totals above. Runtime loading now uses this importer
and the existing typed costume importer, with imported storage retained for
the scene lifetime. The original shifted `x0`/`hats` pointer table remains the
gameplay lookup contract, but its initialization now clears native pointer
widths directly. A native Battlefield run then selected Kirby, inhaled a human
Mario through the named-pipe controller, rendered the Mario copy hat, and fired
the copied fireball without a panic. Screenshots
`screenshots/melee-1789902914-0007.bmp` and
`screenshots/melee-1789902929-0008.bmp` capture the acquired hat and fireball.
This covers one copied fighter and article; the corpus test proves conversion
of the other archives but not the runtime behavior of every copied move.

Game & Watch copy acquisition also has live coverage. Its special visibility
record contains one serialized lookup but is shared with both the one-model
copy parts and Kirby's two-model body state. On GameCube the next relocated
address has its high bit set, so an accidental read as the second signed count
is empty. The importer now represents that effective second entry as an
explicit zero-count native lookup rather than depending on 32-bit address
bits. `native_kirby_copy_assets` checks the seven retail draw indices, the
empty body sentinel, and all three color words. The complete 25-archive corpus
and all 44 native tests pass. A 6,479-sample Game & Watch swallow recording
reaches its endpoint without divergence, and a fresh run visibly acquires the
black copy form; this still does not establish every copy ability or costume.
