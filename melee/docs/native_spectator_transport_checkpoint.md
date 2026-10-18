# Spectator history transport

An inactive PC can now request the current authority's confirmed match history
over a separate ICE connection. The request is explicit. A room-wide Start
request leaves a PC with no active fighters in its local practice scene.
The room also keeps pumping during Results so viewers can receive final records.

This checkpoint covers history delivery. The
[cold-cache checkpoint](native_spectator_content_checkpoint.md) covers missing
fighter/outfit downloads. The player-facing Watch match control, viewer asset
activation and game playback still need integration.

## Delivery

The request names the current match session and uses a new connection serial.
The receiving PC checks the returned description against the room's published
match before accepting records. The description supplies the initial RNG seed,
rules and exact fighter and outfit package IDs.

Descriptions are split into 512-byte fragments. Input packets contain up to
six records. A cumulative cursor acknowledges the received prefix; a bounded
96-record window allows more than one packet to travel at a time. Missing
packets are retried after 250 ms. Duplicate and overlapping delivery preserve
the accepted prefix. The final record count and rolling digest must agree
before the receiver marks a history complete.

Each viewer has its own connection and cursor. The room has fixed storage for
every other participant's viewing route, alongside its gift and cache routes.
ICE agents start only when requested. The authority starts at most one new
viewing connection and sends at most eight viewing packets per 16 ms pump.
Gameplay packets are processed before auxiliary traffic. A viewing failure
closes that viewing route; it does not enter the players' input-confirmation
window.

Stopping a request releases the receiving buffer and its connection. A complete
history closes its connection and retains the buffer for playback. The next
match retires any remaining source routes before replacing authority history.

## Checks

The native room-watch fixture exercises fragmented descriptions, dropped
metadata, a dropped input packet followed by a gap, duplicate packets, live
extension, sudden death, the final digest, cancellation and active-role refusal.
It compares the entire received history byte for byte with the source.

`tools/review_native_watch_transport.py` runs three hidden, muted native clients
with copied profiles and a private room on an isolated local service. It checks
practice isolation, an explicit request, cancellation/rejoin, active-peer
checksum comparisons and final delivery while both players remain in Results.
It uses `melee_native_control room-watch start|stop` for this transport check.

The 2026-10-18 run passes. Before the request, the spectator stays in practice
with no viewing history or gameplay-netplay session. Its first request receives
187 records; after cancellation and rejoin it catches up past 257 records.
Both active peers compare checksums and reconstruct corrected inputs. The
authority makes 483 restores and simulates 3,382 replay ticks during this run.

The complete histories contain 493 records and have the same rolling digest,
`3decaf0a6a0c6455`, and initial seed. Both players remain in Results while the
spectator receives the final marker. Their endpoint frame is 492, fighter digest
`2d6bb7e1819e1e8f`, RNG `9f16834c`. The receiver releases its route after
completion and its buffer after Stop. Its local practice stays active throughout
this transport-only check.

Receipt: `.local/native-watch-transport-runtime-1/receipt.json`.
Binary/source hashes and endpoint audit: the adjacent `audit.json`.
Protocol/input/gift checks: `.local/native-watch-transport-build-4.log`.
The spectator's practice screenshot was inspected. No public service or other
players were contacted; the local clients and signaling process were stopped.

The full spectator gate still requires game reconstruction and live playback,
cached asset activation, viewer departure during gameplay, and measurements
of auxiliary traffic under load. Separate-PC and relay-route acceptance also
remain open.
