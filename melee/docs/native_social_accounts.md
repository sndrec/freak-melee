# Friends and letters

Updated: 2026-10-19. Persistent friends and letters use the passkey account
service. The service has not been deployed. Chat and lobby connections still
use their existing temporary identities; linking those to accounts and sending
verified lobby invitations remain open.

## Using the hub

Sign in through **Settings > Online account**. In **Friends**, enter an account
name and choose **Find account**. The profile shows that player's display name
and bio. You can send a friend request, accept an incoming request, remove a
friend, write a letter, or block/unblock the player. Display names can differ
from account names.

**Inbox** holds letters and friend requests. Opening a letter marks it read.
Friend-request letters offer Accept and Decline. Reply opens a draft addressed
to the sender. **Sent** holds your outgoing letters and their delivery status.
Deleting a letter removes your copy. The recipient keeps theirs, and queued
letters continue toward delivery.

The composer accepts a subject and multiline text. Enter inserts a new line;
Ctrl+Enter sends. Up/Down moves the text cursor between wrapped lines.
Long received letters have Up/Down controls and mouse-wheel scrolling.
An unfinished draft stays in memory while you move between hub pages. It is
cleared when you choose Clear draft, sign out, change accounts or close the
program.

Letters can arrive while the recipient is offline. A sent letter is stored in
their mailbox. If their Inbox is full, the sender sees Queued and the service
retries automatically for up to seven days. A failed delivery remains in Sent.
Retrying an unconfirmed send uses the same message identity and creates one
delivery. Editing an already submitted message requires starting a new draft.

Blocking stops new letters and friend requests from that player and removes
the friend relationship. Existing correspondence stays in your mailbox. Find
the account again to unblock it.

Each folder holds up to 2,048 visible letters. Subjects allow 64 UTF-8 bytes;
letter bodies allow 2,048. The service permits 96 new letters per account per
day. Friend lists include up to 256 accepted or pending contacts. The native
pages show eight entries at a time. Friends and letters belong to the signed-in
account; local guests retain their own match seats.

Fighter sharing uses the lobby's existing package-transfer flow. Letters carry
correspondence. Global and lobby chats retain their separate conversations.

## Records and delivery

An account's permanent ID selects its mailbox Durable Object. A player's
session must authorize each mailbox operation. The service obtains the sender
from the authenticated account, and resolves the recipient's account name to
their permanent ID. Requests cannot choose another sender or another mailbox.
Account recovery preserves the ID and therefore the mailbox and friendships.

The mailbox stores Inbox, Sent, read state, blocked players and pending
deliveries in SQLite. Sending commits the outgoing letter and a delivery job
together. A delivery receipt prevents duplicate arrival after a retry or lost
acknowledgement. Alarms retry pending jobs. Deleting a letter keeps a compact
receipt for thirty days, so an outstanding retry does not restore it.

Each pair of accounts has one friendship record. Requests, acceptance,
declining and removal update that record. Revisioned updates copy the result
into both mailboxes. A persistent queue retries those updates, and each mailbox
rejects an older revision after a newer one. Friend-request letters carry the
same account identities and become handled when the request ends.

The native client shares the account I/O worker and bounded JSON buffers.
Social records stay outside match snapshots. UI diagnostics report counts and
loading state, and omit letter bodies. The new pages use the existing stock
menu scene, panels, typography and hand controls. Their implementation changes
no stock artwork or asset-generation rules.

## Verification

The service suite has 75 passing tests in nine files, including seven social
tests. They cover offline delivery, read/delete persistence, mailbox ownership,
forged-sender rejection, session revocation, duplicate sends, reciprocal friend
requests, removal, stale updates, blocking, capacity, retry after eviction,
lost acknowledgements and payload bounds. Type checking also passes.

Evidence: `.local/social-all-tests-verified.log`,
`.local/social-typecheck-final.log` and `.local/social-deploy-dry.log`.
The deployment command was a dry run. The existing eight native account-service
checks also pass with this implementation:
`.local/native-account-social-regression/receipt.json`.

The native integration review also passes. Two software-authenticator accounts
exercise the normal DPAPI session path and native HTTP worker, then two hidden,
muted Melee clients use those sessions. Five UI checks cover account profiles,
friend acceptance, multiline composition, Inbox/Sent delivery, reading,
replying, deletion and blocking/unblocking. Seven PNGs record the actual menus.
The control driver sends long text as UTF-8-safe 127-byte packets.

Evidence: `.local/native-social-review-3/receipt.json` and
`.local/native-social-review-3/ui/receipt.json`. The reviewed client hash is
`6ad0111f801b597592268c98ab05c199e21520364c84fb2389773f652a782d15`.
The native account contract still passes:
`.local/native-social-contracts-final.log`.

The full service run also exposed a Quick Play test that could consume an old
queued phase-0 packet before its unready request had been processed. The test
now waits for the authoritative unready state before checking its effects.
The 75-test result above includes that correction; gameplay code is unchanged.

Real Windows passkey ceremonies, production deployment, authenticated chat/lobby
identities and invitations remain open. Physical multi-PC acceptance remains
with the user.
