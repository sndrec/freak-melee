# Native menu asset checkpoint

Updated: 2026-10-17. This records the stock-asset and caption integration passes.
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

The ribbon border mesh has separate end geometry and connecting triangles.
Owned vertex arrays move the ends apart or together while keeping their shape
and border thickness. The middle changes width. The selected ornament follows
the right end. Shared archive geometry remains intact.

The blank stock panel texture supplies reusable border corners and edges.
Its border coverage is extracted once when the scene loads, then drawn as nine
slices. Panels, fields and buttons use that border. Button fills use gold or
dark gradients, with separate selected, ordinary and disabled text colors.

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
  the new scene owner, caption placement and font interface. Latest local log:
  `.local/native-menu-caption-build-1.log`.
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
- Release payload checks passed all seven cases, including both new atlases.
- This pass uses local profiles and sends no public chat. It does not reverify
  online matchmaking, gifts or offline account features.

## Remaining work

The reconstructed glyphs and typography in the remaining CSS controls need the
user's visual review. The caption uses its original scene anchor; page content
still uses its 640-by-480 layout during the stock transition and is not yet laid
out through all original menu anchors.

Review the adapted lobby controls with the user, then carry the same component
rules through the other hub pages and CSS-specific controls. Recheck the
online flows after those changes. The working stock frame and sliced borders
do not establish final style acceptance for the whole hub.
