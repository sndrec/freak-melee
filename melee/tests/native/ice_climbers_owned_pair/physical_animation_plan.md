# Self-contained Nana animation materialization (read-only feasibility)

Root direction: physically copy required primary descriptors/clip blobs into the
secondary's own files during package creation/clone construction. No runtime donor
ID or peer fallback. No writer/core/source assets changed by this audit.

## Measured retail inputs

`physical_bank_feasibility.json` pins every selected entry's name,size,flags,command
pointer,blend bytes and exact source clip SHA. Actual retail PlNn xC count321 contains
eight named clips (313..320),313 absent names. Of those holes260 have a populated
Popo animation descriptor,53 are empty in both. The260 entries use199 distinct
PopoAJ blobs, all present in its200-symbol bank. Preserve Nana's8 blobs, giving207
symbols if their names remain disjoint. Physical nodes:256 entries use49; four
262..265 use52-node Taro thrown clips with source rig tag33.

## Existing writer path

`tools/fighter_animation_io.py` has substantive offline `Dat` relocation/string/data
editing, `animation_bank(raw)` indexing and `rebuild_bank(fighter,root,clips,321,14)`.
Existing Rook animation importers already use that writer to produce fighter.dat and
animation.dat, verify output symbol bytes and retain backups. Existing native action
metadata editor writes animation flags only; it does not insert missing names or
merge a missing bank, so those CLI setters alone do not close this gap.

Recommended construction sequence (not implemented):

1. Decode both original fighter tables/animation banks independently. Determine the
   fill set by Nana x8==0 / source primary x8!=0, matching ftAnimationBank_Import's
   eventual NULL clip decision. Keep eight Nana entries and53 common holes intact.
2. For each fill entry, append/deduplicate the original primary symbol string into
   Nana DAT data and add its relocation via Dat.pointer(at,newStringOffset). Select
   the original unmodified primary clip blob in the new owned bank dictionary.
3. Publish that entry's temporary nonzero source size to permit rebuild_bank to
   process it. Rebuild writes the new bank offset and exact member size, including
   32-byte bank padding. Serialize fighter bytes with updated relocation/header
   tables and preserve root symbols. Do not serialize any native x14 pointer.
4. Import the resulting owned fighter DAT and owned combined bank through the actual
   ftCharacterAssets_Import/ftAnimationBank_Import pipeline, checking all321 entries
   plus demo14 separately. Bank lookup indexes names if offset resolution fails;
   reject duplicate names. Import enforces1024-symbol cap,4096 scratch tracks and
   2MiB decoded arena; the extended bank must pass those real bounds rather than
   assume its small8-clip historical allocation is sufficient.
5. Verify all selected decoded clips, availability/loop decisions and flags against
   actual original Nana+Popo resolution, with no global donor definition present.
   Preserve raw rig source tags where necessary for copied Taro clips; physically
   copied animation data does not mean that a source rig tag is a behavior donor.

## Critical field distinction

Do NOT overwrite each complete24-byte Nana descriptor with its primary counterpart.
Fighter_ChangeMotionState reads OWN x24[anim].x10 flags and xC command graph, and OWN
x28 two blend bytes, before the clip lookup. ftCo_8008A6D8 similarly uses its own
script/flags. ftData_80085FD4 returns a primary descriptor for the subsequent shared
clip/availability/loop consumers; those two paths are intentionally different.

256 selected flags differ only serialized source tag10/11 (Popo/Nana). Four Taro
thrown entries262..265 are primary0x80000021 versus Nana0x00000021. Keep Nana's
exact flags, commands and blend fields. All260 pairs have identical serialized
loop bit0x40000000 and blend bytes. Animation name/offset/size/clip availability can
be materialized without changing command policy or observed shared loop decisions.

## Serialized versus native flag layout and all direct consumers

The rig tag occupies SIX bits, not26. The retail big-endian serialized word has
tag mask0x0000003F. Native Fighter.x597_bits occupies high six bits0xFC000000;
the remaining native action/animation fields occupy low26bits0x03FFFFFF.
ftcharacter_assets.c import_wait_table decodes fields explicitly; byte swapping
alone does not convert the PowerPC bitfield order. Its conversions are:

* serialized bits31..24 -> native x594_b0..b7 at bits0..7;
* serialized bits6..8 -> native x596_bone at bits8..10;
* serialized bits9..21 -> native x594_bits at bits11..23;
* serialized low6 -> native x597_bits at bits26..31.

The two unused native bits24/25 remain zero through this importer. Preserve the
serialized source descriptor rather than write a native-layout word into a DAT.

For each Taro entry, primary0x80000021 decodes to0x84000001, while Nana0x00000021
decodes to0x84000000: BOTH have source rig33; only x594_b0 differs. This flag
participates in translation-motion reset/velocity publication in
Fighter_ChangeMotionState and ftCommon_8007D6A4. It is not a rig-tag difference.
Preserving Nana's false b0 is required even though its clip comes from the primary.
Ordinary serialized tag10 decodes to0x28000000, tag11 to0x2C000000 (plus the same
other fields). Keep Nana tag11 for these copied49-node clips, as actual stock
animation installation does; do not copy primary tag10 just because the blob name
starts PlyPopo.

The complete direct x10_animCurrFlags/x597_bits read census is:

| Source | Actual field/policy |
|---|---|
| fighter.c Fighter_ChangeMotionState | Own selected donor-or-self descriptor flags, command graph and own blend bytes; ordinary self path uses Nana flags even when ftData resolves the primary clip. |
| ftwaitanim.c ftCo_8008A6D8 and inline variant | Own flags/command graph/blend; fallback supplies only clip resolution. |
| ftanim.c ftAnim_8006EDD0 / 8006EED4 | Resolved descriptor native bit1 loop policy. The260 materialized pairs have equal loop flags. |
| ftanim.c ftAnim_8006F954 / 8006F628 | Own x597_bits selects same-rig versus cross-rig partial installation; cross-rig source omission masks and ftPartsRemap use that tag. |
| ftanim_tracks.c ftAnim_8006FE08 / 8006FCE4 | Equivalent full-track same/cross-rig selection and semantic remapping. |
| native_fighter_runtime.c slot construction | Rewrites only tag equal to Definition.animation_source_kind to custom kind, preserving flags&0x03FFFFFF. |
| itmariofireball.c trace | Logs x597_bits/x594_bits only; no behavioral selection. |

For an owned Nana definition animation_source_kind11, normal tag11 becomes the
custom own rig. Taro tag33 stays33 and therefore retains real cross-rig remapping.
ftPartsRemap converts source joint -> semantic part -> own joint. Cross-rig tracks
use lbAnim_8001E7E8 where destination flags_b3 is false, filtering translation
channels5/6/7; blindly replacing tag33 would change this behavior. Both source
omission table and semantic source33 mapping must stay available as immutable rig
facts, just as physically copied Zelda Taro clips retain33. They are not a behavior
donor or a reason to query a live primary fighter.

All callers of the whole fallback descriptor have also been enumerated:
ftData_80085CD8 / 80085E50 read x14 (and nonnative archive name/size/offset);
the two ftAnim functions above read loop; ft_08A1 dead-up, AppealS, AttackS3/S4,
SquatWait and pl_80038628 read x8 availability; DownSpot reads x14 presence.
The absent-entity pltrick path already reads its own global table directly, outside
live fallback. Physical copies close the live availability paths without altering
command selection. This census does not claim retargeted poses have been executed.

Raw copied command pointers into PlPp would be invalid. They are unnecessary:
Nana already owns complete command graphs for these motions, including primary
Squall commands. Preserve those bytes/relocations. If later editing intentionally
replaces them, it must copy complete call/jump closure and remap every pointer;
existing ftActionData parser alone is an importer, not such a cross-DAT writer.

The normal native bank importer initializes x14NULL then decodes every x8!=0 entry.
After physical copy, existing custom-kind normal lookup resolves its own bank and
all shared normal/camera/attack selection consumers see the owned availability.
No raw Nana identity branch must be kept merely to provide the copied clips.

Before acceptance, run actual importer and shared animation installation/evaluation
against all five kinds of representative input: own eight Nana clips, copied
ordinary49-node clips, four borrowed52-node Taro clips, preserved empty entries,
and all four Nana costumes. Compare stock fallback to materialized own descriptors
for clip contents, loop, command graph, blend, remapped track channels and resulting
SRT/root-motion state. No missing-bank workaround or universal tag rewrite is
authorized by this read-only feasibility result.

Demo player behavior is the remaining semantic qualification: stock Demo bypasses
primary fallback, whereas unconditional materialization supplies those formerly
missing own entries. Versus scope can use the completed bank while keeping owned
demo bank x14 separate/unchanged; video/demo scenes outside versus need explicit
acceptance rather than claiming this replaces every scene's stock hole semantics.
