# Owned action-bank materialization

This is a native authoring operation, not a gameplay fallback. The implementation is
`src/melee/ft/native_fighter_animation_copy.c/.h`; this namespace exercises that
unit with the immutable SDK71 importer and traversal sources. Nothing in this
fixture installs a package or launches a game.

## API and ownership

`nativeFighterAnimationCopy_Materialize(destination, source, output)` accepts two
immutable fighter DAT views, their named fighter roots and action counts, and two
physical action-bank byte spans. Counts are bounded at 1,024; each indexed bank is
bounded at 1,024 distinct root names. The caller supplies an empty output. Complete
success publishes two ordinary `malloc` allocations; failure leaves the output
unchanged. `nativeFighterAnimationCopy_Free` releases them and clears the result.
Do not release these allocations with `_aligned_free`.

For each destination action with size zero, the corresponding source action can
supply its clip name, physical bank offset, member size and complete DAT member.
Only these first three descriptor fields change. Existing destination flags,
command pointers/graphs, blend bytes, original data, roots/reference/string tables
and demo descriptors remain intact. Destination bank bytes are retained. Only
source members referenced by filled actions are appended, with initialized
32-byte padding. Several names in one member share one physical copy. Complete
identical existing members are reused; conflicting names or partially overlapping
member aliases fail. Both input archives remain immutable. No pointers or donor
queries are serialized. The output DAT is structurally validated before publication.

A successful structural materialization does not claim every clip decodes: normal
construction must still use the real fighter and animation importers. The fixture
performs that actual import for the retail data. Unresolved article externals and
separately loaded demo motion archives are outside this operation. In particular,
this does not replace the separately owned DAT external-symbol materializer.

## Reproduction

Run from the repository root with a fresh output directory on A:

```powershell
python tests/native/fighter_animation_copy/run.py --snapshot .local/baseline71 --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso" --output A:/FreakMelee-test-archives/fighter-animation-copy71-repro
```

The runner verifies every snapshot manifest hash before and after execution. Only
the new materializer header is overlaid; the implementation is compiled directly.
All importer, retarget, track installer, FObj and interpolation sources/headers are
from the supplied snapshot. Python extracts raw ISO inputs and exact production
function bodies for the fixture; the materialization itself is native C. Every
subprocess is created without a window. Visual Studio 14.44 and the snapshot Clang
are selected explicitly. No live SDK72 header is included.

The accepted local receipt is
`A:/FreakMelee-test-archives/fighter-animation-copy71-accepted/receipt.json`.
It records all native unit/fixture hashes, extracted-body hash, original archive
hashes, output hashes and exact commands. Earlier failed outputs remain separate.

## Accepted checks

- Actual Nana/Popo input and real fighter/action import: **161,126 checks**.
  Nana keeps eight original clips; 260 action holes are filled using **199**
  distinct Popo members. One unreferenced source member is not copied. There are
  268 populated actions and 53 deliberate holes. All original descriptor bytes
  outside the three filled fields, all blend/command/demo payload bytes, roots,
  external tables and string payloads are preserved. A second materialization is
  byte-identical. Nine malformed/conflict cases leave empty outputs untouched.
- Structural capacity and deduplication: **9 cases / 5,151 checks**. The exact
  1,024 bound succeeds; larger action counts and a 1,025th merged symbol fail.
  Bad relocation/descriptor references fail; a nonempty output is untouched.
  Two roots in one member are copied once; an identical existing member is reused.
- Retarget/track/pose sampling: **10,720 pairs / 11,759,766 checks**, with
  **1,950,432 scalar pose samples**. All 268 populated actions, all four real Nana
  costume graphs, live/blend targets, complete loads and partial starts 0/1/4/14
  are compared. The actual `ftAnim_8006FE08`/`8006F954` branches and their complete
  full/partial traversal functions run, using actual decoded PlCo part maps and
  omissions and the actual `ftPartsRemap`/`8007506C` functions. Own tag11 becomes
  the fixture's custom kind for the owned registration; genuine tag33 stays33.
  The complete actual `lbAnim_8001E6D8`/`8001E7E8` installers and translation
  filter/order functions run. The actual FObj decoder and Hermite interpolator
  sample frames 0, .25, 1, 5.5, 17 and 31.75 and compare scalar SRT bits exactly.

The retarget fixture controls fixed allocators, AObj scalar setters, JObj flag
writes, and skeleton shells from the actual imported costume descriptor graph.
Part translation flags use the actual extracted `ftParts_80074E58` body. Its scalar
SRT observation sink does not run JObj matrix, IK, callback, rendering, or AObj loop
policy evaluation. It claims full/partial track selection and sampled scalar pose
agreement, not rendered gameplay agreement. Character import uses layout `{0}`;
article ownership/presentation is deliberately not accepted here. Demo descriptors
are preserved byte-for-byte, without attempting action-bank offset reconstruction.

## Integration recipe for the parent

The new production `.c/.h` have no shared runtime/header/compiler dependencies.
Add the implementation to the authoring/content-tool construction path when
reviewed. Apply it after generic external graph reconstruction, before writing
owned project DAT/bank payloads and running actual import. The caller owns success
allocations and commits both payloads together; source archives are never changed.

For a CTest target, register the Python runner with `--snapshot`, `--iso`, and a
fresh A: output. Its standalone source list and extracted functions are explicit
in `run.py`; no ignored generated header is a checked-in build dependency. A
normal current-SDK target can compile the native unit independently, but the
frozen71 source/retarget receipt must retain its explicit provenance.
