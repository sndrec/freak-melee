# Remaining Zelda Item primitives: source audit

Read-only audit after primary model/capsule/child controls. The actual owned
source-check now stops at `examples/zelda_full.fms:802`. None of the operations
below is implemented by this audit, and none should delegate to a stock fighter
handler or legacy Definition.

## Current-owner debug display

The emitter in `tools/native_fighter_item_operations.c` calls
`db_80225DD8(gobj, item->owner)`, using current damage owner. In
`src/melee/db/dbitem.c`, a null/non-Fighter owner ORs the global collision-bubble
mode into `xDAA_byte`, without first clearing its low bits. A Fighter owner
clears only the low two bits and ORs `fn_8022697C(owner) & 3`;
`src/melee/db/dbanim.c` reads that from the Fighter debug flags. Preserve the
high bits and these distinct branches. Capture any nonnull owner in the real
pool/scene before native classifier access; foreign live Fighters are valid.
Suggested generic operation: `item.inherit_owner_debug_display()`.

## Stage collision mode

The later call at Zelda line 872 uses the Bool overload of `item.stage_collision`.
The current zero-argument binding selects `it_8026D9A0`. True should continue
using that body; false uses `it_8026DA08`. They select distinct map-collision
algorithms (`mpColl_800477E0` versus `mpColl_800471F8`). Both perform the actual
Item collision setup and copy current position; only a successful floor result
updates `xC30`. The script statement discards the native Bool result.

## Explosion feedback and data

- `item.rumble(pattern, duration)` uses `it_80273598` and the current owner.
  Null/non-Fighter owners are ordinary no-ops. Preserve native player eligibility
  and flags, signed widths and duration-zero mapping to -2. Bounds come from
  the actual `rumble_pattern_count`, not a guessed constant.
- `camera.quake(kind, x, y, z)` belongs to shared feedback. The native
  `Camera_RequestQuake` has defined kinds 1–4; other kinds include uninitialized
  length/index behavior. Zelda uses 2/3. Implement actual shared camera
  bookkeeping and quake objects rather than a fighter-specific effect shortcut.
- `item.creation_value` needs a saved, actor-owned birth scalar. Factories
  currently write the legacy `native_article` union before Created. Reading that
  overlapping Holdable storage is not the intended source-owned design. Default
  zero at Attach, initialize before Created, and preserve Spawned timing.
- `item.advance_animation()` is not merely a frame setter. `Item_802694CC`
  evaluates the HSD model, copies its current frame, then runs `it_802799E4`, the
  data animation-command interpreter. That command stream can create hitboxes
  and effects. Preserve ordering and source/root lifetime checks; compare
  representative commands as well as controlled animation stages.

The historical `item.current_owner_is_creator` name was misleading: its old
semantics compare current owner against revocable attachment control, which
detach clears. Current templates use `item.attachment_active`, implemented with
the actor's AttachmentOwner relation; the old spelling is not kept as an alias.

Complete this coherent primitive cluster before another whole Zelda preflight.
Zelda has no Chain resource declarations; the source-owned collection/model
resource closure is a separate Sheik task at line 1543. Native comparison leaves
must be explicit: debug globals, map-collision geometry, rumble input, camera
objects, and animation command data each have different boundaries.
