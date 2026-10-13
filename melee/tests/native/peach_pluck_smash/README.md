# Peach pluck, forward smash and Explode source composition

This is an unregistered, inherited source candidate. It assembles the accepted
`peach_fighter_composition44` candidate and adds the complete down-B fighter
phases352/353, forward-smash phases349–351 and both Explode article states. It
retains the existing Turnip, cleanup, Parasol producer and setup/CPU components.
No default template, clone definition, shared SDK or accepted namespace changes.

The final console batch targets frozen SDK44. Its receipt is
`.local/peach-pluck-smash44-final/result.json`. It compares 5,028 original/generated
pairs plus six separately counted authored removal/reuse safety cases. Every
input and immediate output is SHA256 recorded, and `source_snapshot/` archives
this exact namespace. Preparatory parser/fixture failures remain in
`.local/peach-pluck-smash44-{prepare*,check*}`; they are not acceptance receipts.

## Reproduce

From the repository root:

```powershell
python tests/native/peach_pluck_smash/run.py --root . --baseline .local/baseline44 --output .local/peach-pluck-smash44-repro
```

The runner compiles the real DAT importer against frozen SDK sources, imports
`.local/peach-turnip-composition30/retail-peach.dat`, runs frozen `script-c`, builds
the complete generated DLL using frozen clang/SDK, extracts original production
bodies and compiles/runs the console fixture. It does not invoke snapshot
`script-build` (which could use live headers), a global build, install or a game.
`--project path` instead uses that snapshot's real `import-parameters` on an
existing project. A registered Peach project is unavailable in frozen44; root's
later45 normal clone/import verification is a separate acceptance step.

## Owned source and ordering

- `on forward_smash_enter` owns the complete pre-entry policy: clear interrupt
  and command0, draw 0..2 +349 until it differs from the previous owned variable,
  then publish that variable, transition and start animation. The fixture uses
  the actual `ftPe_AttackS4_Enter`, including zero through five repeated draws;
  it does not replace rejection sampling with `random.int_excluding`.
- Both held-item branches return immediately. Ground down-B throws a held
  Turnip with108; air down-B throws it with112. Other held items suppress pluck
  but are not thrown. Air with no held item does nothing.
- Ground entry clears the whole throw word, changes to352, starts animation,
  then binds `spawn_vegetable_accessory`. Ground loss and landing preserve the
  current frame and full Int mask `0x0c4c5080`, change ground/air first, transition
  to353/352, then rebind the accessory. Actual entry/collision callbacks install
  that callback; the fixture invokes the installed callback, never an on_tick
  surrogate. All58 face draws, all six rare-item weighted draws, cue absent/
  present and allocation failure are covered for each of these three routes.
- Completion enters Wait/Fall before forgetting the owned Turnip handle.
  Forward-smash animation, interrupt, physics and collision phases execute the
  corresponding original C bodies. Down-B input phases remain null.
- The six retail motion rows349..353 and356 are compiled as C metadata from the
  actual `ftpeach.c` initializer. Generated callbacks replace only the eighteen
  nonnull phases in the five owned states. Animation IDs, packed flags, camera
  and all other states are preserved. State356 keeps four null phase pointers.
- Explode creation carries a Bool choice through the generic creation value.
  Its owned initializer preserves timer60, command reset, hide, positioned
  effect1040, one-shot sound116/127/64, contact-hitlag clear, manual lifetime,
  physics disable, common expiry, selected state, owner scale and final debug
  notification. Common expiry overwrites60 with `ItemCommonData.xF8`; it is not
  a sixty-frame article lifetime. Both animation phases use the real expiry tick.
  Only damage-dealt(false) and interaction are nonnull global logic callbacks,
  matching the actual engine logic row. Physics and collision are null.

There are53 owned parameters, including26 real DAT attributes. The thirteen
additional parameters are seven named fighter policies and six Explode policies;
no additional fighter variables are needed (fourteen fields,57 bytes). The typed
local `create_explosion` helper is ready for a future Side-B caller but does not
activate or claim Side-B phases. Named parameter import also retains the original
article attribute declarations and true Peach slot mapping.

## Executed source and controls

`fixture.py` extracts complete `ftpeachspeciallw.c`, `ftpeachattacks4.c` and
`itpeachexplode.c` files with only include directives removed. It additionally
extracts actual collision wrappers, weighted Turnip constructor/pickup bodies,
scale/expiry/flag bodies and `nativeArticle_Create`/creator/catalog lookup. The
real `native_item_owner.c` and `native_callback_lifetime.c` are linked.

Allocation is a controlled boundary for both Item_80268B18 and Item_CreateAuthored:
it supplies live Item identity, position/facing/velocity, incoming scale and assets
before initialization. Actual original and generic SpawnItem fields are checked.
The authored constructor itself is real, including return of the captured handle
when its created callback removed/recycled the item. That stale returned handle
is not claimed to resolve live. The six authored safety cases check stopped tails
and invalid generation separately; original dangling-pointer behavior is not used
as a parity reference.

AB54 is a controlled boundary: it records attachment and invokes the appropriate
actual/generated Turnip picked-up callback. Ordinary Bomb/Saturn/Sword picked-up
callbacks are excluded from this fixture, consistent with the earlier accepted
constructor/owner proofs; they are not replaced with a Turnip callback. Owner
scale executes actual it_80274594, including root scale and ordered geometry
refresh calls; its dirty-matrix and geometry suboperations are observed controls.
Null/source/foreign current owners are covered for that scale operation.

Animation availability, shared Wait/Fall/throw/physics handlers, stage probes,
positioned particle allocation, sound playback, visibility and fighter pickup
statistics are controlled engine boundaries. This verifies call ordering, inputs,
mutations and callback lifetime at the composed boundary; it does not claim
animation resource playback, full collision math or audiovisual/gameplay parity.
The rare selection source's unused next-weight read after choosing a result is
still omitted in the accepted script, including its out-of-array final-entry read.
Decision values and RNG call order are compared without masking trace fields.

Float, Side-B, Toad and Parasol travel/animation remain pending. This source
candidate retains source_kind=Peach and makes no independent-fighter claim.

## Root reproduction through a normal SDK46 clone

SDK46 registers the stock Peach asset-clone base. A fresh `clone-stock` project
and the real `import-parameters` path reproduce 5,028 source pairs, 172,711
checks and six authored safety cases. Receipt:
`.local/peach-pluck-smash46-root-normal/result.json`. This removes the earlier
manual-project preparation requirement; the candidate still inherits the
remaining stock phases listed above and is not a fully independent fighter.
