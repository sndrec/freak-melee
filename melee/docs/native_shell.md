# Native shell

The client opens at 1440x810 with Melee aspect-fitted to the centered 1080x810
region. Aurora renders the RmlUi document over the entire native surface;
the left and right panels occupy only the 4:3 game's side margins. Neither
Melee's projection nor its camera bounds are widened. Below 140 density-
independent pixels per margin, panels hide instead of obscuring the game.

Edit `assets/ui/freak_shell.rml` and `freak_shell.rcss` to change the layout.
Building `melee_native` copies these files beside the executable every time,
including when only UI assets changed. Aurora resolves them relative to the
executable, so the existing root launch batch file still works. Fonts are loaded
from the Windows Fonts directory: Segoe UI regular/bold and Arial bold italic.

## Working interactions

- Room sidebar: name, one to four local players, Create, room-code entry, Join,
  Copy code, Start match, and Leave.
- Codes normalize lowercase and whitespace before joining. Invalid codes show
  an inline error. Input names are constrained to the signaling protocol's byte
  limit rather than being cut halfway through a UTF-8 character.
- Presence displays real room participants, host ownership, roles and local
  fighter counts and Ready/Selecting state. The host can start once two to four
  assigned fighters are ready in practice. Offline Start needs two local fighters.
- The local-controller list reports actual port assignments and assigned
  profile names. It does not infer that a controller belongs to a person.
- Room actions are available at home character select and in local practice.
  START in CSS locks the local choices and enters practice. Start match in the
  sidebar opens stage select; guests wait for the host. Text entry uses the
  keyboard; Enter submits a room code and Escape leaves the field.

`native_online_menu.c` queues UI intentions consumed by `native_home.c` in the
existing game loop. Home readiness and stage selection use signaling outside
combat rollback. The old online CSS and VS-menu room hooks are removed.

Shell state is explicitly excluded from the game rollback section. The UI is
updated only on presented frames, and its rendering uses the existing Aurora
render handoff. The render submission owns the overlay viewport and bind group;
the render worker does not consult mutable RmlUi document state.

## UI diagnostics

The existing native control channel also accepts:

```powershell
build/native/melee_native_control.exe ui-click 1350 252
build/native/melee_native_control.exe ui-text "Player name"
build/native/melee_native_control.exe --id guest ui-click 1350 386
```

Coordinates are native client pixels in the RmlUi context. Clicks use ordinary
RmlUi hit testing and event dispatch, and text goes to the focused field. These
commands do not move the desktop pointer or require window focus. They do not
test Windows mouse event delivery; the user separately confirmed normal mouse
clicks work in the new build.

`tools/capture_native_window.ps1 -GameProcessId <pid> -OutputPath <file.png>`
captures the complete window, including both native panels, with PrintWindow.
F12 and the native-control `screenshot` command capture the full presented
game-and-panel surface as PNG under `screenshots/`.

## Initial shell evidence, before home integration (2026-09-22)

- Release client, native control tool, and default targets build successfully.
- All 353 discovered tests pass serially. Parallel execution exposed a shared
  temporary-directory collision among Aurora's existing I/O tests; the full
  serial run passes. No I/O implementation changes were made for this UI work.
- Screenshots inspected at 1440x810 and 1280x720, plus a 960x720 client showing
  the panels correctly hidden at 4:3. No RmlUi font or style warnings remain.
- A real sidebar Create joined room `SKSU5JEF` as `UI Host` with two local
  fighters. A second client entered lowercase `sksu5jef` as `UI Guest` with
  one fighter; both displayed the same room code and both participants.
- Clicking Character select on each client entered the original CSS. This is
  new UI/setup evidence, not another completed-match or cross-network test.
- Earlier one-player Create/Leave returned to the disconnected form correctly.

## Remaining UI work

Sidebar spacing was tightened on 2026-09-22: horizontal padding is 8dp per
edge instead of 16dp, with reduced vertical padding. The full-window
`build/native/sidebars-tight.png` capture was reviewed. The centered 4:3 game
viewport is unchanged. The implemented home CSS/practice flow and its remaining acceptance are
described in `native_home_flow.md`; normal startup now enters home CSS.

Collection remains visibly disabled. Options opens a separate full-window
profile editor with Gameplay, Bindings, and Calibration pages, a large Back
button, and a slide-in transition. Match rules opens its own full-window page
for Time/Stock/Coin, timers, stocks, damage ratio, item frequency, the original
31 item switches, friendly fire, pause, and score display. Changes are saved as
installation defaults and drive the next local or host-owned online match. A
room guest cannot edit the host's rules. Stage selection policy is still outside
this page. See `native_profiles_ui.md` for profile evidence and remaining
acceptance. The rails remain for in-session information and quick actions;
management screens use the space their tasks need.
Global chat is now connected through its own application-lifetime WebSocket,
independent of room signaling and match ICE. The panel supports recent history,
online count, acknowledged sending, reconnect and collapse. See
`native_chat.md` for implementation evidence and remaining limits.

Waiting/spectator management, controller-only text
entry, the collection editor, fuller profile acceptance, and room recovery after matches remain open.

The item-switch UI was inspected at 1440x810 on 2026-09-23. A live click
disabled Bob-omb, Save wrote an item mask ending in `...FFFE`, and a clean
restart reopened the page with Bob-omb off and 30 of 31 items selected. The
settings and room-match serialization tests pass. The original switch table's
unused bit 28 is omitted. This checks selection, persistence, and the existing
match packet path; an actual multi-client match with a custom mask remains to
be played.
Room management is available in home CSS/practice. Home CSS cannot return to
the original menu hierarchy. Narrow-window and higher-DPI layout
improvements remain future UI work rather than changes to gameplay.
