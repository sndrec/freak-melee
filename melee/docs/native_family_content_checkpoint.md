# Received fighter families and edited assets — 2026-10-17

This pass checks independent editing after source-only fighter delivery. It
extends the earlier single-Mario checks to an alternate pair, a companion pair
and another single fighter, each with edited assets and owned resources.

## Fixtures and transfer

`tools/review_native_family_content.py` creates fresh Zelda/Sheik, Popo/Nana and
Fox projects through the native clone UI. Each of the five members receives an
edited costume texture, a copied aerial animation, an owned PNG and an owned
Capsule item blueprint. Its neutral special also pulls a scripted item that
uses those resources. The resulting packages contain source, parameters, CPU
profiles, assets and resources for every member.

Fixture preparation passed in `.local/native-family-content-1`: source
preparation, package verification and installation for all three packages.
The script records the original and edited member hashes and package IDs.

Two hidden, muted clients with isolated profiles used a local signaling
service. `.local/native-family-content-3` records successful transfer of all
three packages. Selecting a fighter cached its exact package and refreshed its
remote portrait while leaving the recipient's installed collection and CSS
layout unchanged. Explicit gift acceptance installed it and added a roster
tile. Accepting the same gift again preserved the existing roster.

The sender then stopped. Both source project roots were renamed within the
isolated test directories. The recipient's later authoring checks therefore
use received package contents and its own new project directories.

## Fixes found during recipient editing

Switching to a dormant alternate correctly loaded its preview actor, but did
not arm the Workshop's loading-completion status. Member changes now use the
existing completion check, which announces when the frozen preview is ready.
Live-test entry, return by button or Escape, and explicit preview refresh now
also complete their loading messages through that check.

Restoring a member's controls also called `SetValue` on unchanged dropdowns.
RmlUi dispatches a change event for that call. Restoring a boolean parameter
therefore rewrote the parameter file, including the decimal formatting of
unrelated floats. This changed the package ID without an authored edit. The
restore path now assigns only values that differ from the rebuilt controls.

The family picker retained options from the previous draft in RmlUi's internal
dropdown. After Zelda/Sheik, the Ice Climbers draft therefore displayed Sheik
as its second member. Opening a family now clears those options before adding
the current member names. The batch checks the complete option list for each
draft, including the hidden single-member picker.

The native control tool now supports named navigation keys in the Workshop.
The hub inspector can enumerate all current hit regions, including repeated
template/catalog regions and their values. The batch uses those actual hit
regions rather than guessing button positions.

## Final combined run

The game and content helper built successfully in
`.local/native-family-content-build-8.log`; the control tool and resource copy
completed in `.local/native-family-content-build-9.log`.

`.local/native-family-content-10/receipt.json` reports `PASS full integration`
for the final build. It records executable/script hashes, package/member
hashes, CLI invocations, delivery assertions and screenshots. This run combines
the three package transfers with the recipient checks in the same two-client
session. The earlier prepared fixtures remain in
`.local/native-family-content-1`.

After the sender stopped and its original project paths became unavailable,
the recipient created its own drafts. All eight payload chunks for every
member matched the received package byte for byte. The native editor and the
content helper validated all five members. Helper invocations used a minimal
Windows/System32 PATH. Rebuilding each untouched draft produced its original
package ID.

Test Character launched each of the three families. Finite neutral-special
inputs produced the owned checkerboard item billboard on Zelda, Popo and Fox;
the Ice Climbers test also showed Nana active. All three returned to the frozen
editor successfully. These are short live exercises, with no whole-moveset or
item-combat acceptance claim.

The header check requested 960-by-600, 1280-by-800, 1440-by-900 and
1600-by-1000 windows. It checked control bounds and pairwise overlap and clicked
all four workspace tabs at each size. It also checked that each family picker
contained exactly its own member names. Narrow/wide header, recipient member
and live-test PNGs were reviewed. All test clients and the local service were
stopped; the real installation's collection and projects were unchanged.

## Evidence boundaries

The initial transfer retry observed the installation marker before the next UI
tick inserted the roster tile. The test now waits for both publications. This
was a test timing issue. Later recipient runs exposed the loading-status and
unchanged-dropdown problems described above.
Run 8 passed its automated checks, but screenshot review exposed the stale
member names; the final batch also checks those names. Run 9 completed transfers
before its harness collided with an already-retired project directory from a
reused fixture. Retirement paths now include the unique run name; run 10 passed
the complete sequence.

Earlier runs exposed overlapping Workshop header controls at narrow widths.
The user approved a second header row for narrow windows. Below 1440 density-
independent pixels, the mode tabs now occupy that row; the context and editing
area move down with it. Wider windows retain the existing header arrangement.

The edited costume is embedded in each fighter package. Separate costume
packages and their parent dependencies still need current integration checks.
This local pass also leaves physical multi-PC connections, relay behavior and
hosted account/inbox delivery to their own acceptance work.

Companion frozen-preview presentation remains open: the scripting preview
capture still shows both Popo and Nana. These content checks do not close the
separate selected-member-only presentation task or the user's usability review.
