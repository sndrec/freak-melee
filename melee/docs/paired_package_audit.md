# Owned multi-member fighter packages

Read-only proposal, 2026-10-03, audited checkout `c5024498d`. No package,
project, runtime, compiler or player implementation was changed for this audit.
This describes an implementation boundary; it does not claim that Zelda/Sheik
or Popo/Nana moves, partner AI or transfer policy are independent already.

## Required identity model

A selectable fighter is one immutable **family** content package with one CSS
tile, title, portrait and installed marker. It owns one or two **members**.
Every member owns its Definition, generated DLL, editable behavior, FMP schema
and values, CPU profile, fighter DAT, animation DAT and costume DAT. Both members
must load without a stock partner or another installed fighter package.

Use a bounded immutable family mode:

| Mode | Members | Initial player entities |
| --- | --- | --- |
| Single | 1 | Member 0 active |
| AlternateForm | 2 | Member 0 active, member 1 dormant |
| SimultaneousCompanion | 2 | Both members active |

This mode belongs to the package/player relationship, not a disguised stock
`source_kind`. It selects the existing common dormant-versus-companion entity
construction semantics. Actual authored callbacks and companion decisions still
belong to the members. Member 0 is the primary CSS/start member in the first
batch. Cloning a different starting form puts that form in member 0; a mutable
per-match start-member selector can be added when explicitly needed.

Member indices remain immutable. `Player_GetEntityAtIndex` uses transformed
player roles, which swap during alternate-form activation; those roles must not
be used as package member indices. This distinction is necessary for rig,
parameters, module ownership, costume and retained article identity.

## Confirmed current contracts

* `src/melee/gm/native_content_store.h:38-62` has one behavior key and exactly
  one set of asset/module/source/FMP/CPU byte spans. `BuildFighter` and
  `ParseFighter` in `native_content_store.c:242-425` serialize one fighter,
  animation, costume and module, then source/FMP/CPU trailers. The hash covers
  the whole payload. Installed/cache/gift state is already content-ID based.
* `src/melee/ft/native_fighter_project.c:313-435` parses a 128-byte manifest
  with `base`, `title`, optional `code`; it opens fixed root filenames.
  `Build:694` copies these fields into a single package. Import-from-package
  writes the completed manifest last. `native_fighter_project_install.c:65-132`
  rebuilds the draft, compares its exact cache ID and preserves one CSS tile.
* FMP is already appropriately member-local. `native_fighter_parameters.h`
  bounds it to 256 typed fields and 64-KiB text; `Parse` hashes field names and
  types, excluding values. `native_fighter_module.c:1411-1454` rejects active
  DLL reuse with a different schema or values. Identical member parameter names
  therefore do not require a family-wide prefix or merged schema.
* `native_fighter_module.c:1457-1632` binds one base and one static Definition
  per loaded DLL. `OpenCached` writes `modules/<family-content-id>.dll` today.
  It cannot cache two different member DLLs under that same path. A member's
  library must not be rebound to another base or parameter set.
* `native_fighter_runtime.c:54-89` has two banks of four **Definition slots**.
  Each slot owns its package allocation, imported arenas, motion table, rig,
  module and CPU profile. `StageCached:1040` prepares one slot per selected ID;
  `ActivateStaged:1166` binds those slots. This is not a four-family/eight-actor
  capacity yet. `prepare_slot:504` and detached `ValidateCached:974` validate
  only one member. `CostumeInfoCached:988` returns the first matching Definition.
* `native_match_launch.c:445-525` sorts and deduplicates the four selected
  content IDs before assigning custom CSS indices. Repeated use of one family
  by several players needs one imported Definition per member, with separate
  Fighter instance state for each player. Its later direct re-entry path at
  `RegisterFighters:1711` currently also assumes one actor per family.
* `player.c:35-100` already stores primary ID, extra ID and transformation mode.
  Stock Zelda/Sheik mappings select a dormant extra entity; Popo/Nana selects
  an active extra entity. Custom registration currently always stores
  `{fighter, -1, false}`. `Player_80031AD0:227` constructs both declared entities
  and `Player_80031CB0/31D2C` preload both rigs using the same color selector.
* Audio preparation precedes activation. `native_fighter_runtime.c:376`
  prefers the staged CharacterDefinition, but `lbaudio_ax.c:1700`
  `lbAudioAx_80026E84` reads only that Definition's sound-bank mask. A family
  must preload the OR of both members' masks, including the dormant member.
* Article asset/logic resolution in `native_fighter_runtime.c:408-457` is by
  the actual author FighterKind/Definition. Same-family membership must not
  loosen exact immutable-author or creator Definition validation.

## Concrete bounded package and project proposal

Replace the single-member package representation directly. Do not nest two
existing fighter packages, fetch a dependency, add an old-format parser or keep
a second compatibility route. Continue using one SHA-256 family content ID.

Suggested fixed in-memory metadata, with typed mode rather than raw flags:

```c
enum { NATIVE_FIGHTER_FAMILY_MEMBERS = 2,
       NATIVE_FIGHTER_MATCH_FAMILIES = 4,
       NATIVE_FIGHTER_MATCH_MEMBERS = 8 };
typedef enum NativeFighterFamilyMode {
    NativeFighterFamily_Single,
    NativeFighterFamily_AlternateForm,
    NativeFighterFamily_SimultaneousCompanion
} NativeFighterFamilyMode;
typedef struct NativeFighterPackageMember {
    char key[32];
    char code_key[32];
} NativeFighterPackageMember;
typedef struct NativeFighterPackage {
    char title[64];
    NativeFighterFamilyMode mode;
    uint8_t member_count;
    NativeFighterPackageMember members[2];
} NativeFighterPackage;
typedef struct NativeFighterMemberData {
    const uint8_t *fighter_dat; size_t fighter_size;
    const uint8_t *animation_dat; size_t animation_size;
    const uint8_t *costume_dat; size_t costume_size;
    const uint8_t *module_dll; size_t module_size;
    const uint8_t *script_text; size_t script_size;
    const uint8_t *parameter_text; size_t parameter_size;
    const uint8_t *cpu_text; size_t cpu_size;
} NativeFighterMemberData;
typedef struct NativeFighterPackageData {
    const uint8_t* portrait_png;
    size_t portrait_size;
    NativeFighterMemberData members[2];
} NativeFighterPackageData;
```

Member keys are stable authored directory/cache identifiers, restricted to the
existing safe lowercase code-key character set; keys must be unique. Definition
keys are validated against each member's generated Definition as today. Module
cache names use the family hash **and member index**, such as
`modules/<family-hash>-0.dll`, so distinct member libraries remain distinct
even if their bytes happen to match. The ordinal is immutable in the family;
runtime player-role swaps do not rename the library.

Use explicit little-endian wire fields, not C struct/pointer serialization.
A concrete wire header is 80 bytes: existing magic at 0..3, fighter payload
discriminator at 4, mode at 5, member count at 6, zero at 7, uint32 portrait
length at 8, zero at 12..15, title at 16..79. Each of two fixed directory entries
is 92 bytes: key[32], code_key[32], then seven uint32 lengths in the order shown
in `NativeFighterMemberData`. Thus payload starts at byte 264. No new format
version or old-header parser is proposed. Payload order is canonical: portrait,
then member 0's seven chunks,
then member 1's. Inactive directory entries and padding must be zero. Validate
mode/count correspondence, unique safe keys, minimum DAT sizes, module/source
requirements and exact remaining-byte exhaustion with subtraction-based
overflow checks before returning any borrowed views. No unbounded offsets or
recursive parser are needed. Require a valid authored independent DLL for
**each paired member**, so missing secondary behavior cannot silently select a
stock/native behavior fallback. Existing built-in single-member support, if
retained, must be an explicit Single policy only.

A consistent editable project layout avoids a special hidden secondary cache:

```text
project.txt                 # family title/mode, ordered member keys
portrait.png                # one CSS portrait
installed.id                # one family install identity
members/primary/member.txt  # base and definition code key
members/primary/fighter.dat
members/primary/animation.dat
members/primary/costume.dat
members/primary/behavior.fms
members/primary/fighter_parameters.fmp
members/primary/cpu_profile.fcp
members/primary/module.dll
members/secondary/...       # same editable fields, only when count == 2
```

Proposed root manifest grammar is exactly `title=<title>`,
`mode=single|alternate_form|simultaneous_companion`, `members=1|2`, then ordered
`member=<safe-key>` lines, each newline-terminated. `member.txt` contains
`base=<supported-copied-base-name>` and `code=<definition-key>`. The mode/count
and number of member lines must agree; no guessed partner, arbitrary filesystem
path, donor package ID or additional trailing directives are accepted. Each
member directory key is resolved from this manifest only.

Make this the canonical layout for Single too; update real callers and tests
directly rather than autodetecting old and new layouts. The actual project
structure becomes family metadata plus `members[2]`, each containing the old
actor-specific fields. Parse and validate all member records before declaring a
draft openable. Write the root completion manifest last. A member's save/undo
must protect its assets and all other members' untouched input bytes; family
build/install must include both members, even if the editor currently shows one.
CLI/Workshop selects a member by its parsed key (default primary when count 1),
then edits/builds that member's real data. The existing one-Definition compiler
can compile each member separately; no new merged script/state format is needed.

The public store build/parse signatures can retain their current names and
arguments, using the new structs directly. No primary-field aliases are retained.
Project `Open`, `ReadInfo`, `Build`, `CreateFromPackage`, `Install` and `Close`
remain whole-family operations. Change the real project storage to family
`title/mode/member_count` plus `NativeFighterProjectMember members[2]`; move the
old base/root/action/key/assets fields into that member type. `GetFloat`,
`SetFloat`, `Save`, `SaveAnimation`, `SaveCostume` and their undo counterparts
take an explicit `unsigned member_index` after the project/path argument.
Update their actual comparisons and write bodies directly. `CreateStock` takes
mode/count and a bounded array of input records containing each source kind,
the three input DAT spans and CPU profile; `CreateStockFromDisc` takes
mode/count and source-kinds array and imports exactly those declared members.
Existing callers constructing a Single pass one explicit source record.
Parameter import and `script-c`/`script-build` accept `--member <key>` and use
the parsed member's base/root/attributes/FMP/module paths, independently for
each member. The default may select member 0; supplying an unknown key fails.
Workshop inspectors receive the real selected `NativeFighterProjectMember*`,
while family install/preview always validates/builds the entire parent project.

All listed assets are owned by the family. Initially store separate bytes and
import arenas per member, even for duplicate clips. Do not introduce mutable
arena aliases or copy relocated pointers between arenas. Optional immutable
byte deduplication is a later storage optimization, not a requirement for this
family contract. Existing generic common engine assets remain common; another
fighter's article/rig/module is never a donor dependency.

## Runtime transaction and publication

Keep two fixed banks with four family records and eight member records each.
A family record owns the package byte allocation once, its title/mode/count,
CSS index and contiguous member range. Each member owns its current arenas,
motion tables, module and Definition; it borrows immutable payload views only
while its family allocation is alive. Release member arenas/DLL references
before freeing the family bytes. Do not double-free a shared package buffer.

Stage sorted selected **family IDs** as today; assign `ChKind_CustomFirst +
family_index`, and allocate member FighterKinds in contiguous ordinal order.
For four pairs this occupies eight custom FighterKinds, well below the existing
128-kind tables and signed mapping ID limit. Prepare both members against their
own copied base and validate all rig/animation/parameter/CPU/article contracts
before any player mapping or fighter table is published. Preflight and detached
validation must exercise the entire family. Any member failure releases the
whole staged candidate while leaving the active practice bank intact.

Activation binds every member's actual tables, then publishes one custom
player mapping for the family with its primary kind, optional secondary kind
and mode. Split this publication out of today's actor `bind_slot`; two calls
must not overwrite the same CSS mapping. Family Title is deterministic;
Definition lookup remains FighterKind-based. Replace the audio-only
CharacterDefinition query with a substantive family sound-mask query that ORs
all prepared/active members, preferring the staged family before audio loads.

`Load`/ValidatePackage/tool probes and the later direct-match re-entry path
must use the same complete family transaction, not an alternative one-actor
load. Requested detached Definition overrides need an explicit member selector
where tests use them. Family mode does not change per-member sound mute masks,
CPU tables, visibility, omission records, state descriptors or article authors.

Current `release_slot` cancels Definition-owned resources only when the slot is
active. Preserve that rule: closing a detached validation candidate sharing a
loaded DLL is not Definition retirement. At real scene clear, cancel live
resources before retiring definitions/rigs, clear both member tables and the
single family mapping, then close modules. Immutable-member binding must survive
alternate-form swaps; mutable player roles must not retag authored state.

## Costumes, network and fixed limits

The initial family bundle carries one valid costume DAT per member. All custom
members are registered with one costume, and direct custom launch selects color
0 today. Validate both costumes independently; do not synthesize the secondary
costume from the primary rig or a retail partner. Reject external costume
overrides for paired families in the initial batch with an explicit error.
`native_costume_runtime.c` has one override slot per player and
`native_costume_package.c` queries one parent rig; enabling pair skins requires
a later single costume-set payload with two explicitly owned member models.
A mere member-index field would make a primary skin possible but would not
provide a complete selectable paired appearance.

Network room content enumeration already sends the selected character package
ID plus optional costume package ID (`native_netplay_room_content.c:111`).
Keep one family ID; its hash includes mode and every member's bytes. Gift/cache
parsers and CSS portrait readers need the new family header directly, but no
secondary dependency exchange or new lobby slot count is necessary. The sender
must possess the entire valid family. Resource capacity is still global, not
automatically doubled when a family has two actors.

| Limit | Current source / proposal |
| --- | --- |
| Selected unique families | Launch arrays: 4; retain 4 |
| Members per family | Proposed 1 or 2, evidenced by player mapping's single extra ID |
| Runtime member slots | Current 4; proposed 8 per active/staged bank |
| Family payload | Current 64 MiB maximum; retain aggregate ceiling, fail explicitly |
| Module/source/FMP/CPU wire chunks | Current each <= 0xFFFFFF; retain module ceiling initially |
| Authored behavior text | 512 KiB per member, `native_fighter_behavior_source.h` |
| Owned FMP | 256 fields / 64-KiB text per member |
| Fighter/article state | 256 / 512 bytes per instance; no shared family state union |
| Character imports | 2-MiB arena, 4,096 records, 16 item/graph slots per member |
| Costume imports | 512-KiB arena, 2,048 records per member |
| Definition bounds | 1,024 actions/motions, 128 demos, 32 omissions, 64 article logics |
| Article resource pool | 6 live collections x 90 nodes globally; exhaustion remains explicit |

Eight actors reserve twice the current per-bank imported asset capacity;
allocations occur only at staging. Keep existing arena ceilings, account for
both active and staged banks, and measure aggregate memory in the integration
receipt rather than guessing that the 64-MiB compressed payload bounds runtime
arena use. Dormant alternate-form members still consume their imported assets.

## Minimal mutation sets and dependency order

1. **Family data and editable projects:** `gm/native_content_store.h/.c`;
   `ft/native_fighter_project.h/.c`, `native_fighter_project_install.c`;
   `tools/native_content_tool.c`, `tools/native_fighter_parameter_tool.c`.
   Factor substantive member filesystem/parsing work into a dedicated project
   member unit if needed; do not forward old single-member APIs through it.
   Wire-format tests and project round-trip tests change in this batch.
2. **Member library identity and complete runtime staging:**
   `ft/native_fighter_module.h/.c` (`OpenCached` member index);
   `ft/native_fighter_runtime.h/.c` (family/member banks and transactions);
   `gm/native_match_launch.c` (later re-entry and family counts);
   `lb/lbaudio_ax.c` (actual family mask read). `pl/player.h/.c` registration
   gains an explicit extra kind/mode, coordinated with the transfer worker.
   Runtime slot cleanup/tracked global state and module fingerprint review are
   mandatory; package mode need not be duplicated into every Definition.
3. **Direct consumers/editor:**
   `ui/native_shell_workshop.cpp`, `_editor.cpp`, `_parameters.cpp`,
   `_script_files.cpp`, `_model.cpp`, `_bones.cpp`, `_pose.cpp` and corresponding
   headers use the selected actual project member. Audit `_hitboxes.cpp`,
   `_materials.cpp`, `_transform.cpp` for selected project references too.
   `ui/native_shell_collection.cpp`, `native_shell_css_original.cpp`,
   `mn/native_css_original.c`, `gm/native_netplay_room_gift.c` parse family
   title/portrait/member metadata. Their current built-in code-key availability
   check must not reject a valid family on a removed family-wide code key;
   availability must validate its member metadata/bases instead.
   `ft/native_costume_package.c`,
   `native_costume_runtime.c`, `gm/native_home.c` enforce paired-skin rejection.
4. **Owned partner policies/assets:** actual Zelda/Sheik and Popo/Nana copied
   bases, complete templates and import metadata/registry/CMake entries follow
   their own source closure. Existing `StockBaseName` and stock clone registry
   contain none of these four members. Packaging cannot supply missing copied
   bases or make stock-specific calls independent. Mewtwo worker owns current
   player/common transfer analysis and implementation; coordinate immutable
   member identity before publishing registrations. See
   `tests/native/sheik_clone_activation/paired_contract.md`.

No need to change the expression/compiler state layout merely to package two
ordinary modules. Explicit cross-member callback invocation can use recipient
exports; private creator/owner helper calls and retained article references
continue requiring the exact recipient/author Definition.

## Coherent acceptance batch

* Build/parse canonical Single and both two-member modes; reject invalid
  count/mode, repeated member keys, missing secondary chunk, overlapping/truncated
  lengths, invalid portrait, total overflow and mismatched Definition keys.
* Normal clone/import/generate two different owned bases with deliberately
  different rig sizes, state offsets, same FMP names and different values.
  Open both real DLLs; prove stable base/parameters and family hash changes when
  only secondary source/FMP/CPU/assets change. Verify active DLL rebinding
  rejection remains intact and identical active parameters still reuse safely.
* Four paired families stage eight member Definitions; repeated family choices
  deduplicate imports. Broken secondary rig/schema/module fails before any
  mapping, primary table or active scene changes. Detached validation/Close
  cannot cancel a live resource collection from the same active Definition.
* Publish both actor tables before mapping. Assert alternate member sleep vs
  companion active construction, immutable member indices across role swap,
  distinct live state/CPU/article author and aggregate prepared sound masks.
  Actual transfer behavior remains a separately extracted source acceptance.
* Round-trip editable project, select/save/build secondary, undo, export/reopen,
  install/replacement with one CSS tile. Draft/cache identity compares all
  members; incomplete member creation never publishes a completed manifest.
* Gift/cache/network content remains one complete family ID. Paired external
  skins reject explicitly. Scene teardown clears exact actor/resource bindings
  before module/package release; test partial staging failures at every member.

These are console/package/source tests first. Full moves, authored companion AI,
results/death transitions, rendered skins and hidden/muted actual match evidence
remain distinct acceptance boundaries; no such tests were run by this audit.

## Audit receipt

The source pointers above were read from checkout `c5024498d`; selected audit
inputs were clean in `git status`. This document is the only created file.
Source hashes below identify the inspected foundation; subsequent worker edits
must be reviewed separately rather than treated as this receipt.

<!-- source hashes populated below -->

```text
dec0b8276fdce019c263dab5623c31b7337a5463e7ccaf9f6061fab57a355e43  src/melee/gm/native_content_store.h
37941067631a34212d78cca08ba85e57d2eba0ec21d10001ef132f7cf307a6b8  src/melee/gm/native_content_store.c
ef5da99347ddd9d8b0297d3ad67556b638af361395f475c1fad914bc363177f8  src/melee/ft/native_fighter_project.h
35f428359013b1bbc3db6df38aa4b7c8e40134ed4afdd752a5937be58d70579c  src/melee/ft/native_fighter_project.c
db8654e1b6e95b65bd7e7a0af3229ec66a26fff8461f16ce1fa2ea9400d1f836  src/melee/ft/native_fighter_project_install.c
3050831575594bc5cce8d4eb205336eca651f7ce469acc7265accaaa0327ab04  src/melee/ft/native_fighter_module.h
64bf89defeb0cfd1207111216018b671239254df20c532a87d3494b3e82bec62  src/melee/ft/native_fighter_module.c
cddd5d943f66690008cadd8c6a8de251e491fc828a22ae23afedf6dfe870dcb6  src/melee/ft/native_fighter_runtime.c
d0744a4443818107a4c8e3c0e535383ae7d0ad7898e23bea85b14d307e3229aa  src/melee/ft/native_fighter_parameters.h
d0f408187175b5c51f163e404230a12bcea3e5eeb7b3260688c377e5d92f7e7b  src/melee/gm/native_match_launch.c
61ce8e1f2ed54b0138682936acf397650a60b047c2d65e077497f3afad83d20a  src/melee/pl/player.c
b93cc129a1b0ac304b87eaecab26040ec8d5672edb833eebeaca55366a011d2c  src/melee/lb/lbaudio_ax.c
262ffe7ea8e35b7e4b7b1a1ef2af8fc3ddf31cf5495c81bf4a7a67dfe807eef4  src/melee/ft/native_costume_runtime.c
e45c08deccce8d2588da897af15789633d872c2d5c2c72862e1bf992fc7eb9a8  tests/native/sheik_clone_activation/paired_contract.md
```
