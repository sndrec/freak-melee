# Samus grapple descriptor portability checkpoint

Before implementing generic script-owned fighter hitbox creation, inspection
found a concrete native-port mismatch in `it_802B7160`. Its descriptor is a host
C bitfield structure. The original GameCube halfword read at byte offset 2
selects damage on the console but selects bone-ID bits on Windows. The stock
`it_803B8660` descriptor declares damage 0 and bone 139; the host read supplied
17 to `ftColl_8007ABD0`.

The initializer now reads the named damage field. Its grounded flag and the
other final-command fields also use named fields instead of byte-offset casts.
The stock descriptor, common damage/angle helpers, exact `0.003906f` coefficient,
x/z offset convention, activation, victim-history and cleanup policy are retained.
This restores console descriptor semantics; it is not an authored gameplay change.

The three modified-ground creation sites previously assigned only the five
command words, leaving the trailing `x134_4` victim-only flag uninitialized.
They now initialize the entire descriptor with the unchanged stock command data.
That flag is deterministically false, allowing this ordinary attack to hit foes.
Retail uninitialized memory cannot have reproducible parity; this choice is
documented in the code rather than copied into the future script API.

## Evidence

Receipt: `A:/FreakMelee-test-archives/samus-grapple-hitbox-descriptor-oct11-b/receipt.json`.
The console-only fixture extracts both the original initializer at `1ba857e70`
and the current body, plus unchanged common group/victim-history routines.
It compares the current body with independent GameCube command-word decoding.
The stock case and 8,192 varied descriptors over eight initial state/group modes
pass **65,537 cases and 884,753 checks**. They compare all capsule bytes for all
four slots, cleanup marker, damage/angle/common-bone arguments and call order.
Signed offsets and shield damage, flags, history preservation/reset/copy, ordinary
and common bones, and the reserved endpoint index139 are covered. The original
Windows initializer disagrees on damage in 65,489 cases; its stock value is17.

Damage/angle helpers and common-bone conversion are controlled observers in this
fixture. Their gameplay effects are not acceptance evidence here. Initializing
the three caller descriptors is checked statically. No console/Dolphin runtime,
script VM, graph constructor, resource lifetime or full Samus gameplay was tested.

Seven native targets build and both focused script/parameter CTests pass. The
build retains an existing pointer-to-integer warning in a different grapple
function (`it_802BABB8`); it was not changed speculatively in this checkpoint.
No game was launched, no installed content was changed, and nothing was pushed.

## Remaining scope

The generic immutable hitbox specification, bounded creation/binding and
script-owned modified-mode transition still need implementation. This checkpoint
does not add a fighter-specific scripting extension or delegate script behavior
to this initializer. Samus remains an unregistered authoring candidate. Throw
accessories and complete resource/solver/lifecycle/gameplay closure also remain
open.
