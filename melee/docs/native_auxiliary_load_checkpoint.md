# Concurrent lobby activity — 2026-10-18

This check runs an active rollback match while two late spectators download
the required fighters and outfits, receive confirmed inputs and catch up.
A fifth room member sends ten private lobby messages during that work.

`tools/review_native_watch_load.py` launches hidden, muted native clients with
copied profiles against a loopback signaling service. Global chat is disabled.
Both spectators begin with unrelated practice content and empty caches for the
match's packages. They choose Watch match, verify playback through Results and
return to their own rosters. Downloads remain cached; they create no installed
markers or roster tiles.

## Local results

| Case | Active players / fighters | Received bytes across both viewers | Sampled interval | Active tick rates |
| --- | --- | --- | --- | --- |
| Zelda/Sheik and Ice Climbers | 2 / 3 active fighters | 44,196,540 | 24.843 s | 60.02 / 59.98 ticks/s |
| Four Ice Climbers slots, two players per PC | 4 / 8 fighters | 20,904,180 | 24.641 s | 60.22 / 59.98 ticks/s |

Both runs show overlapping downloads and no increase in input-stall counts or
input-wait time during their sampled intervals. One active peer has an earlier
startup wait: 31 ms in the first case and 15 ms in the second. The authority
reports no input waits over either match.

Each active PC retains its selected packages before gameplay. Its package
file-read counter stays at two while serving spectators. The first case serves
four requests from each active PC. In the eight-fighter case all four slots use
the same package IDs, so the authority serves the four deduplicated requests.
All downloaded packages pass their SHA-256 hashes on both viewers.

All ten chat messages appear in all four native clients. Sender acknowledgments
take at most 15 ms in the first case and 16 ms in the second. Chat inspection is
available while the hub is hidden, allowing the check to read the actual chat
state during gameplay.

Both viewers verify all 3,194 ticks in the first match and all 3,600 ticks in the
second, plus their final markers. Both active peers agree at each endpoint:

| Case | Fighter digest | RNG | Viewer combined digest |
| --- | --- | --- | --- |
| Paired families | `302a8b0f37c7b961` | `2be78c8d` | `f88036accabba004` |
| Eight fighters | `a3967b668db7073e` | `ec1f6a09` | `a120e5f7f1319475` |

The eight-fighter run checks eight live costume actors on each active client
and canonical port masks `0x3` / `0xc` for the two shared PCs. Gameplay screenshots
were inspected. Both viewers complete Results and return to their original
local fighter selection.

Receipts and independent audits:

- `.local/native-watch-concurrent-load-2/receipt.json` and adjacent `audit.json`
- `.local/native-watch-concurrent-eight-2/receipt.json` and adjacent `audit.json`
- Native build: `.local/native-watch-load-build-1.log`

The first attempt stopped before load because chat inspection required a visible
hub. The inspection route now works during gameplay. The first eight-fighter
attempt missed overlap because one viewer finished downloading while the other
was starting. Both viewer practice scenes are now prepared offline before
joining, so the measured transfers overlap.

This closes local acceptance for concurrent content, spectators and room chat,
including four players on shared PCs. Physical multi-PC tests remain with the
user. Larger spectator counts and constrained-network throughput can be tested
separately as needed.
