# Editable native character select

The CSS layout belongs to the installation, shared by all four local
players. Installed content and layout are separate: removing a tile hides it
from CSS but does not uninstall its fighter or costume. Other peers never need
this layout. Room messages carry the selected fighter and costume identities,
not tile positions.

## Current source boundary

Versus uses the original GX character-select scene in `mncharsel.c`. Its 25
`CSSIcon` entries bind stock fighters to menu joints and controller hit bounds.
`native_css_original.c` applies saved geometry to those joints and bounds after
the original scene builds them; the original player slots, animations, sounds,
and selection flow remain in use. `nativeHome_LockSelection` publishes the
selected stock or installed fighter identities when local players enter practice.

RmlUi draws the editor controls and translucent drag/resize grips over the
real GX stock portraits. Installed custom fighter portraits are decoded from
their content-addressed bundles at CSS entry and composited into the same 4:3
canvas at their saved tile coordinates. In edit mode the left rail expands,
the room/chat rail hides, and Aurora presents the same 4:3 game image beside
the editor. Leaving edit
mode restores the normal presentation margins. There is no second CSS roster.

## Selection and layout contract

- A fighter tile refers to a stable stock fighter or installed package ID. Its
  installation-wide geometry is independent of player port and room assignment.
  Custom package tiles have native portrait visuals and controller selection.
  A custom costume belongs under its fighter.
- Positions and sizes are normalized to the CSS canvas so a saved layout works
  at different window resolutions. Keep tiles inside the usable canvas and
  impose a minimum clickable size. Overlap is allowed because this is a freeform
  layout; explicit ordering decides which tile receives a click.
- The Add fighter drawer lists stock fighters removed from the CSS and
  installed native fighters with compiled definitions and
  decodable portraits. Cache-only packages remain unavailable there.
- Editing is a distinct mode: drag to move, drag a corner to resize, add from
  the collection, remove from the visible layout, and restore the default
  roster. Normal selection mode never edits layout accidentally. The original
  CSS retains controller navigation and the four local player slots.
- A player locks a specific fighter ID and costume ID. The selected package IDs
  are attached to that player's practice, room selection, match proposal, and
  replay. If any dependency is absent on an online peer, agreement waits for
  cache transfer; it must not silently choose stock content.
- The layout file is saved per installation outside replaceable builds, with an
  atomic replace after a valid edit. Corrupt files are reported and do not
  silently overwrite the previous layout. No per-profile copy is made.

## Implementation order

1. Persist normalized stock-icon geometry per installation and apply it to the
   original GX joints and CSS hit bounds.
2. Edit the original CSS in place: move, resize, remove, restore, and re-add
   stock portraits. Test persistence, controller selection, and window resizing.
3. Add native fighter definitions and behavior registration. Place package
   portraits directly over the original CSS canvas and let the existing editor
   manipulate those visible images. Extend the original cursor and match
   selection path beyond its fixed stock icon table.
4. Add installed-content browsing and costume selection, then exercise room
   selection, match launch, and replay with exact package identities.

Current status: `native_css_layout.c` provides fixed-capacity identities,
normalized geometry, edit operations, and atomic per-installation persistence.
The Versus home flow now uses the original CSS again. Live runs confirmed a
stock portrait moving and resizing in place, removal and re-addition, restart
persistence, selecting the moved Mario icon with the original controller cursor,
and returning from the expanded editor to the normal room/chat rails. The editor
hides the right rail and shifts the live 4:3 CSS right while its tools occupy
the widened left rail. Installed costumes are selectable from the home CSS;
the room setup transfers an absent selected costume to a peer's cache before
match readiness. The installed Rook bundle's PNG is decoded on CSS entry and
drawn in the empty third-row cell. A live editor drag moved the visible image;
remove and restore operations returned the installation layout to stock.
The original controller cursor now selects Rook's custom portrait. A live
isolated run entered practice with Rook, moved him, and returned to CSS with
his selection retained. The home roster publishes the fighter package ID;
online package agreement and match adoption remain open. The overlay is part
of the live original CSS canvas, not a second roster screen.
