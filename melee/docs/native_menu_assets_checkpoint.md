# Native menu asset checkpoint

Updated: 2026-10-17. This records the stock-asset, caption and camera integration.
The current requirements remain in `PROJECT_PLAN.md` and
`native_social_hub_design.md`.

## Current implementation

The hub loads `MnMaAll` through the same archive decoder as the original main
menus. Each scene owns its archive buffers and imported descriptors. Existing
retail-menu call sites now pass their menu asset context directly.

`native_menu_scene.c` owns the hub's stock background, panel, camera, lights,
navigation ribbons and transition tracks. It loads `MenMainBack_Top`,
`MenMainPanel_Top`, `MenMainCursor_Top` and the original camera/light descriptors.
The outer frame and scrolling decoration use their authored animations.

The hub shares the original C-stick camera orbit with the retail menus through
`mncamera.c`. Each camera owns its resting descriptor, so opening the hub does
not change another menu's camera. The visible panel's joint 2 anchors the hub
content, navigation and hand. Its authored depth is 17 world units, close to the
caption's original joint at 17.2. Using the panel root instead placed the new
controls behind the frame and made them drift apart when the camera tilted.
Page transitions compose their authored transforms with this shared view.

Mouse positions are projected onto that same surface for hit testing. A
stationary mouse stays at its screen position as the C-stick changes the camera.
The controller hand moves in menu coordinates. Practice retains its own flat
overlay and camera; opening the hub restores the stock menu view, and closing
it returns to the practice camera.

The left navigation's ribbon mesh has separate end geometry and connecting
triangles.
Owned vertex arrays move the ends apart or together while keeping their shape
and border thickness. The middle changes width. The selected ornament follows
the right end. Shared archive geometry remains intact.

The blank stock panel texture supplies reusable border corners and edges.
`prepare_menu_skin.py` extracts its coverage into a 768-byte I4 asset at build
time. `native_menu_skin.c` holds that asset in fixed shared storage, acquired
by the hub and CSS/practice presenters independently of the main-menu archive.
Panels, fields and buttons use its nine slices. Button fills use gold or dark
gradients, with separate selected, ordinary and disabled text colors. Delivery
notices and the Quick Play countdown use the same border and menu lettering.

The new menu-option atlas extracts lettering from the English option textures.
Missing letters are reconstructed from the source proportions. IA4 stores the
white fill and dark outline together. A fill-only mask supports black selected
labels. A separate caption atlas recovers the italic page-heading face from the
main panel's baked strings. The generator separates the letters after removing
their shared shear. The renderer restores that shear and places the string
through the original caption joint and quad proportions, using the current
material color and opacity. Existing notice, results and CSS tile faces retain
their separate uses. Release packaging includes both new atlases.

The duplicate page heading was removed. Navigation sits below the caption, and
section labels use the menu-option face. Descriptions and ordinary notices keep
their notice face; the footer notice is larger.

`--retail-menu` starts the original Main Menu through its existing forced-main
entry path. `review_native_menu_assets.py` captures Main Menu, 1-P, VS and
Options references before entering the adapted hub.

## Verification

- Native client, content tool and skin-weight target built successfully with
  the new scene owner, caption placement and shared skin. Latest local log:
  `.local/native-menu-camera-build-6.log`.
- The camera build passed seven C-stick poses at three window sizes, projected
  button clicks, stationary mouse placement and profile editing/return:
  `.local/native-menu-camera-projection-2`. Neutral and tilted PNGs were reviewed.
  Forward/back/profile transitions, blocked transition clicks and the
  CSS/practice/hub round trip passed in `.local/native-menu-camera-motion-2`.
  Original Main, 1-P, VS and Options navigation and the adapted hub were also
  captured in `.local/native-menu-camera-retail-1`; screenshots were reviewed.
- The camera build's doubles check exposed a Ready click being consumed during
  the CSS entrance delay. The request now waits until START is accepted.
  The corrected build passed ready practice, B-to-unready, countdown restart
  and actual four-player netplay in `.local/native-menu-camera-quick-doubles-3`,
  with no session failure or checksum mismatch during the five-second input
  interval. An earlier retry hit Windows's paging-file limit; it is not counted
  as a successful run.
- Retail references and the adapted hub were captured in one hidden, muted,
  isolated client: `.local/native-menu-assets-2`. The screenshots were reviewed.
  That capture exposed an invisible-gradient bug, corrected in build 6.
- The corrected build passed forward/back navigation, ignored clicks during
  transitions, profile editing/return, CSS to practice, opening the hub in
  practice, and practice back to CSS. Screenshots were reviewed under
  `.local/native-stock-menu-scene-3`. The caption build passed the same checks
  under `.local/native-menu-caption-motion-1`; screenshots were reviewed.
- The caption build passed empty first launch, playable clone creation, CSS
  placement, rename replacement, independent copy, opening/closing the frozen
  Workshop preview with CRLF source, profile saving and controller return.
  Evidence and reviewed PNGs: `.local/native-menu-caption-authoring-1`.
- Two native clients passed the CSS and gift flow against an isolated local
  signaling service in `.local/native-menu-skin-room-1`. This covers local-first
  card order with canonical identities preserved, remote preview caching without
  installation, provisional selection, free cookies, mouse tile drop, cancel,
  uncached spectator delivery during practice, explicit acceptance and roster
  insertion, role changes and duplicate-install prevention. Received fighters
  opened as independent Workshop drafts and passed source validation after the
  sender disconnected and its authoring directory was moved aside.
- Shared-PC Quick Play doubles passed ready practice, B-to-unready, countdown
  restart and actual four-player netplay launch in
  `.local/native-menu-skin-quick-doubles-1`. After a five-second input interval,
  the test checked for process failure, session failure and checksum mismatch.
  Countdown and delivery screenshots were reviewed.
- Release payload checks passed all seven cases, including both new atlases and
  the shared border asset.
- These runs use isolated profiles and a local service, with hidden muted game
  clients. Test clients and the service were stopped. They do not establish
  physical multi-PC behavior, hosted deployment or offline account delivery.

## Remaining work

The reconstructed glyphs and typography in the remaining CSS controls need the
user's visual review. The caption uses its original scene anchor; page content
uses its 640-by-480 authoring coordinates on the original frame surface.

Review the adapted hub and CSS controls with the user. The stock frame, sliced
borders, camera-aligned hand and local online checks do not establish final
style acceptance or the feel of physical controller navigation.
