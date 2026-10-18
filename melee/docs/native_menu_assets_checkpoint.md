# Native menu asset checkpoint

Updated: 2026-10-17. This records the first stock-asset integration pass.
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
labels. Existing notice, results and CSS tile faces retain their separate uses.
Release packaging includes the new atlas.

`--retail-menu` starts the original Main Menu through its existing forced-main
entry path. `review_native_menu_assets.py` captures Main Menu, 1-P, VS and
Options references before entering the adapted hub.

## Verification

- Native client, content tool and skin-weight target built successfully with
  the new scene owner and font interface. Local log:
  `.local/native-menu-scene-build-6.log`.
- Retail references and the adapted hub were captured in one hidden, muted,
  isolated client: `.local/native-menu-assets-2`. The screenshots were reviewed.
  That capture exposed an invisible-gradient bug, corrected in build 6.
- The corrected build passed forward/back navigation, ignored clicks during
  transitions, profile editing/return, CSS to practice, opening the hub in
  practice, and practice back to CSS. Screenshots were reviewed under
  `.local/native-stock-menu-scene-3`.
- Release payload checks passed all seven cases, including the new atlas.
- This pass uses local profiles and sends no public chat. It does not reverify
  online matchmaking, gifts or offline account features.

## Remaining work

The large page caption still needs its original typeface, placement and scene
anchor. Section headings and notice sizing need a context-by-context visual
pass. Current page content still uses its 640-by-480 layout during the stock
transition; it is not yet laid out through all original menu anchors.

Review the adapted lobby controls with the user, then carry the same component
rules through the other hub pages and CSS-specific controls. Recheck the
online flows after those changes. The working stock frame and sliced borders
do not establish final style acceptance for the whole hub.
