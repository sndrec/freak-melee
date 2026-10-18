# Spectator match packages

Inactive room members can download the exact fighter and outfit packages used
by an active match. A viewing request keeps those package IDs and their owning
peers pinned while the room's current selections change. The viewer can finish
its downloads through Results.

## Active uploads

At match preparation, each participating PC reads and verifies one copy of each
unique package owned by its local slots. Up to eight entries cover four fighters
and four outfits. Storage uses the packages' actual sizes and sits outside the
copied simulation arenas. All spectator uploads borrow these immutable bytes.
They allocate no package copies and perform no file reads on the active sender.

The retained bytes survive Results. Starting the next match or closing the room
first retires any uploads that borrow them, then releases their storage. If
retention fails, the active match continues and that PC cannot supply missing
packages during gameplay.

Uploads use the existing separate content connections. Each connection sends
up to eight packets per upload pump, with at least 4 ms between pumps. The sender
has a fixed pool of three concurrent cache uploads; excess requests retry.
Gameplay input processing runs before this auxiliary work. Opening new upload
connections is also bounded to one per pump.

## Readiness and ownership

The receiver verifies the package's content hash and canonical structure before
adding it to the cache. Downloading creates no installed marker and adds no CSS
tile. Explicit acceptance remains the action that adds a fighter to the roster.

`nativeOnlineSession_WatchContentReady()` requires a validated match description
and cached entries for every required fighter and outfit. It checks fixed cache
state without file access. Game scene construction must still validate and
activate the models, animations, scripts and resources before playback.

The reliable transfer receiver now stores valid out-of-order chunks directly
in its final package buffer. A 32-bit window tracks chunks ahead of the received
prefix. Filling a hole advances through those saved chunks without sending
them again. Conflicting duplicates fail the transfer. This adds no secondary
chunk buffer and preserves the existing packet format.

## Checks

The cache fixture uses the actual cache and reliable-transfer state machines
with controlled transport and storage. It makes source files unavailable after
setup, verifies shared uploads without further source reads, clears active
selections mid-transfer, finishes the exact outfit dependency, checks readiness
and releases retained storage.

The transfer fixture covers loss, duplicates, reordered chunks, a short final
chunk, acknowledgements beyond a retransmission cursor and conflicting data.
The sender tracks its highest sent byte separately from its retry cursor so
those cumulative acknowledgements remain valid. The existing gift, input, match-history and viewing
transport tests are included in the same batch.

`tools/review_native_watch_content.py` runs two active native clients, then joins
a third PC whose cache lacks both required source families and their outfits.
The third PC practices with an unrelated fighter while receiving match history
and packages. The driver checks exact hashes, cache-only ownership, source read
counters, active-upload counters, peer checksums and the final history digest.

The 2026-10-18 run passes with received source-only Zelda/Sheik and Popo/Nana
families and their coordinated outfits. The late PC begins with none of those
four packages, joins as waiting and practices with its unrelated Fox clone.
It downloads and verifies 22,098,270 bytes. All four IDs remain absent from
installed markers and its CSS layout. Each active sender retains two packages;
its cache file-read counter stays at two while both requested packages are sent.

The authority performs 6,195 restores and 43,366 replay ticks. Both active peers
end at frame 6,206 with fighter digest `1eae06690778d299` and RNG `5add78b8`.
The received history has 6,207 records and the same rolling digest as the
authority, `6d95945a37a14457`. The spectator stays in practice through completion.
The match and practice screenshots were inspected.

Cold preparation takes 93.344 seconds measured from joining through entering
practice and receiving the dependencies. This proves functional delivery and
ownership; transfer throughput still needs improvement. It is not performance
acceptance for concurrent content, chat and spectators.

Receipt: `.local/native-watch-content-runtime-2/receipt.json`.
Source/binary hashes, package checks and endpoint audit: the adjacent `audit.json`.
Six-test batch: `.local/native-watch-content-build-4.log`.
The earlier functional run passed with slower delivery. The loss fixture then
exposed acknowledgement validation against the retry cursor; the final batch
passes after separating that cursor from the highest sent offset.

Game reconstruction, live playback, the Watch match control, source validation
at scene construction and auxiliary-load performance acceptance remain open.
