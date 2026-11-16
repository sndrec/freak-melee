# Native feedback: third batch, 2026-11-15

This batch contains six submissions and eleven pinned notes. Captures and diagnostics are private under `.local/feedback-review-2026-11-15-round-3/`.

## Stage-selection requests

- [x] Replicate the cursor, hovered stage and chooser identity to players watching stage selection. Only authorized active seats may control it; retain stage-policy and shared-PC mapping rules.
- [x] Automatic stage picks go directly into match preparation/loading. Random, Ordered and Quick Play automatic starts must not display the native stage-select scene.

## Feedback submissions

### `9c946ef7-1a04-44b8-86cc-ecc670ec5965`

- [ ] Pin 1: Should say 'Duplicate' to separate its functionality from 'Clone Fighter'.

- [ ] Pin 2: Should probably say "Clone Vanilla Fighter"

- [ ] Pin 3: This should be a button in the workshop editor itself, not here.

### `65e41944-be3d-48f6-8f03-402142d6a3b4`

- [ ] Pin 1: NONE of these buttons should live in the Attributes menu! Especially Build & Install, which should be visible at the top hotbar with the Save and Test buttons.

### `003b62dd-fc4c-4637-a90c-70849010d87d`

- [ ] Pin 1: After doing Warlock Punch (neutral special) ganon's cape starts flipping the fuck out. I think generally the code for these kinds of physically integrated joint chains seems off? Also I just realized that the render texture shadows for characters are also completely broken and missing LOL

### `78d0b72e-6e83-4a31-99ff-d35305207382`

- [ ] Pin 1: Wrong stock icons? I think we need to make sure that the correct stock icons are picked when cloning a character from a vanilla character. We also need a way for the player to upload or create these stock icons.

- [ ] Pin 2: Same for this icon - by default it's the smash bros logo, but we want to let the player define or upload it in the workshop or somewhere similar.

- [ ] Pin 3: Main body of percentage is dark? At 0% it should be white not grey

### `f7b8b1bd-f8cf-4d72-9774-07772335af81`

- [x] Pin 1: LRA+START here should take you back to CSS, not to the hub menu

### `b7ce2c09-b253-48b4-873f-88c6167bb64f`

- [x] Pin 2: Why are the other hands showing even though the lobby is empty and I only have one local player connected?

- [x] Pin 3: Copy code should probably be a "Copy connect code" button beneath Ready / START

## Working rules

Use the original Melee menu art and styling. Keep authoring features generic. Run owned clients hidden and muted, and never close a user-owned game to replace a locked executable. Finish and verify every note before deleting its hosted report and attachments. Commit and push coherent batches. Deploy required service changes without publishing a release or changing the auto-updater. Pause after the batch is complete for Twilight's next review.

## Implementation and verification

Record source changes, test receipts, captures, deployment and per-report removal evidence here.

### Stage flow and CSS navigation

The original stage cursor and hover highlight now replicate through the room
service. A chooser label follows the stage menu's camera. The service validates
the active seat, chooser policy, epoch and lobby-flow revision; movement has no
effect on readiness or the committed stage. Quick Play's rematch page uses the
same visual relay with its losing-team permissions.

Automatic starts use a separate match-setup scene after practice actors are
retired. Random, Ordered and Quick Play no longer pass through native stage
select. A normal paused practice exit requests CSS directly. CSS hands and
tokens now check configured local seats before drawing. The normal lobby's
copy-code button is labeled **Copy connect code**, below **Ready / START**.

The release client and control tool build. Native signaling-wire tests pass.
The signaling suite passes 85 tests in 12 files, including signed cursor codec,
chooser permissions, cancellation, stale rounds, hibernation and Quick Play
losing-side cursor delivery. All five lobby stage policies pass actual hidden,
muted match/rematch reviews, with a spectating owner and a shared PC whose only
active fighter uses controller 2. The review checks replicated cursor position
and hovered tile, and proves Random/Ordered never enter stage select. Evidence:
`.local/native-ui-round3-stage-review-1/review.json`,
`.local/native-ui-round3-service-tests-2.log`,
`.local/native-ui-round3-stage-build-3.log`.

The normal-lobby portion of
`.local/native-ui-round3-quick-stage-review-3/review.json` verifies a single
configured local seat, the copy-code button below Ready, and paused
L+R+A+START returning directly to CSS. Its captures were inspected. This run
also verified the Quick Play cursor after a real four-stock result; its final
rematch step exposed an outdated test assumption about a button that the
current UI closes automatically. The final Quick Play receipt is recorded
separately once the revised driver completes.

The final Quick Play review passes in
`.local/native-ui-round3-quick-stage-review-4/review.json`: configured hands and
practice exit, a four-stock game, losing-side cursor delivery, stage selection
and a second agreed match. Neither match enters stage select. Final build and
wire receipts are `.local/native-ui-round3-stage-build-4.log` and
`.local/native-ui-round3-stage-wire-tests.log`. The final service suite remains
85/85; its cursor tests also cover adjacent updates arriving together after
network jitter.

The signaling change is deployed as `f973e205-f438-4782-aeb7-c9e32edd1bd0`.
No client release or updater publication was made. The two completed reports
`f7b8b1bd-f8cf-4d72-9774-07772335af81` and
`b7ce2c09-b253-48b4-873f-88c6167bb64f` were removed with both attachments;
their private `removal.json` receipts have `deleted: true`.
