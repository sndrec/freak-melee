# Native menu asset checkpoint

Updated: 2026-10-17. This records the stock assets, camera and dynamic lettering.
The current requirements are in [the social hub design](native_social_hub_design.md).

## Current implementation

Asset edits require a specific base-game reference and a before/after comparison
showing closer agreement with that reference. Stock shapes and ornamentation,
including the navigation ribbon's interior cutout, are intentional artwork.

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

All seven faces now decode UTF-8 and cover accented Latin letters and common
punctuation. Captured letters supply the bases for additional accented forms.
Folk supplies its own accents where available. Common composed and decomposed
spellings share the same glyph and width. Unsupported characters display a
single question mark. The atlas records carry Unicode values and vertical
offsets so accents can extend above the cap line without moving ordinary text.
ASCII retains its original artwork and advances. Runtime lookup storage is
fixed; texture storage is acquired with the menu fonts.
Font references, lookup tables and texture pointers are host-owned UI state,
outside the rollback game-state section.

The duplicate page heading was removed. Navigation sits below the caption, and
section labels use the menu-option face. Descriptions and ordinary notices keep
their notice face; the footer notice is larger.

`--retail-menu` starts the original Main Menu through its existing forced-main
entry path. `review_native_menu_assets.py` captures Main Menu, 1-P, VS and
Options references before entering the adapted hub.

## Verification

- Native client/content tool and UTF-8/chat/signaling test targets built with
  the expanded fonts: `.local/native-menu-unicode-build-5.log`. The control
  tool now converts Windows UTF-16 arguments to UTF-8 before sending text;
  its build is `.local/native-menu-unicode-control-build-1.log`.
- The four CTest checks `native_utf8`, `native_chat_wire`,
  `native_signaling_wire` and `native_menu_fonts` passed. The decoder check
  covers all Unicode scalar values, truncated prefixes and malformed encodings.
  Receipt: `.local/native-menu-unicode-ctest.log`.
- `menu_font_test.py` verified all seven generated font files, including every
  GPU texel against its inspection PNG. Each face has 328 or 329 glyphs and
  161 composition pairs. The optional pre-change ASCII reference confirmed
  identical artwork and advances for all 95 ASCII entries in every face.
  Receipt: `.local/native-menu-unicode-atlas-check.log`.
- One hidden, muted client passed real glyph selection and equal measurements
  for composed/decomposed accents in all seven faces, unsupported-scalar
  fallback, UTF-8 backspace, profile saving and page return. Reviewed PNGs and
  measurements are in `.local/native-menu-unicode-review-2`. This uses an
  isolated profile and sends no public chat. The client was stopped afterward.
- The final build classifies font storage with `MELEE_ROLLBACK_HOST_STATE`.
  The hidden, muted CSS/practice/hub round trip and forward/back/profile
  transitions passed in `.local/native-menu-unicode-motion-1`, with the final
  returned-menu screenshot reviewed. These checks exercise font release/reload
  across scene changes. The test client was stopped afterward.
- Native client, content tool and skin-weight target built successfully with
  the new scene owner, caption placement and shared skin. Camera-pass log:
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

## Typography feedback, 2026-10-18

The left navigation now uses the original ribbon lettering's animated ink:
gold on idle ribbons and black on selected yellow ribbons. Selected black text
uses its fill without the expanded outline. The labels are drawn with the
ribbon, and get their colour from the original lettering asset.

Letters recovered from different menu textures now share a baseline in both
the button and page-title alphabets. Lowercase descenders and Q's tail keep
their extension below that line.

The original main menu was captured in
`.local/native-menu-type-reference-1`. The corrected Workshop and Lobbies
screens are in `.local/native-menu-type-fix-3`; all three references were
visually reviewed. Eight focused tests pass in
`.local/native-watch-clock-build-1.log`. These changes still await the user's
visual review.

## Remaining work

The reconstructed glyphs and typography in the remaining CSS controls need the
user's visual review. The caption uses its original scene anchor; page content
uses its 640-by-480 authoring coordinates on the original frame surface.

Review the adapted hub and CSS controls with the user. The stock frame, sliced
borders, camera-aligned hand and local online checks do not establish final
style acceptance or the feel of physical controller navigation.
