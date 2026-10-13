# Native custom-fighter bring-up

## Rook workshop draft

### Authored damage and knockdown animations

The local Rook draft now uses `rookriggednew.blend` for 19 clips that were still
byte-identical to Jigglypuff's: `DamageHi1–3`, `DamageN1–3`, `DamageLw1–3`,
`DamageAir1–3`, all five `DamageFly*` clips, and `DownDamageU/D`. The existing
Rook `DamageFall`, `DownBoundU/D`, `DownWaitU/D`, attacks, and other custom clips
remain unchanged. The separate `SpecialLwR` entry was intentionally left alone
because the user reports her down-special animation already works in game.

The export validates the 50-joint rig and bakes evaluated Blender poses onto
Rook's editable figatree tracks. It samples only the existing clip duration, so
the blend's long 280-frame hold keys do not extend short damage actions or
change their gameplay timing. The action scripts, flags, and fighter behavior
are unchanged. The source blend is untouched. The import is one-shot: it
refuses to overwrite any clip that no longer exactly matches the stock donor.

Reproducible import (run from `melee`, with the game and draft closed):

```powershell
python tools/import_rook_damage_animations.py --project "$env:LOCALAPPDATA\Freak Melee\projects\Rook" --iso '..\Super Smash Bros. Melee (USA) (v1.02).iso' --exports '..\.local\rook-damage-export' --backup-root '..\.local' --prepare
& 'A:\SteamLibrary\steamapps\common\Blender\blender.exe' --background --factory-startup --disable-autoexec 'C:\randomfiles\GodotProj\PFighter\Rook\rookriggednew.blend' --python tools/blender_fighter_animations.py -- --costume "$env:LOCALAPPDATA\Freak Melee\projects\Rook\costume.dat" --joint-symbol PlyRook5K_Share_joint --actions DamageHi1 DamageHi2 DamageHi3 DamageN1 DamageN2 DamageN3 DamageLw1 DamageLw2 DamageLw3 DamageAir1 DamageAir2 DamageAir3 DamageFlyHi DamageFlyN DamageFlyLw DamageFlyTop DamageFlyRoll DownDamageU DownDamageD --frame-counts '..\.local\rook-damage-export\frame-counts.json' --output '..\.local\rook-damage-export'
python tools/import_rook_damage_animations.py --project "$env:LOCALAPPDATA\Freak Melee\projects\Rook" --iso '..\Super Smash Bros. Melee (USA) (v1.02).iso' --exports '..\.local\rook-damage-export' --backup-root '..\.local'
```

The previous draft is backed up at
`.local/rook-before-damage-animations-20260926-025338` in the outer workspace.
The rebuilt bank retained all 207 clip symbols, with exactly those 19 clips
changed; all 327 action symbols, playback flags, and script pointers stayed
unchanged. Package `23e6c42d89463a1e032030a20d933387ee3c5f6acd5b518056ade0366474a1cc`
registered with 285 own-rig clips and no borrowed clips and is installed
locally. This validates loading and registration; visual quality and hit
reaction transitions still need an in-game check.

### Authored grabs, throws, and editable victim poses

Rook's local draft now imports `Catch`, `CatchDash`, `CatchWait`, `CatchAttack`,
`CatchCut`, and all four `Throw*` actions from
`C:/randomfiles/GodotProj/PFighter/Rook/rookthrows.blend`. The source blend is
unchanged. The export verifies the costume's 50-joint order, hierarchy, and
rest transforms, converts the Blender root basis, and samples evaluated local
transforms at integer frames into editable float figatree tracks. TopN remains
engine-owned. Constant channels retain editable endpoints; this is a baked
animation import, not a lossless copy of Blender's Bezier handles.

Release uses the last authored key on ThrowN. Only Jigglypuff's final throw
values are inherited; her extra forward/down-throw attack hits are omitted by
request. Existing Rook grab/pummel events are retained.

| Throw | Action | Release frame | Damage | Angle | Growth | Base knockback |
| --- | --- | --- | --- | --- | --- | --- |
| Forward | 247 | 40 | 5 | 55 | 30 | 100 |
| Back | 248 | 34 | 10 | 135 | 25 | 90 |
| Up | 249 | 41 | 11 | 90 | 25 | 130 |
| Down | 250 | 24 | 2 | 80 | 45 | 100 |

Forward/up throw now enable the DAT's `0x80000000` TransN extraction flag.
Up throw also executes Set grounded / airborne = Airborne (one jump used) at
frame 5, where the authored TransN Y curve first rises above the floor. The
standard airborne throw physics follows the extracted translation, then the
standard throw-ending path falls normally. No global throw physics changed.
The earlier draft kept Puff's forward/up metadata with root extraction off;
the flag alone would still leave up throw grounded.

These controls are exposed in Action Editor:

- **Action playback → Use TransN root motion → Apply playback** toggles the
  extraction flag on the selected action and supports the existing undo/save flow.
- **Add event → Set grounded / airborne** inserts a source-linked transition;
  its inspector offers Grounded, Airborne (one jump used), or Airborne (no jumps
  left). Its timing is controlled by the normal source timer editor.
- **Release throw** is now an identified, selectable timeline event. Select it
  and edit **First execution frame → Apply timer change** to move the release.
- Throw-value setup records have separate timeline lanes by ID. They stay at
  their real setup frame (usually zero); they do not represent an active spatial
  hitbox. The release marker is drawn separately at the damage/release frame.
  Opcode 0 is labeled **End script** in the command list.

`ThrownF`, `ThrownB`, `ThrownHi`, and `ThrownLw` (actions 262–265) are independent
victim clips in this draft, replacing the borrowed `TPurinThrow*` entries.
Search **Thrown** in the Workshop action list, select a clip, and open Animation
to edit its pose/keys. These start as static poses seeded from the stock victim
rig's first authored rotations, mapped by Melee's shared bone identities onto
Rook's skeleton, with Rook's limb lengths and rest transforms for unmapped
joints. They are templates for authoring, not full copies of Puff's moving
victim animations. Melee retargets them onto the actual victim at runtime.
The existing capture attachment uses Rook's ThrowN (joint 48, also mapped as
TransN2). Root-chain translations in the victim clips are zeroed so HipN follows
that attachment instead of adding the stock victim animation's offsets.
The Workshop currently previews the selected victim clip on Rook alone; this
does not add a synchronized two-character throw preview.

Reproducible import tools (run from `melee`, with the game/draft closed):

```powershell
& 'A:\SteamLibrary\steamapps\common\Blender\blender.exe' --background --factory-startup --disable-autoexec 'C:\randomfiles\GodotProj\PFighter\Rook\rookthrows.blend' --python tools/blender_fighter_animations.py -- --costume "$env:LOCALAPPDATA\Freak Melee\projects\Rook\costume.dat" --joint-symbol PlyRook5K_Share_joint --actions Catch CatchDash CatchWait CatchAttack CatchCut ThrowF ThrowB ThrowHi ThrowLw --output '..\.local\rook-throws-export'
python tools/import_rook_throws.py --project "$env:LOCALAPPDATA\Freak Melee\projects\Rook" --iso '..\Super Smash Bros. Melee (USA) (v1.02).iso' --exports '..\.local\rook-throws-export' --backup-root '..\.local'
```

Import explicitly replaces these clips/events, so do not rerun it over later
Workshop edits unless that replacement is intended. It backs up the entire
draft before writing. The pre-import backup is
`.local/rook-before-throws-20260925-224201` in the outer workspace. Release frames
are imported values, not a live link to Blender; subsequent timing edits need
the corresponding release event moved in the action script.

Installed package:
`24d20a877ac0076051dbb2bbe52828e4ff6ffbadd606c39c6fb83a8cf8668d57`.
Validation: 54 samples of exported HipN/ThrowN tracks matched the native FOBJ
interpreter (maximum error below `4e-8`), all 194 unrelated embedded animation
clips remained byte-identical, and package registration passed with 285 own-rig
clips and no borrowed clips. In-game grab attachment, retargeted victim poses,
and release feel still need the user's gameplay check; these data checks do
not establish visual correctness of the complete throw sequence.
The root-motion/UI follow-up passed the native build, stock throw-damage
roundtrip, airborne-value validation, release-timer editing/undo, root-flag
inspection, and package registration. The prior draft is backed up at
`.local/rook-before-root-motion-20260925-225608`. The user confirmed the throws
look correct before this follow-up; airborne travel and the new UI remain for
their in-game check.

### Base assets and script module

The Workshop's Install action replaces the previous installed build of the
same draft. The draft's `installed.id` records which exact package it owns;
this does not group independent drafts by display name or shared code key.
The old package remains cached for peers and existing transfers, while its
installed marker is removed. If the in-game CSS layout has that fighter tile,
the replacement keeps its position and size with the new package ID. Workshop
installs also refresh the live CSS portrait catalog and any local fighter
selection using the replaced ID. The CLI equivalent for a cached build is
`native_content_tool install-project PROJECT_DIRECTORY PACKAGE_ID`; use it for
local draft updates instead of `accept`. `uninstall PACKAGE_ID` removes only
the installed marker and leaves the cache untouched.

The six older local Rook installs were removed after the current draft was
installed. Their markers are backed up in
`.local/rook-install-markers-20260925-231652` under the outer workspace.

Numeric `state.enter(id, frame)` now preserves the requested entry frame, as
MexTK's `ActionStateChange` does. `Fighter_ChangeMotionState` already evaluates
that frame; the compiler previously emitted another animation update afterward.
For Rook's up special this skipped travel frame 1, including initial horizontal
velocity and starting-facing capture. A left-facing start could immediately
reverse and display the wrong orientation. The native Rook fallback had the
same extra update and is corrected too. The move's script and animation data
are unchanged. Named stock-special entry retains its existing startup behavior.

The focused module regression fails with the old installed DLL and passes with
the rebuilt draft DLL: ground/air entry facing left/right and the later 16-frame
reversal are covered. This is a callback-level check, not an in-game visual
confirmation. The draft backup is
`.local/rook-before-up-special-timing-20260925-230920` in the outer workspace.

The local Workshop library now contains a `Rook` draft under the Workshop
project root (`%LOCALAPPDATA%/Freak Melee/projects/Rook`). This is separate from
the installed package store in `%LOCALAPPDATA%/Freak Melee/content`. Its fighter,
animation, and costume data now come from
`A:/melee modding/chessmelee/files/PlRk*.dat`; the portrait remains
`A:/melee modding/Rook/rookCSS.png`. Its native
module was built from the editable sources in
`examples/rook_workshop/fighter_state.fms` and
`examples/rook_workshop/moves/*.fms`. The installed fighter package and draft
both use this compiled module. The neutral special remains unfinished because
Rook's original `specialn.c` has empty callbacks.

The chessmelee asset update replaced the older MexTK exports. All three DATs
differ; ten embedded animation clips changed, including back-air. The old draft
was preserved at `.local/rook-before-chessmelee-20260925-213801` under the outer
workspace. The updated draft passed package/module validation and was installed
as `03060b6765c8d1ad2cf9253a02641ed3789371ae01d0091e4042d9939daee46b`.

The rig-binding fix separates `animation_source_kind` from stock behavior
inheritance in the native fighter definition. Rook's back-air clip and costume
both contain 50 joints, but the DAT tags her own animations as Purin (15).
Leaving that tag unchanged incorrectly invoked Jigglypuff-to-Rook retargeting,
skipping unmapped custom joints and assigning tracks through the donor's map.
Rook now declares Purin as her own-rig animation tag while keeping
`source_kind = Ft_Kind_None`. Runtime import rebinds those entries to her
allocated fighter kind; genuinely borrowed tags remain unchanged. This applies
to both action and demo banks. No animation keys or stock gameplay were changed.

After rebuilding her script module, package
`363ede6ca9f0ee45c2c826f7d4989f74a04201e2789a6991efec9ca29a416013`
registered successfully with 281 own-rig clips and four borrowed clips and was
installed locally. The native build and scripted Mario installed-clone
roundtrip passed. Visual confirmation of the arm, tail, and back-air is pending.
The native definition layout changed, so other existing compiled script modules
must be rebuilt against the current headers before reuse.

### Rook air jump and metal visibility

Rook retains Purin's multi-jump initialization despite allowing only two total
jumps. Without `can_multijump` and the extended-attribute pointer, the ordinary
jump path multiplies her common jump speed 3 by her multiplier 3, producing 9.
Her authored multi-jump impulse is 3 and its start state is 341. The generic
script primitive `fighter.enable_multi_jump()` binds the copied extended
attributes to the engine's multi-jump schema and enables that path. It does not
change the jump limit or tune any attributes. Both the Rook script and native
fallback enable it after copying attributes.

Windows captured the 2026-09-25 21:46 crash in `HSD_DObjSetFlags`, called while
`Fighter_Create` initialized the metal visibility list (`x203C`). The source DAT
contains six metal DObjs but its visibility selection lists indices 0 through
11. The Workshop draft's count at file offset `0x852c` was corrected from 12 to
6; the source archive remains untouched. The prior draft is backed up under
`.local/rook-before-jump-metal-20260925-215408` in the outer workspace.
Character import now rejects out-of-range metal visibility references before
they can read unused pooled DObj pointers. The original Rook source fails that
check and the repaired draft passes; all stock fighter archives still import.

Package `943e867660695f382cc9d6392fedb2979635886c31ef2c9fcf0c6fb8f7180b63`
contains the jump script and visibility repair and is installed locally. Build,
package registration, stock fighter asset checks, and the scripted Mario clone
roundtrip passed. Live gameplay confirmation remains with the user.

Frame-speed modifiers still need investigation before timing parity can be
claimed. Crazy Hand's `MeleeEdit.java` restricts its FSM editor to 20XX ISOs;
`FSMPanel.java` reads an external executable table at DOL file offset `0x4089b0`,
with fighter/frame/action selectors and a float rate. These entries are not in
the three fighter DATs. The extracted `chessmelee/sys/main.dol` contains debug
menu strings at that offset, so this particular table location does not apply
to that executable. This does not establish whether a relocated FSM patch or
another implementation exists there. No rates have been guessed or imported.

The extended script language can target an explicit fighter root and motion
table with `base FighterRoot FirstState StateCount`. It accepts `on load`,
`on death`, `on landing`, special-entry callbacks, and individual motion-state
animation, interrupt, physics, and collision callbacks. Transitions, movement,
input, collision, random numbers, joint rotation, visibility, and on-hit state
changes are expressed through engine primitives. Fighter variables and the
on-hit transition target live in each fighter's rollback-tracked module state.
The root and state bounds are checked when generating and loading the module;
the same syntax is available to other fighter definitions that expose their
motion tables. The current editor still needs gameplay testing of the authored
moves and broader validation of its generic script tools.

The local validation compiled both Rook's source and the existing Mario script,
then packaged the Rook draft and preflighted its assets and native module.
Those checks establish build and load validity, not move-by-move gameplay
parity. Open the Rook draft in Workshop to inspect and edit her scripts, action
events, and animations.

The first authoring target is Rook in `A:/melee modding/Rook/MexTK`. The
reference files stay outside this repository. Native character behavior will
be compiled into the client; PowerPC functions embedded in DAT files will not
be executed. Asset conversion and code registration are separate steps.

## Asset preflight

`native_content_tool` can inspect a fighter DAT, its animation bank, and a
costume DAT without installing content or starting the game:

```powershell
build/native/native_content_tool.exe inspect-fighter "A:\melee modding\Rook\MexTK\PlRk.dat" ftDataRook 327 14 1 "A:\melee modding\Rook\MexTK\PlRkAJ.dat"
build/native/native_content_tool.exe inspect-costume "A:\melee modding\Rook\MexTK\PlRkNr.dat" PlyRook5K_Share_joint PlyRook5K_Share_matanim_joint
```

The first command imported 327 actions, 14 demo entries, 444 character
records, 1,793 action instructions, and 272 animation references. The bank's
largest clip has 438 tracks. The second command imported 599 model/material
records into 175,912 arena bytes. These are import/preflight results, not
evidence of a playable Rook.

The three DATs and a CSS portrait PNG can be validated together and cached as
one content-addressed fighter bundle:

```powershell
build/native/native_content_tool.exe bundle-fighter "A:\melee modding\Rook\MexTK\PlRk.dat" "A:\melee modding\Rook\MexTK\PlRkAJ.dat" "A:\melee modding\Rook\MexTK\PlRkNr.dat" "A:\melee modding\Rook\rookCSS.png" rook Rook ftDataRook 327 14 PlyRook5K_Share_joint PlyRook5K_Share_matanim_joint
```

The real Rook bundle imported all three graphs and cached as
`4fc6fb3ec1bfda9ebd2d939142d3a7e26389794b883ae6ec2d0beabf31257cc3`.
Its package includes the three original-format DATs, the 64×56 CSS portrait,
and the stable `rook` code
key; no executable code comes from the DATs. Cache reads verify the package
structure and SHA-256 identity. `accept` now recognizes bundles with compiled
native behavior, imports all three asset graphs against that definition, and
marks the verified bundle installed. The local direct-match command can now
select an installed Rook by package ID. The original home CSS now selects it too.
Bundles without compiled behavior remain cached. The costume
package path remains unchanged.

The portrait is part of the same package hash and transfers with the fighter.
The bundle parser checks its PNG header, dimensions, and terminal chunk. At CSS
entry Windows Imaging decodes installed portraits; Aurora composites Rook's
portrait into the original 4:3 menu image. The in-place editor can add, drag,
resize, and remove it. Melee's controller cursor now selects the custom
portrait, and the home roster resolves its installed package before practice.
A live run selected Rook, entered practice, moved Rook, and returned to CSS
with the choice retained. Online match content agreement is now exercised by
the room run documented below.
The earlier three-DAT-only Rook package ID is obsolete in this development
build. An isolated install of the new bundle entered Battlefield and reached
the one-stock results screen.

The bundle command also probes the native match-table registration path. It
rejects a mismatched compiled code key, imports the cached Rook bundle into
custom fighter/character slot 34, binds its animation and costume data to the
existing Melee tables, and exercises the ordinary preload calls without an ISO
lookup for those three DATs. It rejects a duplicate registration, then clears
the table entries and restores the unregistered character mapping. For the
`rook` code key, the probe now binds the compiled 12-state Rook definition and
checks its special-move callback table. This still does not start a match.

Rook's animation table contains stale offsets and lengths beginning at action
68. That action points to byte 420,608 with a recorded length of 14,544, while
the embedded DAT declares 14,584 bytes. The next action points before its
named DAT, and later entries drift further. The native animation importer
checks the indexed DAT and root symbol first. If either is invalid, it scans
the bank's embedded DATs once and resolves the action by its named root. It
rejects a missing or ambiguous symbol. A stock Mario animation test shifts a
real table offset and verifies that import restores it by symbol; the complete
stock animation-bank preflight still passes.

## Runtime work still required

`Ft_Kind_None` remains the original value 33. The native build now separates
the 33 stock entries from a 128-entry fighter runtime capacity; values 34-127
are reserved for custom fighters without changing the original sentinel.
Fighter data/callback arrays and Kirby copy storage have that capacity. The
native character-to-fighter mapping likewise reserves character IDs 34-127
while keeping `ChKind_None` at 33. Stock-only selectors, events, and audio
tables still use the stock count. The 359 native tests pass and a fresh client
boot reached the original CSS after this separation. Custom fighter selection
is available through the local direct-match command, and registration now runs
after the fighter-system reset before match preload. The
original CSS owns 25 stock portrait joints; installed portrait images now share
that canvas. Home CSS and practice now resolve an installed package to a
registered native fighter. Room selection carries its package identity,
and each match peer resolves that identity against a compiled definition.

Rook's source supplies several native-portable callback bodies, but
`rookmain.c` also contains hard-coded PowerPC callback addresses and an
included copied common move table. `specialn.c` has empty callbacks. The
native definition must name and register its own moves and state, call the
shared decomp functions directly, and leave unfinished behavior visible until
it is authored. Its mutable per-fighter state must live in rollback-tracked
fighter state, with no gameplay-relevant standalone globals.

The native definition now declares the number of character-specific motion
states and rejects animation IDs outside its imported action table. Rook's
`move_logic` has 12 rows, so its contiguous character-specific states begin at
341 and end at 352. Its sixth and seventh comments say 365/366 because those
rows copied Jigglypuff's up-special callbacks; the rows themselves occupy
346/347. The copied callbacks contain transitions targeting Jigglypuff's own
state IDs, so mapping their PowerPC addresses directly to native function
pointers would be unsafe. The first compiled Rook definition now maps all 12
states by their actual table indices, ports the source's side, up, and down
special transition and movement logic, and stores its crime counter and move
variables inside `Fighter`, which the rollback snapshot covers. A native
landing callback replenishes the counter on the actual air-to-ground edge,
in addition to load and death. It replaces
the copied Jigglypuff callbacks with Rook-owned safe transitions. Since the
MexTK neutral-special callbacks are empty, its temporary native callbacks
use ordinary animation-end, physics, and collision behavior; the intended
effect is still unknown. The remaining gaps include live animation-script and
hitbox inspection, verifying Rook's joint rotation and land/ledge behavior,
and a complete neutral special. The local direct match has now exercised Rook
in gameplay and through a one-stock result, but it has not validated move
parity or a full CPU move set.

## Registration boundary

`Fighter_800679B0` resets imported fighter and costume pointers through
`ft_800852B0` during fighter-system initialization. `Player_80036DA4` now
clears any previous custom registration immediately before that initialization.
A match must therefore resolve installed custom content and populate the
selected native definitions after the reset, before Melee preloads or creates
those fighters. The
definition needs the character-to-fighter mapping, action/demo counts,
costume list and names, base and animation DAT imports, FX choice, and the
appropriate callbacks and motion states. `ftData_800855C8` currently consults
the costume names and FX ID during preload; `ftData_8008572C`,
`ftData_80085820`, and `ftData_80085A14` otherwise read their files from the
ISO. Registered package assets must enter those existing tables with owned
lifetimes so the ISO path is never asked for a custom DAT.

The store recognizes a separate typed fighter bundle. Local direct-match,
home CSS practice, and room-match selection are live. Match peers
resolve the same package identity to a
native definition compiled into their matching client build. A missing
definition must reject the selection before scene preload rather than reaching
the uninitialized custom-kind table entries. Additional costume DATs will need
dependencies on that fighter bundle; the initial bundle carries one base
costume.

## Local direct-match checkpoint

An accepted, installed Rook bundle can be selected with `--p1 human:custom`
and `--p1-fighter 4fc6fb3ec1bfda9ebd2d939142d3a7e26389794b883ae6ec2d0beabf31257cc3`.
The same syntax works for a CPU slot. Unique package IDs receive stable,
sorted custom character/fighter slots for that match. Admission checks the
installed bundle and compiled native definition before entering the game.
A room guest receives and caches a selected bundle before match startup;
a local player still needs to install it to select it.

An isolated two-client room run selected Rook on the host's original CSS and
Falco on a guest with no installed Rook. Both entered practice, joined one
room, selected Battlefield, rendered the same Rook-versus-Falco match, ran
through the two-minute round and sudden death to the results screen, and
returned to CSS in the same room. The guest's content store contains Rook's
1,801,640-byte package only under `cache`, with no installed marker, and its
CSS does not offer Rook. The peers agreed on RNG seed `1bfc65b5` and compared
7,436/7,437 confirmed checksums in the timed round and 1,613 on each side
before the sudden-death result, with no reported mismatch. The first setup
attempt had transferred the bundle but failed because practice still owned
Rook's runtime slot; match adoption now clears the previous scene's custom
registration before validating and registering the roster. A second run began
with an empty guest content root; the guest received the bundle into `cache`
and its SHA-256 matched the package ID before both clients entered the same
Battlefield match. This is a
same-machine test of one custom fighter and one stock fighter; it does not
prove cross-network transfer, multiple custom kinds, rollback correction during
Rook gameplay, or full move parity.

The Rook costume imports 50 joints. Its native definition supplies the
fighter-part mapping, and its load callback copies character-specific
attributes into the fighter. PlCo.dat has CPU attacks for stock fighters only;
the native CPU table now covers custom slots with empty, terminated attack
lists. This prevents an out-of-bounds lookup but means CPU Rook does not yet
have an authored offensive move policy.

Rook's original `OnDeath` sets model visibility groups 0 and 1 to their base
variant. The native callback now does the same on spawn and rebirth. Before
that correction only his ungrouped sword and small accessories drew; a live
practice capture now shows the body, sword, and Sandbag together. Full costume
and animation parity still needs visual checks.

Live Battlefield runs have reached gameplay and a completed one-stock result
with human and CPU Rook. The results scene now tolerates a fighter with no
stock emblem or result-motion actor. Its stock result archive has fixed fighter
names, so the native shell covers only the wrong title and player name with
the installed package title. A full-window capture verifies "Rook" in both
places while the untouched Zelda panel remains correct. Victory poses, CPU
attacks, and full visual and move parity still need implementation and checks.
The game-only screenshot command does not capture the shell overlay.
The native trophy-display pointer table also had a 32-bit element-count
assumption that cleared the match-end header on scene exit; fixing its native
pointer count restored the correct Stock Battle result for stock and custom
matches.
