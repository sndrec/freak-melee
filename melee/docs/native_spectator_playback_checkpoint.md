# Watching a lobby match

A room member with no active fighters keeps practicing when the match starts.
The practice toolbar offers **Watch match**. Choosing it receives the confirmed
input history and the match's fighter packages while practice continues. Once
the required packages are ready, the client leaves practice and loads the match.

The match starts from the authority's recorded random seed and uses all four
recorded controller inputs. Catch-up runs simulation and the renderer's state
callbacks for each tick. It submits a visible frame every 100 ms while behind,
omitting GPU output and model primitives between those frames. Catch-up audio
is muted. Near the live head, every available tick is displayed normally.

Each tick's gameplay digest is compared with the authority's recorded digest.
A mismatch stops viewing and reports the failure. Sudden death keeps the random
state and continues into the next recorded phase. Ordinary completion opens
Results. B or Escape leaves viewing; the player's own character selection is
restored on return to CSS.

## Scene and content ownership

The practice scene is destroyed before the watched fighter assets are adopted.
A short preparation scene stages those assets and applies the recorded rules.
The regular versus scene then creates the match. Watching borrows the receiving
history until the scene has finished, so lobby topology changes cannot replace
that history during playback.

Downloaded fighters and outfits remain cached. Viewing creates no installed
marker or CSS tile. Only explicit acceptance adds received content to the roster.
The watcher creates no rollback snapshots; confirmed input records advance a
separate playback cursor outside the simulation arenas.

## Checks

`native_match_playback_test` covers growing input histories, exact controller
values, sudden-death phases, final completion and rejection of state/result
divergence.

`tools/review_native_watch_playback.py` uses three hidden, muted clients with
copied profiles and an isolated signaling service. It exercises the actual
Watch match button, catch-up, local cancellation, re-entry, Results and return
to the local roster.

The 2026-10-18 paired-family run passes with source-only Zelda/Sheik and
Popo/Nana packages and their coordinated outfits. The spectator first verifies
262 frames, cancels with B, returns through the hub and chooses Watch match
again. The second playback verifies 1,186 frames and consumes the final result
marker. Both active peers end at frame 1,186 with fighter digest
`26dd026555b88d57` and RNG `9d790b41`. The viewer's final combined digest is
`cb1128f420d6d762`, matching that endpoint. It uses 870 CPU-only render passes
while catching up. Gameplay and Results screenshots were inspected.

That run exposed a delayed return to live input after accelerated playback.
The driver sent START once; two later diagnostic START presses advanced the
Results pages and returned home. Gameplay, cancellation and repeated entry
ran through the driver without supplemental input. The driver now repeats
START until CSS is reached, as the existing room tests do.

Receipt: `.local/native-watch-playback-runtime-5/receipt.json`.
Eight focused tests pass in `.local/native-watch-clock-build-1.log`.

The first playback attempt exposed retained character IDs in empty CSS slots.
Match preparation now resets all player slots before applying the agreed
description. This gives viewers and active peers the same initial slot data.

Cold-cache runs reproduced complete matches of 6,100 and 6,197 ticks, including
their final digests, but exposed the input delay more clearly. Advancing the
input schedule during catch-up put its deadline about 100 seconds ahead of real
time. Results waited for that deadline and stopped pumping its viewing connection
in the meantime. Every scene now rebases the host sampling schedule to the
current time. Simulation phase and game speed are preserved. The input timing
test covers 6,000 accelerated ticks followed by an immediate live scene sample,
plus rebasing during half-speed playback.

The viewing transport also accepts an already complete, verified history after
its data route closes. The transport fixture covers that ordering explicitly.

The final cold-cache run passes through Results and back to the viewer's own
roster without supplemental input. It downloads all four required packages
(22,098,270 bytes), checks their hashes and verifies 5,982 gameplay frames,
including 5,726 hidden render passes during catch-up. Both active peers finish
at frame 5,982 with fighter digest `523c5946b4c193b2` and RNG `fd4e4881`.
The viewer's combined digest is `4c66e56eff7177a9`, matching that endpoint.
All 5,983 history records, including the final marker, arrive with the same
rolling digest as the authority. Gameplay and Results screenshots were inspected.

Each active sender's package file-read counter stays at two. All four packages
remain cache-only and absent from the viewer's installed markers and CSS.
Joining, entering practice and receiving the dependencies takes 89.297 seconds;
this is functional evidence, with transfer throughput still an open target.
Receipt and independent endpoint/package audit:
`.local/native-watch-playback-cold-3/receipt.json` and adjacent `audit.json`.

The sudden-death run also passes with two source-only Fox clones. A one-minute
stock tie produces 3,838 initial-phase ticks, followed by 1,489 sudden-death
ticks. The viewer verifies all 5,327 ticks and both phase markers, then reaches
Results and returns to its own CSS. Both source peers agree at each endpoint;
the viewer's final combined digest is `1746b7b9f68866ad`. The sudden-death
screenshot shows both fighters at 300%. Receipt:
`.local/native-watch-sudden-death-1/receipt.json`.

`review_native_watch_playback.py --sudden-death` exercises that transition.
Its `--inactive-owner` case reproduces a separate start-flow gap: Start match
is enabled, but active players never enter stage selection because roster
collection skips the inactive owner's start request. The stage-selection and
proposal paths also assume the owner is an active player. The choice of who
selects the stage in this case is awaiting user feedback.

Match start by an inactive room owner and concurrent content/chat performance
acceptance remain open.

The subsequent [bounded-upload check](native_spectator_content_checkpoint.md#bounded-upload-throughput)
reduces the same cold dependency setup from 89.297 to 28.781 seconds, with
verified playback, Results and local-roster return on the current build.

The [concurrent-load check](native_auxiliary_load_checkpoint.md) also passes with
two viewers and private chat during four-player shared-PC matches. The remaining
start-flow issue here is the inactive room owner, whose stage-picker choice is
awaiting user feedback.
