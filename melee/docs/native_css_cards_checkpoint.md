# Native CSS cards and fighter delivery

Updated: 2026-10-17.

The online character-select screen shows one card per active lobby seat, up to
four. Local players appear first. Each card retains its room peer, local seat,
physical controller port and canonical match port. Changing presentation order
preserves the match's ports and colors. Waiting players and spectators have no
match card.

## Player controls

Place the character cookie to choose a fighter. It can be left anywhere on CSS.
An empty hand can then grab an installed fighter's portrait. Drop it onto a
remote player card to open delivery confirmation, or press X for the recipient
list. B cancels. The recipient list has page controls for larger lobbies.

Mouse clicks and drags move the stock CSS hand. The pointer intersects the menu
plane through its current camera, so the hand, cookies and cards follow the same
scene transform. Button transitions are retained in a fixed queue. Dragging a
cookie places it on release; clicking it picks it up for another click to place.
Original offline CPU, costume and slider controls remain available.

The recipient accepts a fighter from Workshop. The existing transfer validates
and installs the package, updates the roster and refreshes portraits. Sending an
already installed package preserves one installation.

## Source ownership

- `native_css_participants.c` builds the card list from room seats and local
  choices. Its fixed result stores presentation order and original identities.
- `native_css_cards.c` loads the stock panel backgrounds, emblems, door branches
  and human/CPU indicators. The original animation data selects their appearance.
  The parts render in HSD's opaque, texture-edge and translucent passes. Portraits
  and labels follow model drawing, with GX state invalidated at the boundary.
- `native_menu_portrait.c` reads a fighter package's portrait and title, or a
  stock portrait prepared from the user's ISO. Decoded images live until the
  choice changes or the scene closes.
- `native_css_original.c` masks the original player-card subtrees while the
  online presenter draws the participant cards. It restores authored visibility
  before the first cursor update and only restores nodes it masked. This keeps
  later stock visibility changes, including hidden portraits and team labels.
  Offline cards retain their normal controls.
- `native_css_interaction.c` owns cookie and portrait carrying, pointer events
  and target selection. Delivery opens the hub with a package ID and peer ID.
- `native_home.c` publishes provisional CSS choices with readiness clear. The
  existing practice transition confirms the selection.
- Fighter receipt adds a tile to the saved roster even while practice is active.
  Existing tiles retain their position. Each accepted delivery is handled once,
  including repeated deliveries of the same package.
- `native_netplay_connection.c` closes gameplay routes when the local player
  leaves the active set. Both endpoints then create fresh ICE sessions when
  that player returns. Descriptions that arrive after their route closes are
  discarded. This fixes the one-sided reconnection found during role testing.
- `native_netplay_content_channel.c` owns six content connection slots, separate
  from the three gameplay connections. Offers reserve a slot; accepting an
  uncached package starts ICE using the connection's existing STUN/TURN settings.
  Two slots cover gifts; automatic caching has one receive and three send jobs.
  The route records its peer, transfer serial and owner, so simultaneous transfers
  and delayed descriptions retain their identity. Role changes keep these routes;
  leaving or rejoining the lobby closes them. A failed content connection affects
  its transfer rather than failing the gameplay connection.
- Gift offers and replies travel through bounded room-signaling messages. Package
  bytes and transfer acknowledgements travel through the accepted content route.
  The recipient chooser permits every compatible lobby role. Starting a configured
  match clears pending local gifts; delivery remains available in ready practice.
- Lobby pumping drains stale gameplay datagrams before the next match. Content
  polling reads its own queues and leaves gameplay packets to their owner.

All presenter and pointer state belongs to the menu lifetime. These modules are
listed with host-owned state in the native build configuration.

## Verification

`native_css_participants_test` covers physical-port gaps, CPUs, shared-PC seats,
local-first ordering, unchanged canonical ports, malformed choices, active-seat
limits and waiting/spectator exclusion.

The hidden, muted `review_native_css.py` acceptance passed in
`.local/native-css-cards-offline-4`: custom choice, free cookie placement,
stable position, pickup, START's carried-cookie guard, a custom CPU, opening and
closing the detailed collection, and actual practice entry. Reviewed PNGs show
the custom portraits without old stock portraits or team labels behind them.

The two-client run in `.local/native-css-room-acceptance-6` passed local-first
cards, provisional choice updates, mouse cookie dragging, a portrait drop onto
the remote card and cancellation. The receiver entered ready practice, accepted
an uncached fighter through the delivery notice, then returned to CSS with the
new tile in its saved roster. The check verifies one package installation and
one tile after a repeated delivery. It also covers spectator card exclusion,
recipient availability and reconnecting after Fighting → Waiting → Spectating →
Fighting. Reviewed PNGs show stock card artwork, portraits and native delivery
confirmation on both clients.

The hidden, muted authoring run in `.local/native-css-cards-hub-2` also passed
empty first launch, starter cloning, rename replacement, independent copying,
opening and closing the Workshop with CRLF source preserved, profile saving and
return from CSS. The participant and lobby-selection CTest checks both pass.

The two-client run in `.local/native-css-content-acceptance-1` extends this with
an uncached delivery to a spectator during ready practice. Returning to CSS shows
the new tile with no spectator match card. Switching back to Fighting restores
the second card and permits another portrait-drop delivery. The duplicate
acceptance preserves one installation and one tile. Both clients run hidden and
muted with isolated profiles. Reviewed PNGs show the spectator's offer during
practice and the roster tile after returning to CSS.

`native_content_channel_probe` runs five native connection clients in a fresh
private lobby. Four have their full three-peer gameplay topology; the fifth has
no gameplay connections. The test passes binary controls, two simultaneous
content transfers with equal serials and different owners, an independent
gameplay packet, a waiting-to-spectating role change, a stale description and
member departure cleanup. Reservations allocate no ICE agent. The bounded
control queue and route pool reject overflow. The log is
`.local/native-content-channel-probe.log`. This proves local direct connections;
it does not establish physical multi-PC or forced-relay delivery.

The wire, participant, lobby-selection, netplay-input and content-transfer CTest
checks pass. The wire check covers distinct message kinds, exact lengths,
transfer identity, embedded terminators and binary control limits.

The existing `native_connection_probe` now uses the current rollback input-history
API and links the package validators from the runtime. It passes its real-room
check: shared-PC gameplay inputs still arrive over ICE after room signaling
closes. The log is `.local/native-content-gameplay-regression.log`.

CSS/practice rails are replaced by the native Ready, lobby, host-start and
delivery controls described in the [play-menu checkpoint](native_play_menu_checkpoint.md).

## Remaining work

Automatic pre-ready caching and portrait refresh are covered in the
[lobby-cache checkpoint](native_lobby_cache_checkpoint.md).

- Add the Quick Play queue controls with matchmaking.
- Complete Quick Play admission, ready practice, countdown, stage picks and
  rematches, then authored menu transitions.
- Recheck GameCube adapter movement with captured hardware samples. Mouse input
  does not establish the controller's range or deadzone behavior.
- Refine offline portrait layering around the original CPU controls as the
  remaining native CSS presentation is completed.

Account sign-in/recovery and offline inbox policies remain deferred for user
review. No physical multi-PC result is claimed by these local tests.
