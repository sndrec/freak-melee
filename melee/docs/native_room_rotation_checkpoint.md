# Shared-PC seat promotion and rotation

Checked on 2026-10-18 with the current interpreted fighter runtime. Two hidden,
muted native clients use copied profiles and an isolated local signaling service.
The host has two local seats; the guest has one. The fighters are the received,
source-only Fox and Popo/Nana packages from the family-content fixture.

## Roles and owner override

The normal Lobby controls pass these checks:

- The host puts its second seat into waiting, with queue rank 1 on both views.
- The host changes the guest from fighting to spectating. The guest chooses
  fighting again; the override leaves the guest free to change its choice.
- The guest's attempt to change the host's seat leaves the host's role intact.
- The host cycles its second seat through spectating, fighting and waiting.
- Owner-selected rotation policies appear on the guest, whose policy control
  remains disabled.

Seat controls now have distinct inspection IDs for each participant and seat.
Their labels, positions and behavior are unchanged. Read-only room inspection
reports active/waiting masks, queue ranks, confirmation and match epochs.
Read-only match inspection reports each canonical fighter's physical controller
and position.

## Two consecutive matches

1. **Winner out:** host seat 0 fights guest seat 0. Host seat 1 waits. Input on
   the waiting controller leaves the active fighter still. Host controller 0
   moves its fighter from X=-60 to X=-44.100903 while the guest remains at X=60.
   The guest loses its one stock. The host wins and moves to waiting; host seat 1
   becomes active, and both clients return with readiness cleared.
2. **Winner stays:** the promoted host seat 1 fights the guest. Canonical port 0
   maps to physical controller 1. Input on controller 0 leaves it still;
   controller 1 moves it from X=-55 to X=-44.870274. The guest remains at X=60.
   The host loses, so the guest stays and host seat 0 is promoted from waiting.

In each Results screen, START from the host's waiting controller leaves that
screen open. Its assigned active controller advances it. Both clients return
to the same lobby and observe the same new seat masks and cleared readiness.

Both rounds compare confirmed peer checksums and exercise delayed-input
checkpoint reconstruction. Their final peer endpoints match exactly:

| Round | Frame | Fighter digest | RNG |
| --- | ---: | --- | --- |
| Winner out | 573 | `9457ae7c5be91702` | `edf2c4a3` |
| Winner stays | 627 | `7f2a10d9fee2beb7` | `43d0e45c` |

Driver: `tools/review_native_room_rotation.py`.
Receipt and input hashes: `.local/native-room-rotation-2/receipt.json`.
Final endpoint/hash audit: `.local/native-room-rotation-2/audit.json`.
The first attempt passed its first round, then asked for a Lobby-opening button
while the Lobby was already open. The corrected driver handles that return state.

These are virtual-controller tests on one Windows PC. Physical shared-PC
controllers, separate machines/networks and relay routes remain external
acceptance. Spectator roles are covered here; watching a match and catching up
to its live input stream still need implementation. The agreed viewing flow
keeps local practice running until the player chooses **Watch match**.
