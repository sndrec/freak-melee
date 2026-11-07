# Startup review: next implementation pass

Updated: 2026-11-07. This list records the user's second wizard review.
The goal is active. Work through these changes, then pause for another hands-on
pass. Commit locally; do not push.

## First batch: implemented and checked

The welcome screen has only the temporary Freak Melee text logo, startup
indicator and Continue. Profile fields use separate positions, the empty name
field has a faded Player example, account-management buttons stay out of the
completed setup account page, and the controller button says Select controller.

The calibration capture stores a 32-direction outline for each stick. The
calibration page draws those outlines; the review page displays scaled axis
values and raw samples. Measurement pages suppress hand motion and menu tilt.
The pre-click trigger capture and adjustment controls remain to implement.

A render-order error caused stock portraits to reappear even when the saved
layout contained only a custom tile. The CSS now applies its visibility mask
at its draw boundary, after cursor/animation updates. The native capture after
cloning shows one custom Mario tile and no stock roster. Existing saved layouts
are preserved. Missing native-home layouts start empty.

Build: `.local/startup-review-batch-1-build-2.log`. Five focused input/profile
tests pass in `.local/startup-review-batch-1-contracts.log`. The four-check
setup walkthrough passes in `.local/startup-review-batch-1-ui/receipt.json`;
welcome, profile and returned CSS captures were inspected. The walkthrough
skips physical calibration and does not create an online account. Account
error popups, trigger click limits and the new starter flow remain open.

## Screens and messages

- Welcome contains only Freak Melee, FIRST STARTUP with four progress dots,
  and Continue. Use text for the logo until the real logo is designed.
- Give the pages distinct compositions. Use larger captions and buttons;
  display name and bio may occupy opposite sides. Keep the progress indicator.
- A fresh display-name field is empty. Show a faded Player example while it is
  unfocused. Clicking it starts with empty text.
- Remove the account page's Optional paragraph. After signing in and saving
  recovery information, show the identity and Continue. Account management stays
  in Settings.
- Show failed account requests, including an occupied name, in a reusable
  dismissible notification using the original unlock-notice presentation.
  It slides in from the side and consumes the dismissing button press.

## Controller calibration

- Rename the controller confirmation button to Select controller.
- Suppress stick-driven cursor movement and menu tilt during measurements.
- Show the measured stick outlines and numerical readings during review.
- Capture a trigger's analog maximum from the last sample before its physical
  digital click, with a slow squeeze and later numerical adjustment. Controllers
  without a digital click use full analog travel. Distinguish physical clicks
  from generated button thresholds.

## First fighter and CSS

- Replace the starter's button grid with a rotating carousel inspired by early
  DDR selection screens and Melee's trophy gallery. Show standing idle models,
  proper fighter names and short descriptions. Keep paired fighters together.
- Confirm the base fighter before cloning. Do not ask for a new fighter name in
  this selection step.
- Then frame the CSS portrait with a camera, write the short tile text, and
  place the tile directly on the actual CSS.
- A fresh CSS begins empty and contains only the tiles the player places.
  Investigate the stock portraits shown after the previous wizard run, including
  render-time visibility and saved-layout behavior. Preserve existing layouts
  unless the user explicitly replaces them.
- Offer a secondary Fill CSS with Melee Defaults action. Confirm with the exact
  question: Populate the Character Select Screen with the default SSBM roster?
  Show empty and populated CSS previews with an arrow between them.
- That shortcut creates editable clones and reproduces the original tiles as
  closely as possible. Leave Kirby out while his scripted clone is deferred.
- Revisit the CSS's Ready/START, Workshop and Back placement as part of the
  placement/finish flow.

## Intro: storyboard together after the playable flow

The user chose to finish the playable flow before producing the cinematic.
Preserve these storyboard beats for the later discussion:

1. A fighter starts as a faithful copy. Show editing hitboxes, animation and
   scripts, with gameplay from cinematic camera angles.
2. Show online sharing and the unrestricted possibilities: an overpowered
   low-percent KO and a censored appearance example.
3. Show disabling custom appearances: a censored fighter becomes a neutral
   wireframe based on hurtboxes, then falls asleep. Implement the appearance
   filter before presenting it as an available setting.
4. Show several Falco outfits dropping in and taunting. Appearance changes are
   shared even when moves stay unchanged.
5. End with Be prepared for anything in Freak Melee.
