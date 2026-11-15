# Native UI feedback: second batch, 2026-11-15

This fixed batch contains 11 submissions and 16 pinned notes. Captures and diagnostics are stored privately in `.local/feedback-review-2026-11-15-round-2/`.

## Agreed behavior

- 1v1 Quick Play uses all configured local hands and tokens. The most recently placed token claims the local seat; a handoff unreadies it and cancels a countdown.
- Player-created lobbies automatically begin a three-second countdown when all active fighters are ready, followed by stage selection using the existing loser-picks policy. Waiting players and spectators do not count.
- START toggles readiness and cancels the countdown. B retains its ordinary character-select and practice controls.
- Queue entry happens on CSS before START enters practice. Practice shows matchmaking status and fading incoming chat; the normal paused match-exit command returns to CSS.
- Preserve the vanilla CSS card aspect ratio. Twilight chose to fill the row width, keep the existing top edge and chat width, and leave a small bottom gap.
- Implement the stock stage-selection policies too; default to Loser to preserve the current lobby policy.
- Use the original versus Rules screen and extend it for lobby controls. Inspect stock menu assets and animations directly.
- Keep automated game clients hidden and muted. Ask or wait for a user-owned client that locks the build. Preserve unrelated files, commit/push coherent changes, and do not publish a release.
- Delete a hosted submission and both attachments only when all its notes are implemented and checked.

## Submissions

### `62f5cda4-5002-42f0-afa2-50b352000345`

- [ ] Pin 1: Why did we make a totally custom menu for these settings?! The vanilla game has a match rule settings menu! We should be extending that for lobby-related settings too.

### `cd377f53-d66a-4534-98c5-e07853fe5e8a`

- [x] Pin 1: Why is this a separate menu you get to from Settings -> Online, instead of just having all these buttons IN the Settings -> Online menu?

### `3837f208-4ad5-4146-8023-bc14d846a8fb`

- [x] Pin 1: Why do we not just have three separate Effects/Music/Master volume sliders here...? C'mon, guys...

### `75e9ede1-266d-43c8-85c6-ea48a8e39436`

- [x] Pin 1: There is a graphic here that says "NEXT SCREEN" - for now, we should probably just hide or remove it from every menu that uses this backdrop.

### `e944fc10-73be-41eb-99af-28dbed9a28fb`

- [ ] Pin 1: There is no way to enter queue or press any of the buttons at the top of the screen here without using the mouse pointer. We probably don't want to even let the player do "READY / START" at the CSS until they've entered queue, and that way we don't need to permit any controls here except LRA+START to return to the CSS, and we can reserve the bar at the top of the screen for matchmaking status / etc.

We also probably want to have chat messages sent in the CSS by other players show up in this screen somewhere, like a source engine style chat (though stylize it like Melee obviously, just in the sense that the chat messages would appear floating, with newest messages appearing from the bottom and pushing older messages up, and then fade out after being on screen for long enough)

### `2c54aace-ae9e-4eae-9168-e894fc46fa5f`

- [x] Pin 1: While in a Quick Play lobby, clicking Create lobby here just takes you to your Quick Play CSS. It should take you OUT of quick play and have you make a normal multiplayer lobby.

### `b86f1e44-6e97-4dd9-bd12-322559c8c0e9`

- [x] Pin 1: The cards here should span more space; should be relatively flush against the left side of the screen, the bottom bar, and the left side of the lobby chat.

- [x] Pin 2: Left side flush.

- [x] Pin 3: Bottom bar.

- [x] Pin 4: Lobby chat.

- [x] Pin 5: Keep the current top height though.

### `424bbf0d-1bd4-426e-837b-c745889154be`

- [ ] Pin 1: Again, the vanilla game has an existing menu for adjusting game rules like Stock/Coin/Time which is ordinarily what the vanilla game opens when you press the '4-man survival test!' text at the top of the menu here, and we should be extending it to include this lobby stuff here instead of making a brand new menu.

### `d7c23f1d-e041-44ee-a77c-67419089f7a3`

- [ ] Pin 1: On second thought, maybe what we *do* want is to have READY / START be the thing you do here, and have the match automatically begin when all players are ready (pre-empted by a 3 second countdown?)

### `27282fcc-2c8e-48ee-8fd4-05c303d9278d`

- [x] Pin 1: The original game menus that use these kinds of buttons give them variable horizontal offsets - please check the original game menus! They are not all laid out linearly with fixed widths and horizontal positions. I encourage you to experiment with positioning and sizing these kinds of buttons a little more creatively.

- [x] Pin 2: Same goes for the ones in the hub menu which use this same button styling.

### `a7ebc9cd-2f3b-4a7a-813e-88a918169a5d`

- [ ] Pin 1: If joining quick play 1 vs 1 with more than 1 local player on the client, we should have a way for the local client to pick which player is playing in the quick play match. Maybe give them all cursors on the screen and in the CSS they each have their own grabbable token, and then the active player is just whoever most recently placed their token (shoving whoever elses token was on a character off?) Worth figuring out.

## Implementation and verification

Work in related batches: native menu presentation/settings; stock Rules and CSS layout; matchmaking, readiness, local handoff and practice status/chat. Record implementation commits, checks and per-report removal receipts here.

### Native presentation and settings batch

- Added independent Master, Effects and Music sliders with local persistence. Effects/music gains change presentation without changing decoder progress. Master gain is applied at audio output.
- Online account controls are directly on Settings > Online; its Back action leaves nested account tasks before returning to Settings.
- Hub and Settings ribbons have varied widths, heights and horizontal offsets, checked against native main-menu assets and captures. Native pages hide the stock NEXT SCREEN lettering joint; surrounding art remains intact.
- Creating a normal lobby first leaves Quick Play. Confirming creates a normal listed lobby, verified with a second local client joining it.
- Online CSS cards use one uniform 0.8 scale for geometry, portraits and lettering. They fill the available row width, preserve their top edge and the chat width, and leave the agreed bottom gap.
- Fixed two truncated child-pointer reads in the stock Rules digit renderer so the original menu can be inspected in the 64-bit client.

Verification: native release/control targets built successfully; audio mix/frame/resample tests passed (3/3). Hidden, muted client reviews passed for Settings (sliders, persistence, held-button input and two sizes), direct Online account controls (local fixture, not real-account authentication), and Quick Play to normal lobby with two clients. Uniform-card screenshots were inspected at 1440x900 and 960x540. Receipts: `.local/native-ui-round2-settings-review-1/`, `.local/native-ui-round2-online-review-1/`, `.local/native-ui-round2-lobby-review-uniform/`, `.local/native-ui-round2-audio-checks.log`. Actual stock Main, Versus and Rules captures: `.local/native-round2-retail-reference/`.

Six submissions (11 pins) are complete in this batch. Their hosted records and attachments may now be removed; removal receipts stay in the private feedback directory. Five submissions remain open.
