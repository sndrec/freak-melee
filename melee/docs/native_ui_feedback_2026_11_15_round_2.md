# Native UI feedback: second batch, 2026-11-15

This fixed batch contains 11 submissions and 16 pinned notes. All are implemented, verified and removed from the hosted feedback list. Captures, diagnostics and removal receipts are stored privately in `.local/feedback-review-2026-11-15-round-2/`.

## Agreed behavior

- 1v1 Quick Play uses all configured local hands and tokens. The most recently placed token claims the local seat; a handoff unreadies it and cancels a countdown.
- Player-created lobbies automatically begin a three-second countdown when all active fighters are ready, followed by stage selection using the existing loser-picks policy. Waiting players and spectators do not count.
- START toggles readiness and cancels the countdown. In online practice, START unreadies and pauses; another START resumes and readies. The normal paused L+R+A+START exit returns to CSS. B retains its ordinary character-select and practice controls.
- Queue entry happens on CSS before START enters practice. Practice shows matchmaking status and fading incoming chat; the normal paused match-exit command returns to CSS.
- Preserve the vanilla CSS card aspect ratio. Twilight chose to fill the row width, keep the existing top edge and chat width, and leave a small bottom gap.
- Implement the stock stage-selection policies too; default to Loser to preserve the current lobby policy.
- Use the original versus Rules screen and extend it for lobby controls. Inspect stock menu assets and animations directly.
- Keep automated game clients hidden and muted. Ask or wait for a user-owned client that locks the build. Preserve unrelated files, commit/push coherent changes, and do not publish a release.
- Delete a hosted submission and both attachments only when all its notes are implemented and checked.

## Submissions

### `62f5cda4-5002-42f0-afa2-50b352000345`

- [x] Pin 1: Why did we make a totally custom menu for these settings?! The vanilla game has a match rule settings menu! We should be extending that for lobby-related settings too.

### `cd377f53-d66a-4534-98c5-e07853fe5e8a`

- [x] Pin 1: Why is this a separate menu you get to from Settings -> Online, instead of just having all these buttons IN the Settings -> Online menu?

### `3837f208-4ad5-4146-8023-bc14d846a8fb`

- [x] Pin 1: Why do we not just have three separate Effects/Music/Master volume sliders here...? C'mon, guys...

### `75e9ede1-266d-43c8-85c6-ea48a8e39436`

- [x] Pin 1: There is a graphic here that says "NEXT SCREEN" - for now, we should probably just hide or remove it from every menu that uses this backdrop.

### `e944fc10-73be-41eb-99af-28dbed9a28fb`

- [x] Pin 1: There is no way to enter queue or press any of the buttons at the top of the screen here without using the mouse pointer. We probably don't want to even let the player do "READY / START" at the CSS until they've entered queue, and that way we don't need to permit any controls here except LRA+START to return to the CSS, and we can reserve the bar at the top of the screen for matchmaking status / etc.

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

- [x] Pin 1: Again, the vanilla game has an existing menu for adjusting game rules like Stock/Coin/Time which is ordinarily what the vanilla game opens when you press the '4-man survival test!' text at the top of the menu here, and we should be extending it to include this lobby stuff here instead of making a brand new menu.

### `d7c23f1d-e041-44ee-a77c-67419089f7a3`

- [x] Pin 1: On second thought, maybe what we *do* want is to have READY / START be the thing you do here, and have the match automatically begin when all players are ready (pre-empted by a 3 second countdown?)

### `27282fcc-2c8e-48ee-8fd4-05c303d9278d`

- [x] Pin 1: The original game menus that use these kinds of buttons give them variable horizontal offsets - please check the original game menus! They are not all laid out linearly with fixed widths and horizontal positions. I encourage you to experiment with positioning and sizing these kinds of buttons a little more creatively.

- [x] Pin 2: Same goes for the ones in the hub menu which use this same button styling.

### `a7ebc9cd-2f3b-4a7a-813e-88a918169a5d`

- [x] Pin 1: If joining quick play 1 vs 1 with more than 1 local player on the client, we should have a way for the local client to pick which player is playing in the quick play match. Maybe give them all cursors on the screen and in the CSS they each have their own grabbable token, and then the active player is just whoever most recently placed their token (shoving whoever elses token was on a character off?) Worth figuring out.

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

The first batch completed six submissions (11 pins). Their hosted records and attachments were removed, leaving five submissions at that checkpoint. Removal receipts stay in the private feedback directory.

### Stock Rules and extended-row presentation

The CSS header now opens the original versus Rules renderer, with the original red frame, row models, baked labels, number sheets, animated values and selection highlights. The Additional Rules page also uses its original renderer and labels. The native hub supplies the edited rules directly to those renderers. Stock retail callers were updated at their call sites. The duplicate Additional Rules data declarations were consolidated into their header.

Lobby listing, rotation and fighter/waiting/spectator choices now live under Additional Rules > Lobby settings. These extensions reuse the stock row models and label positions, with gray inactive labels, black selected labels and white values in the original value cells. The old dark Lobby rules panel and its dead input actions were removed. Mesh bounds use the engine's posed-geometry query so the hand hit regions and added lettering account for the artwork's skinning. Rule edits save immediately; the separate Save rules button and numerical-entry form were removed. The stock third self-destruct penalty value is accepted by local settings.

Hidden, muted reviews: `.local/native-ui-round2-stock-rules-review-4/` passed ordinary rule edits, held-A/B input, items, persistence after reopen, CSS header/return and two sizes. `.local/native-ui-round2-lobby-rules-review-2/` passed listing changes, rotation changes, host seat override, and the guest changing their own choice afterward. Screenshots of main Rules, Additional Rules and Lobby settings were compared with the captured stock Rules screen. The initial added-text layout was replaced after Twilight's feedback about sizing and alignment.

The two Rules submissions were kept open for the stage-selection work below, alongside readiness/countdown, local 1v1 token handoff and practice queue/chat. No release was published.

Final UI receipts for this pass: `.local/native-ui-round2-stock-rules-final-review/` and `.local/native-ui-round2-lobby-rules-final-review/` both passed on the rebuilt release client. Main Rules and Additional Rules constructors accept explicit edited rules and presentation options; original retail call sites retain their original visibility and SIS descriptions. The native menu exposes all additional options and retains Auto handicap after reopening. `native_settings` passed after rebuilding its test, including the third SD-penalty value. The six earlier report removals are confirmed by private receipts; a fresh hosted query found exactly the five pending submissions listed above.

### Local handoff and online practice

Quick Play 1v1 now keeps every configured local controller's hand and token on CSS. The latest placed token owns the single network seat and ejects the previous token into empty space. Only the chosen controller can ready that seat. Disconnected room controllers remain human seats instead of silently becoming CPUs. The CSS card and practice fighter use the chosen physical controller; returning from practice restores all configured hands.

Queue entry works through the controller hand. Ready and START remain unavailable until the client enters the queue. The native readiness check handles custom fighters and the selected seat directly; unused local tokens cannot block entry to practice. Online practice has a status-only top bar. Incoming lobby messages appear at the bottom, push older messages upward, and fade after ten seconds.

START readiness follows successful game pause/resume transitions: pause unreadies, resume readies. An input rejected during Ready/Go cannot change readiness by itself. B remains a gameplay button. The old B-countdown input module and its obsolete test were removed. The normal paused L+R+A+START exit returns to CSS, including after Quick Play assignment.

The hidden, muted two-client suite passed in `.local/native-ui-round2-quick-handoff-review-7/review.json` on the release build from `.local/native-ui-round2-handoff-practice-build-8.log`. It exercised four local controllers, handoff/ejection, held A/START, queue gating, physical-controller practice entry, pause/resume both before and after assignment, incoming chat order/fade, and normal exit. Handoff, practice status, floating chat, faded chat and returned-CSS screenshots were inspected. Submissions `a7ebc9cd-2f3b-4a7a-813e-88a918169a5d` and `e944fc10-73be-41eb-99af-28dbed9a28fb` are complete. Three submissions remained after this batch: shared Rules, stage-selection policies and the authoritative lobby countdown.

Both completed submissions and their attachments were removed; private `removal.json` receipts confirm each deletion. The production GM-loop snapshot fixture also passed after removing the old input hook: 2,019 state checks, 31,797 camera checks and a repeated 16-tick window (`A:/freak-melee-tests/native-ui-round2-pause-gm-loop-2026-11-15/receipt.json`). This fixture uses controlled external endpoints; the two-client review above supplies the live UI evidence.

### Shared Rules and countdown

The signaling service now stores lobby rules and the three-second readiness deadline. Active fighters determine readiness; waiting and spectating seats are ignored. Unreadying, changing rules, or changing the active roster cancels or restarts the countdown. The deadline and stage history survive service hibernation. The room rejects proposals that disagree with its selected stage or shared rules.

Stage Selection supports On (any active player chooses), Random (Random Stage Switch choices with the stock recent-stage/paired-stage cooldowns), Ordered (the stock stage-select order), Turns (active canonical seats in order), and Loser (the agreed previous-loser/fallback policy). Random and Ordered publish their automatic pick after the countdown. Turns and Random history advance when a match is committed. Fresh native settings default to Loser; an explicitly saved policy is retained.

The native client receives the rule set and countdown through a lobby-state subscription. The guest's Rules display uses those values without saving over personal preferences. Match proposals now carry the self-destruct penalty too. The normal lobby's separate Start match button was removed. Successful START pause/resume still controls readiness. A spectating owner can publish the agreed match while staying on CSS. Empty inactive local seats retain their controller mapping and create no practice actor.

Current checks: the full signaling suite passed 82 tests in 12 files (`.local/native-ui-round2-lobby-flow-full-tests.log`). Native signaling-wire, match and selection tests passed (3/3) after rebuilding. The release/control targets built successfully in `.local/native-ui-round2-lobby-flow-build-3.log`. The complete three-client review passed in `.local/native-ui-round2-lobby-flow-review-3/`, with the owner spectating on CSS, an empty waiting seat on the shared PC, and its second controller fighting. The loser received the next stage pick. The other four policies passed real match/rematch checks in `.local/native-ui-round2-lobby-flow-review-other-policies-1/`. These reviews checked countdown cancellation/restart, read-only guest rules, personal-settings preservation and match agreement. Guest Rules and countdown screenshots were compared with the stock menu reference. The final deployment and report-removal evidence is recorded below; no release has been published.


### Final audit and deployment

All 11 submissions (16 pinned notes) are complete. The final three hosted records and both attachments for each were removed only after the full live reviews passed. The private removal receipts confirm every deletion.

The signaling service was deployed from implementation commit `96d9175` as Cloudflare version `fbfd0768-98a4-4430-8491-3ae67d8a4a2a`. Its health endpoint returned OK. Two hidden, muted native clients then passed the full match/rematch flow against the hosted service: `.local/native-ui-round2-lobby-flow-hosted-review-2/review.json`. This checked shared Rules, START pause/unready and resume/ready, countdown cancellation/restart, canonical match agreement, actual results, and the previous loser's next stage pick. It uses two clients on this PC, not separate-PC acceptance.

The final service check passed 82 tests and the deployment dry run passed. Source changes and the local release executable are ready for Twilight's next review. The auto-updater channel and GitHub Releases were not changed. The goal is ready to pause after the final hosted-list and Git checks.
