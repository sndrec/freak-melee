# Interpreter motion, visibility and collision checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

This grouped slice adds eleven reusable operations without stock-special
delegation: recipient motion ID, held-item visibility, three horizontal-smash
input observations, velocity-to-facing callback installation, Item segment
sweep, and four stage collision-line operations. Public contracts are linked
from the [scripting reference](scripting/index.md).

The queries retain actual physical objects and owned program/storage identity.
Recipient motion accepts foreign stock or source-package Fighters. The smash
observations read native common input settings, including the integer window
converted to Float. Held visibility reads the Fighter flag independently of
whether an item is currently held. Callback installation selects the unchanged
native alignment callback and clears the corresponding authored slot; it does
not immediately change velocity.

The Item sweep executes `it_8026EA20`, preserves initialized in/out values on
miss, and retains the native position snap on hit. Collision-line operations
use current endpoint geometry, native line normal and velocity remapping, and
the actual Item collision record. `item.bind_stage_line` notifies a stage joint
contact callback; despite its historical name, it does not store a line index.
Native callback effects remain intact. Publication checks do not contain
arbitrary destructive native callbacks or undo completed native side effects.

Popo/Nana's `rope_launch` local is now `companion_facing`; its previous name
shadowed the engine `facing` namespace. Their one-argument legacy landing call
now directly uses existing `state.landing_special(false, lag)`, preserving the
original native arguments. No source body was removed or compatibility alias
added.

## Verification

The client, content tool, source-assets and selected integration targets build.
Sixteen selected CTests pass. All tests for this slice are console processes;
no game, GPU or audio process was launched.

| Connected fixture | Checks | Native comparisons | VM coverage |
| --- | ---: | ---: | --- |
| Recipient motion | 407 | 37 | 26 calls |
| Held visibility and smash settings | 2,287 | 594 | 107 Fighter calls |
| Velocity-alignment callback | 301 | 16 complete Fighter images | 3 calls |
| Item sweep and stage lines | 1,018 | 59 | 10 Item and 2 Fighter calls |

These fixtures run the actual compiler, prepared programs, actors and fixed
object pools. Callback alignment compares the actual `ftCommon_8007F76C` body.
Asset/common-data inputs and selected lifetime mutation boundaries are
controlled. They prove the named operations, not complete fighter behavior,
retail rendering or whole-game deterministic parity.

Receipts, independently rehashed against current pinned inputs:

- `A:/FreakMelee-test-archives/script-target-motion-oct10-root-final/receipt.json`
  (1,691 inputs), SHA256
  `7b3290e5065a7b1e3fc828fcdf6856a05ece3b0795fb1ce1ddc9af0f52b17cfc`.
- `A:/FreakMelee-test-archives/script-held-item-visible-smash-controls-oct10-final2/receipt.json`
  (1,694 inputs), SHA256
  `8b072be60a920a7907d1b5187f240e86262140c8b4f967032e3def3a7996a3d9`.
- `A:/FreakMelee-test-archives/script-callback-velocity-oct10-root-final/receipt.json`
  (1,696 inputs), SHA256
  `3b1015f8290499872e18c47e3b0ef741499435ef30fd6c1f5163d0441fd64e7d`.
- `A:/FreakMelee-test-archives/script-item-sweep-lines-oct10-final3/receipt.json`
  (1,818 inputs), SHA256
  `b5f5a21a5c6d215b3849503d7eb257baab4bc493536332dae5d3eaa040d01efd`.

The collision fixture compares unchanged Item/map/line/remap/joint/contact
kernels and actual Aurora normalization. The intersection leaf, geometry and
contact callbacks are controlled. It covers both Item and Fighter query VM
paths, ordinary stage callback effects and traversed joint-prefix admission.
It does not prove the complete collision solver or stock stage callback closure.

The existing visuals regression passes 17,249 checks / 2,245 native comparisons
with 544 setter and 774 query calls. Its unselected new dependency leaves abort
explicitly; it does not substitute for the connected four-property fixture.
Receipt `A:/FreakMelee-test-archives/script-common-visuals-oct10-reader-closure-final2/receipt.json`
pins 1,596 inputs, SHA256
`08d74ad88e40b297bc32715c82d63fd73c54eadfd837c09369850104244a8ca9`.

The existing stage query regression passes 2,062 checks / 128 cases / 86 native
comparisons / ten VM calls. Its isolated unselected Item collision leaves reject
explicitly. Receipt
`A:/FreakMelee-test-archives/script-stage-queries-lines-oct10-02/receipt.json`
pins 1,721 inputs, SHA256
`d2bc38976a59fc51b9f1021e7f170e8a02a6350a8a68db453e7b9f530da4a381`.

## Next integration boundary

Ten retained whole sources still prepare Mario, Dr. Mario, Peach, Game & Watch
and Zelda. Next exact diagnostics are Link 695 / Young Link 699 at
`controls.try_side_special`, Sheik 1627 at `audio.play_global`, and Popo 1002 /
Nana 1006 at `action.category`. This is preparation evidence, not full runtime
acceptance. The current rebuilt-tool census is
`A:/FreakMelee-test-archives/source-motion-visible-sweep-census-oct10-final4/receipt.json`,
SHA256 `bcefd5423f67787daed52da3a7a69d1275686ed4b16563ebb07b1b79c73f6a87`;
root verified the tool and all ten source hashes.

Next work includes ground special arbitration, generic action/state reads and
global sound, followed by actual paired roles/member mapping and full fighter
behavioral acceptance. Public resource-animation application and compact
whole-match save/load remain unfinished. Regions/Pages still owns broad
snapshots. The project goal remains active.
