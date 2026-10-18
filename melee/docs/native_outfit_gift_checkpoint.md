# Outfit delivery with a missing parent

Checked on 2026-10-17. Accepting a custom outfit delivery now fetches its exact
parent fighter when needed. It uses the existing dedicated content connection
for both packages, in parent-then-outfit order. Cached packages are reused.
Both remain outside the installed collection until the complete outfit passes
source and model validation.

The offer identifies the parent. The recipient checks that identity against
the outfit's own metadata. Only the offered outfit and parent may be requested.
A delayed parent request cannot replace an outfit transfer already in progress.
Incoming data and acknowledgements are matched to the current artifact as well
as the gift's peer and serial.

After validation, explicit acceptance installs both packages. A newly installed
parent is added to the CSS roster. Accepting an outfit for an already installed
parent preserves that parent's existing roster placement. Repeated acceptance
preserves one installation and one roster tile.

The sender keeps the delivery open until the recipient confirms installation.
An acknowledged byte transfer alone does not report success. Repeated offers
recover a lost installation receipt. Closing the byte connection before the
receipt arrives is allowed; waiting for that receipt is bounded to two minutes.
Failure and room teardown release both retained package buffers.

## Evidence

`native_room_gift` passes twelve cases using the actual gift, content-transfer
and content-store code. Transport queues and engine validation outcomes are
controlled. It covers an empty recipient, either package cached, both cached,
an already installed parent, dropped final data acknowledgements, a dropped
installation receipt, source/model rejection, corrupt outfit bytes, decline,
clearing an active receive, a false advertised dependency, receipt timeout,
unrelated requests and delayed parent requests. Ordinary cached and uncached
fighter gifts also pass. This test establishes protocol/storage behavior; its
controlled validation callback does not establish engine model correctness.
Logs: `.local/native-gift-dependency-test-6.log`.

`tools/review_native_outfit_gifts.py` exercises real hidden, muted clients with
an isolated local signaling service. The sender has the previously checked
Zelda/Sheik, Popo/Nana and Fox outfits. The recipient begins without either
package for each family and remains in baseline practice while receiving them.
The test uses the Collection recipient picker and Send buttons, then the native
Accept outfit prompt. It verifies both package hashes and installed markers,
the parent's single CSS tile, the sender's installation receipt and unchanged
markers/roster after a repeated delivery. Exactly six installed identities are
added across the three families. The recipient has no authoring drafts.
Receipt and executable/script hashes:
`.local/native-outfit-gifts-2/receipt.json`.

The first live attempt failed before offering a package because the test
driver's End key left the recipient picker on its placeholder. The corrected
driver selects and verifies the recipient with Down/Enter. This was a test
input failure, not evidence of a network or package failure.

These checks cover local direct transfers and actual source/model validation
during acceptance. A fresh online match using these received outfits, relay
transport and other-PC behavior remain open. The broader goal remains active.

The subsequent [2026-10-18 room-match checkpoint](native_outfit_match_checkpoint.md)
adds current online match, rollback and Results evidence for received outfits.
