# Rook neutral special

Neutral-B pulls a holdable item into Rook's hand. Successful pulls choose one
of eight snacks with equal odds. They use the normal item throw controls after
the pull animation. Rook starts with three snack charges; one charge restores
every 240 simulated frames (four seconds at 60 fps), up to three. A pull with
no charge gives her a weak, throwable crime shard instead and deals 2% self
damage. The snack meter is unrelated to her down-B `crimes` state.

The eight supplied 46x46 source images are in
`examples/rook_workshop/items/snack0.png` through `snack7.png`, in this order:

| Index | Item | Provisional throw behavior |
| --- | --- | --- |
| 0 | ChuftyCookie | Weak, ordinary arc (strawberry-like). |
| 1 | MuscleMilk | Short arc; breaks open and leaves a food pickup. |
| 2 | FortuneCandy | Drops, then travels along the ground briefly. |
| 3 | SportsDrink | Slow outward-and-returning boomerang path. |
| 4 | MindBone | Short arc, strong hit. |
| 5 | GummyShark | Moves forward, then curves upward off screen. |
| 6 | BirdSeed | Straightforward throw. |
| 7 | PHYSICS_OBJECT | Erratic path and long hitstun. |

These are behavior sketches from the 2021 concept notes; damage, knockback,
speeds, food-healing amount, and action timing are provisional tuning values.
The crime shard needs a temporary icon of its own. Placeholder pull animations
are acceptable until Rook's animation work catches up.

Implementation boundaries:

- Item definitions, visuals, and throw behavior must travel in the fighter
  package. Loading icons from rook.zone at play time would break offline play,
  rollback parity, and automatic sharing.
- Add a generic authored holdable-item operation to the scripting language and
  a native item definition/instance path. Rook's script owns charge selection,
  random choice, and self damage; item callbacks own held/thrown/dropped state.
  Do not route this move through an old Rook handler (the MexTK neutral-B
  callbacks were empty) or reinterpret down-B's `crimes` variable.
- The item's stable identity, subtype, timers, velocity, and hit data must be
  part of game state saved for rollback and replay. Drawing must not advance
  them. Package-owned visual data is immutable while a match runs.
- Verify pull timing, all eight thrown behaviors, empty-charge shard and 2%,
  passive recharge, holding/dropping/throwing, item collision, workshop
  preview, and deterministic replay before treating this as playable.

The neutral-B script and nine item declarations now live in
`examples/rook_workshop/moves/005_NeutralSpecial.fms`. The generic scripting
syntax is described in `docs/native_fighter_items.md`. The eight source icons
are not yet rendered; all nine items use a temporary common capsule model.
The provisional item behavior has built and been installed, but the full
manual gameplay and replay acceptance list above remains to be checked.
