# Selected fighters download before readying

Updated: 2026-10-17. Native build tag: `freak-melee-native-dev-cache`.

## Behavior

Joining a lobby and selecting a custom fighter publishes that choice to the
other members. Their clients automatically download its package into the local
cache. The character-select card replaces its placeholder with the fighter's
portrait and name when that download finishes. Selecting a different fighter
cancels an incomplete download for the old choice. A downloaded package stays
in the cache; adding it to the personal roster still requires explicit acceptance.
Selected costume packages use the same automatic path.

Waiting players and spectators can receive these packages, including while they
run ready practice. The native status line reports download progress. A change
to the source's selection removes obsolete jobs. Starting a configured match
releases these lobby transfers; match setup uses the existing verified cache
and its ordinary content preparation for anything still missing.

## Implementation

`native_netplay_room_cache.c` owns the bounded selection queue, one receive job
and three send jobs. It reads published selections and checks the content store
when a package first enters that queue. Received bytes pass the existing package
structure and requested-hash checks before they are written. Cache completion
increments a presentation revision; `native_css_cards.c` uses it to reload card
art even when the selected fighter itself has not changed.

`native_netplay_room.c` now drains content controls and content datagrams once,
then dispatches them to gifting and caching. The gift code owns its packet
handlers and its existing offer/accept/install behavior. The cache code owns
request/ready/missing/cancel/done controls. Both use the same reliable chunk and
acknowledgment protocol over dedicated ICE content connections. Gameplay inputs
keep their three separate routes.

The fixed content-route pool has six entries: two gifts, one cache download and
three cache uploads. Additional cache requests retry after a bounded delay.
Transfers retain their peer, serial and initiating owner. A shared local serial
counter prevents gifts and automatic requests from reusing the same identity.
Role changes preserve routes; leaving/rejoining closes them. An unavailable
package or failed automatic transfer retries without failing the room. A transfer
times out after 120 seconds without byte progress; larger packages can continue
for as long as data is advancing.

Only packages currently selected by an active local seat can be served by this
automatic path. Transfer storage belongs to the host online session and is
excluded from rollback state. No new network thread or hosted endpoint is added.

## Verification

The native client builds. Content store, chunk transfer, lobby match, lobby
selection, lobby topology, connection wire and CSS participant tests pass.
The five-client content-channel probe also passes with the six-slot bound,
including four active players, a waiting/spectating recipient, simultaneous
transfers, role changes, stale descriptions and departure cleanup.
Its log is `.local/native-lobby-cache-channels.log`.

`review_native_css_room.py` passed in `.local/native-lobby-cache-acceptance-1`.
Two hidden, muted clients use isolated stores and a fresh private lobby. One
starts without the other's fighter. Selecting it downloads the package and
renders the remote portrait before readying. Assertions verify that the roster
file and installed collection remain unchanged. The test then evicts only that
fixture cache entry and separately exercises an uncached explicit gift to a
spectator during practice, native acceptance, roster insertion, reconnection and
duplicate-install prevention. Reviewed PNGs show both actual character-select
portraits and their distinct package names.

`review_native_lobby_cache.py` passed in
`.local/native-lobby-cache-lifecycle-final`. It observes the first download in
progress, changes the source selection, and verifies that the old receive job
ends without writing that package. The recipient then becomes a spectator and
enters practice. A new selection downloads there, without an installed marker
or roster change. Returning to CSS shows the verified remote portrait.

The shared-PC gameplay connection probe passes after the cutover; its log is
`.local/native-lobby-cache-gameplay-regression.log`. It still exchanges validated
rollback inputs after closing the signaling sockets.
Physical multi-PC and forced-relay acceptance remain deferred.
