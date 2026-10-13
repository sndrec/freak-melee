# Interpreter tether movement and Article motion checkpoint

This batch continues the complete retained Link/Young Link and Popo/Nana source
integration after `1e9849431`. Generic engine operations remain native; authored
fighter decisions remain in the interpreter.

## Connected operations

- `capture.mask(Int)` writes the common exclusion mask. It does not arm a grab
  detector. The accepted range is 0 through 32767, matching the retained source
  exporter; implicit Float and Bool conversions are rejected.
- Damage-fall entry and the second ledge-jump phase execute their actual common
  kernels. Ledge probing uses the existing generalized geometry primitive;
  scripts retain responsibility for explicitly clearing velocity. Ledge
  availability checks the actual occupant rather than entering a catch.
- Article collision facing accepts a signed Int. Default ECB reset uses the
  Item's stored default bounds and scale. Ground sliding drives both authored
  stage-left and stage-landed callbacks through the retained synchronous scope.
  Sliding does not require a rendered model. Clearing animation commands only
  clears the command-stream pointer, preserving command variables.
- Article floor-normal components read the actual Item collision result,
  preserving its Float bits. Healing-pickup rejection is an editable Bool
  property on retained Fighters, including independent and stock receivers.

The full Link sources now use ordinary velocity assignments, the shared
position-based wind operation and local joint visibility where these exactly
replace redundant exporter spellings. Popo/Nana no longer copy or reload unused
stock attribute scratch: their editable `attrs` read immutable owned parameters.
Scratch operations remain available for genuine users such as multijump setup.

See [tether controls](scripting/tether_controls.md),
[Item terrain](scripting/item_terrain.md),
[Item observations](scripting/item_observations.md) and
[pickup policy](scripting/held_item_controls.md).

## Validation

Grouped receipts independently rehashed by root against their frozen inputs:

- `A:/FreakMelee-test-archives/script-item-common-oct10-final1/receipt.json`:
  4,133 checks, 896 complete Item comparisons, 128 terrain cases and 20 authored
  VM calls; all 1,719 input pins match.
  SHA256 `8874a0601f4df0dede2c31adfb29299588833cf881e54b50ea7d998008f264b2`.
  Actual native slide and fixed-ECB/facing kernels are unchanged. Geometry and
  collision response leaves are controlled; no-model Items, both authored tails
  and destructive callback reentry are exercised.
- `A:/FreakMelee-test-archives/script-surface-healing-oct10-final1/receipt.json`:
  10,167 checks, 1,961 native comparisons and 814 VM calls, including 516
  receiver calls; all 1,706 input pins match.
  SHA256 `5010ef39148e9d8bc06794832adde1be458273e3ddbc03d3748a03e2a5ccbb9e`.
  Actual healing amounts and the native eligibility expression are compared;
  full pickup searching and inventory interaction remain outside this proof.
- `A:/FreakMelee-test-archives/script-tether-prerequisites-oct10-final/receipt.json`:
  1,904 checks, 40 native comparisons, 44 VM calls, 20 retirement/fault cases
  and 29 source compiler builds; all 1,714 input pins match.
  SHA256 `387eb8eb55b07bf21e959b282b7cc80629ba2cf5b30fb2952d2c2d91434672cd`.
  Selected common kernels, including actual parasol entry, execute against
  `1e9849431`. Motion/animation delivery, geometry and drift/statistics leaves
  are controlled. The fixture exercises active published-program identity,
  rather than merely modifying a preparation copy.

The client, content tool, source-assets and selected integration targets build;
all sixteen selected CTests pass.
These selected-kernel console comparisons do not establish complete tether or
paired-fighter gameplay.

The ten-source census is
`A:/FreakMelee-test-archives/source-tether-motion-census-oct10/receipt.json`,
SHA256 `f495a682bd3aee620922c588ddea59f68f0204198c6e2cf0d1a121eff10d548a`.
Root rehashed the actual content tool and all ten full retained sources.
Mario, Dr. Mario, Peach, Game & Watch, Zelda, Sheik, Popo and Nana now prepare.
Popo and Nana have 257/266 compiled functions and 6,276/7,268 instructions respectively.
Link 906 / Young Link 910 reach the next cached `collision.position.x` observation
gap. No source body was removed for these preparation results.

## Remaining work

Link/Young Link still require their real dynamic-joint and animation-descriptor
operations, existing-matrix observations, release-attachment ownership and
Arrow effect semantics. These cannot be substituted with similar-looking
imported animation or position helpers. Shared target observations remain to
be connected where required by those retained Article bodies.

Simultaneous companion activation/member mapping, resource animation application
and whole-fighter acceptance remain separate work. Compact whole-match save/load
still retains Regions/Pages as its broad oracle. The goal remains active.
