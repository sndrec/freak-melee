# Popo/Nana copied-base metadata and retail import proof

The new ftPopo/ftpopo_clone and ftNana/ftnana_clone files define unregistered copied
retail bases. They retain their stock source_kind and original motion-table pointers;
they do not claim independent scripted behavior. The frozen owned-pair composition
namespace is unchanged. No new exported costume arrays or runtime donor fallback
are introduced.

Each base owns40 semantic bindings/49 runtime parts/zero omissions,321 actions,
14 demos, three article slots0/1/2, the ClimberString serialized schema, effect
bank14, sound bank13/mute mask0x44, its own fighter/default costume roots and demo
names. Popo exposes all eight direct special entries; Nana exposes only N/Nair and
LW/LWair, preserving the stock asymmetry. Both tables retain96 nonnull callback
phases, with all four phasesNULL in363/366. The raw source_kind provides stock
lifecycle inheritance; generated independent modules must replace these decisions.

`fixture.py` reads the actual USA1.02 ISO. It derives both54-entry semantic maps,
runtime count and omissions from PlCo.dat, verifies all eight costume joint and
material-animation roots, and extracts complete immutable71 stock motion-table
initializers. Callback identities are inert, aborting controlled boundaries and
are never invoked. The test therefore verifies metadata, not callback behavior.

`run.py` compiles the actual new C Definitions as overlays against frozen71 headers,
plus the complete frozen ftcharacter_assets.c, ftcostume_assets.c and
ftanimation_bank.c import units. Existing static libraries supply pinned import
dependencies. There are no importer snippets or replacement import algorithms.
Every SDK/compiler/support-library/source input and extracted retail archive is
hashed in the receipt. The console process has no game, window or audio path.

Reproduction (fresh output directory):

```powershell
python tests/native/ice_climbers_catalog/run.py --root . --baseline .local/baseline71 --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso" --output A:/FreakMelee-test-archives/ice-catalog71-root-repro
```

Acceptance receipt is supplied in the handoff after the final frozen run. It covers
two copied Definitions, all eight costumes, six declared article slots and actual
raw animation imports. Popo imports260 clips; Nana imports only8. This deliberately
preserves the missing clip evidence rather than claiming a self-contained Nana bank.

## Intentional shared-article placeholders

Raw Popo DAT has one external symbol `ItmIceclimberGumStrings_TopN_joint` at0x8FB8.
Raw Nana has four external symbols:

| Symbol | First slot | Complete reference chain |
|---|---|---|
| ItmIceclimberGumStrings_TopN_joint | 0x6D50 | 0x6D50 |
| ItmIceclimberGum_TopN_joint | 0x1BA8 | 0x1BA8 -> 0x1BAC -> 0x6D48 -> 0x6D4C |
| ItmIceclimberIceShot_TopN_animjoint | 0x6CE8 | 0x6CE8 |
| ItmIceclimberIceShot_TopN_joint | 0x6CD8 | 0x6CD8 |

Chains terminate at0xFFFFFFFF; slot payloads are NEXT LINKS, not local graph offsets.
Both Nana String attribute pointers x24/x28 resolve to the Gum_TopN symbol.
The generic article import plus typed String import visits the String references
again, yielding six unresolved import occurrences for four named symbols. Popo
has one unresolved import occurrence; its typed String attribute graphs are local
and both import successfully. Tests assert the actual NULL/unresolved outcomes.

These are intentional stock placeholders, not a failed name lookup: original
lbArchive_InitializeDAT walks every external symbol and relocates its chain toNULL.
None of these names is exported as a public root in the805 retail single-DAT
archives scanned, including all eight costumes. Popo OnLoad registers its three
complete articles globally; Nana OnLoad does not register its placeholder table.
Thus stock items created by either fighter receive the Popo article, including
its local String graphs. Popo's NULL primary String model is intentional because
the real custom rope constructor builds its models from the local attribute graphs.

The failed early assertion that Nana had complete local String graphs is retained
in the second output directory. This is an ownership qualification for an authored
Nana package, not an importer defect. Physical copying must reproduce the actual
primary registered article table, not invent public symbols or substitute a model
for an intentionally NULL field. Source-kind None activation also waits for fighter
animation materialization, load/common/CPU ownership and paired package acceptance.

## Registration handoff

`catalog.patch` is a proposal only. It adds the two base includes/registry entries,
normal project base names, six import article-slot bindings and CLI help text.
No shared file was edited. It is generated against live source and passes:

```powershell
git apply --check --ignore-space-change tests/native/ice_climbers_catalog/catalog.patch
```

The whitespace option accounts for the existing Windows CRLF working files; it
does not alter the edits. Root owns application/CMake integration. No paired family
registration, default template, copy materializer, or UI choice is included here.
Normal CreateStockFromDisc already reads immutable original DAT/AJ/default costume
filenames, so the verified alternate costume facts stay in the fixture rather than
unused exported C arrays. New source files are under110lines each.
