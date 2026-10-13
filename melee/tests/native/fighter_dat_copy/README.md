# Generic owned DAT pointer-field transplant

The generic copier is implemented in `src/melee/ft/native_fighter_dat_copy.c/.h`.
It is not a public-symbol linker. The source investigation corrected the initial
assumption that Ice Climbers' external names must have an exported owner elsewhere.
`retail_symbols.json` scans the public roots of805 retail single-DAT archives: none
exports any of the four names. All eight costume DATs expose only their own joint
and material-animation roots. These are deliberately NULL placeholders.

## Actual source selection and source values

`ftPp_Init_OnLoad` registers its item_list0/1/2 via it_8026B3F8 as Ice, Blizzard and
GumStrings. That function writes it_804D6D38[kind-It_Kind_Kuriboh].
`ftNn_Init_OnLoad` copies fighter attributes/offset and does not register articles.
Item_80267AA8's normal fighter-item category obtains xC4_article_data from this
global table. The entire Article then supplies common/special attributes, states,
model descriptor and dynamics; initialization stores xC8_joint and xCC_item_attr
from the same Article. Therefore BOTH stock forms use the primary's actual Article,
not a mix of primary model plus Nana parameters.

The native parent-selected authored path in native_item_behavior.c instead asks
nativeFighterRuntime_ArticleAssets for the creator's package, so a self-contained
Nana must physically own the primary table's complete three-article data.

Actual retail data offsets (DAT.data-relative):

| Field | Popo | Nana |
|---|---|---|
| ftData root | 0x8FEC | 0x6D84 |
| root+0x48 item-table pointer target | 0x8FDC | 0x6D78 |
| article0 Ice | 0x8F60 | 0x6CF8 |
| article1 Blizzard | 0x8F98 | 0x6D30 |
| article2 String | 0x8FC8 | 0x6D60 |
| String attrs | 0x3CA0 | 0x1B84 |
| String attrs x24/x28 local target | both0xE8E8 | external Gum_TopN |

The compared special scalar ranges Ice0x34/Blizzard0x14/String0x24 and first128
common-attribute bytes are byte-identical between forms. This is limited evidence;
the correct selection remains the full registered Article rather than relying on
that accidental scalar equality.

Original lbArchive_InitializeDAT calls HSD_ArchiveLocateExtern(symbol,NULL) for every
external. Popo's primary String model placeholder and Nana's unused article graph
placeholders therefore becomeNULL. it_802C248C actually builds all40 String links
from the primary typed attrs x24/x28 local graph; it does not use the NULL primary
ItemModelDesc root as an endpoint substitute.

## Implemented generic construction operation

One source DAT is enough for this concrete transplant; repeated construction calls
can select other explicitly supplied sources without runtime donor lookup. C entrypoint:

```c
bool nativeFighterDatCopy_PointerField(
    const void* destination, size_t destination_size, uint32_t destination_slot,
    const void* source, size_t source_size, uint32_t source_slot,
    void* output, size_t output_capacity, size_t* required_size,
    NativeFighterDatCopyStats* stats);
```

Both slots are explicit serialized pointer-field locations. The source must resolve
LOCAL via HSD_DatResolve, selecting its target. Destination replacement accepts an
existing LOCAL pointer field only for the first bounded implementation; external
chain surgery or null-field inference is not needed for Ice Climbers and should not
be silently added. Inputs are immutable, output nonoverlapping and caller-owned.
This is an offline/construction transition operation, never a gameplay allocation.
OutputNULL may perform checked sizing only, with no partial publication on failure.

Concrete caller derives roots using HSD_DatFindRoot and sets destination_slot=Nana
root+0x48, source_slot=Popo root+0x48. The operation is generic; no fighter tags,
article indices or proprietary descriptor shapes are interpreted inside it.

## Why whole DAT.data copying is the robust bounded first step

A typeless relocation walk cannot recover allocation extents reliably. DAT contains
raw arrays, embedded structures, interior pointers, aliased targets, and nonrelocated
scalar words. HSD_DatNextTarget is a nearest-target bound, not an object-size proof.
Guessing a graph's end would risk cutting textures, model animations or commands.

Append the complete source DAT.data block once, aligned32, and rebase every source
local relocation slot AND its target by the append base. This costs101,664 bytes for
the actual Popo source. It retains complete aliasing and all source command/model
data without a type-specific graph parser. Destination root entries, command graph,
animation flags, blend data and every other local field remain unchanged; replace
only destination_slot. Do not publish the source's public roots in the destination.

Preserve destination symbols/string bytes and retain original placeholder semantics.
HSD_DatResolve identifies externals through their LINK LISTS, not their payload as
an offset. Source external slots must remain external in the appended data. Rebase
their next-link words except0xFFFFFFFF and rebuild external symbol entries.
For a same-name destination/source external, merge the copied chain with the existing
chain using the copied tail -> old head and publish copied head; this changes no
original destination link words and keeps all slots' symbol identity. Distinct
source names append to the symbol string table. Duplicate names/overlapping chains
within an input must be rejected rather than relying on lookup order. No external
name is automatically searched for in another archive or invented as a root.

An unresolved external here has the explicit original NULL-at-import meaning.
The copier cannot prove arbitrary typed graph completeness. The caller must validate
the resulting actual import/required resource graph contract; for this transplant,
String x24/x28 must be local while the primary String model remains intentionally
external/NULL. A requested source requiring an unavailable nonNULL external needs a
separate explicit binding contract and must not be declared complete by this API.

Reject malformed DAT/header/relocations/chains, source-slot NULL/external/invalid,
unaligned/out-of-range destinations, overlaps, ambiguous external names/chains,
duplicate relocation slots, capacity/32-bit size overflow, and external-chain cycles. Revalidate
the completed archive with real HSD_DatOpen before publication. Source/destination
files remain byte-identical after either success or failure.

## Actual importer acceptance and boundaries

Use actual HSD_DatOpen/Resolve and real ftCharacterAssets_Import, never alternative
import algorithms. Synthetic fixtures cover relocation aliases/interior targets,
source-zero local targets, external chain merging, unchanged destination roots and
other fields, invalid input/capacity/overlap and transactional failure. Actual retail
Nana<-Popo table transplant must import all three complete Articles with source
attribute/model/state/dynamics values and local String endpoint graph topology.
Compare output decoded articles against an independently imported Popo source, and
all other Nana fighter flags/commands/blend/scalars against original Nana. Preserve
raw placeholder NULLs separately from mandatory endpoint resources. No animation
materialization, behavior-independent activation or gameplay is part of this gate.


The cohesive console gate passes **193,601 checks and17 transactional rejection
cases** against frozen71 complete production character/costume/animation import
units and hash-pinned actual DAT/graph/article support libraries. The transplant
output is204,210 bytes and imports22 copied typed graph descriptors. It retains
all original Nana bytes except root+0x48, including commands, flags, other pointers,
public roots, external-chain payloads and original string bytes. Appended source
bytes remain identical except serialized local pointers/external chain links.
Every copied local/external reference is independently checked with actual DAT
accessors; complete three-article imports match a separately imported Popo source.
This acceptance does not register the bases or modify the construction pipeline.

The full imported unresolved visit count is deliberately **primary1, original
Nana6, transplanted Nana2**, not1 for the transplant. Frozen71
`import_part_animations` scans Nana group2's animation array from0x6CCC through
three local entries then external0x6CD8 (`ItmIceclimberIceShot_TopN_joint`), before
stopping atNULL0x6CDC. This is an unchanged pre-existing native importer scan of
the following placeholder; original Nana and transplanted Nana both visit it.
The second transplanted visit is the copied primary String model placeholder.
Required String x24/x28 graphs are local and import successfully. No importer
change or blanket unresolved-count suppression was made. Temporary instrumented
complete source copies are preserved in the archive diagnostic directory only;
final acceptance uses unmodified importer units.

Exact console reproduction:

```powershell
python tests/native/fighter_dat_copy/run.py --root . --baseline .local/baseline71 --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso" --output A:/FreakMelee-test-archives/fighter-dat-copy71-root-repro
```

Final source-path receipt: `A:/FreakMelee-test-archives/fighter-dat-copy71-final/result.json`.
Earlier failed first/diagnostic runs and temporary importer instrumentation remain
in neighboring archive directories. Allocation failure is a production error path
but is not allocator-fault-injected by this matrix. There is no gameplay, animation
materialization, paired-family registration or independent activation claim.

Integration recipe: compile the new production `.c` with the existing actual DAT
unit and Native headers. The console test also links the complete frozen character
import units and their normal asset support libraries (all pinned in `result.json`).
`run.py` is standalone; no extraction-generated headers or CMake mutation is needed.
