# Normal Popo/Nana asset construction proof

`run.py` executes the actual rebuilt content tool's `clone-stock`, `inspect-project`,
`bundle-project` and `verify-package` commands for Popo and Nana. It never calls an
offline materializer or writes replacement project assets. `audit.py` only reads
produced bytes; `import.c` only imports/audits those produced payloads. Neither
native copier is linked into the audit executable. All outputs and the temporary
content store are isolated on A:. Nothing is installed or launched in a game.

## Reproduction

After the parent grants a matching rebuilt-tool handoff, run from the repo root:

```powershell
python tests/native/ice_climbers_asset_pipeline/run.py --tool build/native/native_content_tool.exe --snapshot .local/baseline71 --iso "C:/gayms/freak-melee/Super Smash Bros. Melee (USA) (v1.02).iso" --output A:/FreakMelee-test-archives/ice-climbers-asset-pipeline72-root
```

The tool performs normal production construction using its current registry/base
metadata. The runner hashes that tool and the live production sources before and
after execution. The independent importer audit uses exactly frozen71 sources,
headers and Clang, whose whole manifest is verified before and after. This explicit
split is intentional: the importer is an asset-format proof, not a claim to have
compiled a module against an earlier ABI. No source overlays are used. Every text
read/write is UTF-8; every subprocess has `CREATE_NO_WINDOW`; scoped Clang selects
Visual Studio 14.44. The ISO is read-only.

## Accepted local evidence

`A:/FreakMelee-test-archives/ice-climbers-asset-pipeline72-final/receipt.json`
contains exact commands, logs/output hashes, retail archive hashes, production
input hashes and the frozen importer manifest hash. The first completed evidence
is retained separately at `ice-climbers-asset-pipeline72-first`.

- Both normal raw cloned projects inspect successfully, bundle into ordinary
  single-member packages, and pass actual registration/release verification.
- Popo's produced `fighter.dat`, `animation.dat` and `costume.dat` are exactly the
  original retail PlPp, PlPpAJ and PlPpNr bytes. The native import audit explicitly
  imports Popo's produced files after its own byte comparison.
- Nana keeps eight original clips and fills 260 missing actions using 199 distinct
  physically copied Popo members. Its bank has 207 root symbols, with no extra
  unreferenced source member; 53 deliberate action holes remain. Every produced
  descriptor points at its actual member offset/name/size. Every complete member
  is byte-identical to the corresponding retail blob.
- Every original Nana data byte is preserved except the article-table pointer and
  the first three fields of the 260 filled action descriptors. Original command
  pointers/graphs, flags, 321 two-byte blend rows, demo descriptors, roots and
  original reference meanings are checked. Own source tag11 is preserved for
  ordinary copied clips; genuine Taro source tag33 remains for motions262–265.
- The independent serialized audit checks the entire copied Popo data region:
  every local relocation now addresses its shifted target, external references
  retain their exact symbol, and every other byte is literal-identical. All three
  article descriptors point into the owned copied region. Original source roots
  are not published as destination roots. This covers complete source command,
  attribute, geometry and animation graph byte closure rather than guessed spans.
- The actual complete frozen fighter/article/action-bank importer audit passes
  **76,610 checks**, compares **31,058 decoded tracks**, and confirms **268**
  populated action clips plus all **three** article descriptors. Common/special
  attributes, models/attachment fields, state animation/material/shape graphs,
  dynamics counts and the GumStrings shared-joint alias are compared with retail.
  All **22** imported records in the copied region retain actual primary importer
  offset/type/count identity. The GumStrings40-node count and aliased non-null
  x24/x28 joint are present.

The actual tool package IDs from this receipt are:

- Popo: `5edd163fd3a44cb5164696decff249e9bd34d37498fb0c506822c40dafae3248`
- Nana: `fd0e293f4d9256ea5aa150d4741b4df9447203810486218522307b90381c5873`

## Limits and interpretation

These raw bases still inherit stock fighter behavior. The proof closes asset
construction, not independent scripting or simultaneous family activation. Both
packages are verified as single members; no transfer, live entities, rendering,
gameplay, editor family support or human install is claimed.

Tool verification reports Nana277 own-rig tags and4 borrowed tags. Those totals
include nonempty **undecoded demo metadata** and are not a decoded-action count.
The independently imported action count is268; the four tag33 Taro clips use
immutable common source-rig facts, not a live Popo Definition. Demo descriptors
remain untouched, without asserting that separate result/intro archives are in
these action banks.

The full importer reports two unresolved visits: the unchanged Nana part-animation
placeholder and the primary model placeholder. They remain explicit external
metadata; the active three-article table and required Gum/Ice graph payloads are
physically local. No fixture silently synthesizes those placeholders. Article
construction/callback/render execution remains outside this asset-import proof.
Earlier `fighter_animation_copy` evidence separately exercises actual full/partial
track selection and scalar SRT; this pipeline does not repeat matrix/IK/render tests.

Root changed a production project-source comment after the successful final run,
clarifying that saved **assets** need no live donor fighter. The recorded source
hash remains the actual captured input; the comment-only change does not change
this receipt into a claim of behavior independence. A fresh run pins current text.

## Parent integration

Only this namespace is owned by this task. No production/CMake change is needed
for this test to run. A future console test registration can invoke `run.py` with
a rebuilt tool, immutable snapshot and fresh A: output. The standalone native
source list is explicit in the runner. Its actual source units are frozen import
implementations; no ignored generated header or copier output is a test input.
