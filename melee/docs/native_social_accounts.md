# Friends and letters

Updated: 2026-10-19. Friends, letters and verified player-list entries use the
account-key service. Lobby invitations arrive in the recipient's Inbox.
The social service and approved account-key replacement are deployed.
Two updated hidden/muted native clients pass the hosted invitation and
verified-presence review: `.local/account-key-hosted-1/receipt.json`. See the [account guide](native_account_auth.md).

## Using the hub

Sign in through **Settings > Online account**. In **Friends**, enter an account
name and choose **Find account**. The profile shows that player's display name
and bio. You can send a friend request, accept an incoming request, remove a
friend, write a letter, or block/unblock the player. Display names can differ
from account names.

Signed-in players appear as **@account_name** in the global and lobby player
lists. Choose one to open their account profile. Guests appear by their display
name and have a guest profile. Signing out leaves your current lobby and
reconnects global chat as a guest. If you sign in after joining a lobby as a
guest, leave and rejoin that lobby to use your account there.

To invite a friend, open their profile and choose **Invite to lobby**. If you
have no lobby, choose **Create & invite** instead. Enter the lobby name, listing
visibility and local player count, then confirm. The invitation is sent after
the new lobby confirms your membership and its name. B returns to the friend's
profile without creating a lobby or sending an invitation.

The invitation remains in their Inbox while they are offline. Open it and choose **Join lobby** to leave your current lobby and join
the sender. Invitations last fifteen minutes. Joining checks that the sender
is still in that lobby and the friendship is active. Invitations also support
an idle doubles party with space for a teammate.

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

Chat and lobby connections authenticate with the same session. The service
sets each connection's account ID, account name and display name, and checks
that the session is still active before handling each command. Revocation
closes the connection on its next command. Public presence and chat history
contain the account's public identity. Credentials stay in the native HTTP
transport and are sent only to the configured account service's HTTPS origin
(or localhost during development). Redirects are disabled for these requests.

Each invitation records its lobby and expiry in the mailbox. The service checks
the sender's verified lobby membership and friendship before creating it, then
checks both again when the recipient chooses Join lobby. Retrying an invitation
uses the same delivery record. The native client switches lobbies only after
that check succeeds.

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

The following receipts predate the account-key replacement. Current key-file
verification is recorded in the [account guide](native_account_auth.md).

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

The account-connection extension passes 79 service tests in ten files. Four
additional cases cover verified presence and history, forged identity and name
rejection, hibernation, session revocation, peer ownership, offline invitations,
retries, expiry, blocking, friendship and live lobby membership. Type checking
passes. Four native contracts cover account storage and credential-origin
checks, chat/signaling identity parsing, and room chat.

Evidence: `.local/identity-all-tests-final.log`,
`.local/identity-typecheck-final.log`, `.local/identity-native-contracts.log`.
The full native social review passes again, followed by four invitation UI
checks and five new PNGs. Two hidden, muted clients open verified global/lobby
profiles, send an invitation, receive it in Inbox, switch from another lobby
through Join lobby, and sign out back to guest chat.

Evidence: `.local/native-identity-review-1/receipt.json` and
`.local/native-identity-review-1/invitations/receipt.json`. Reviewed client hash:
`89c00669cd39f0ae0afaddb578d1f4df8c2c29787c8724297c53195aa2a3497d`.
These changes preserve the stock artwork and asset generators.

Windows security-prompt acceptance was replaced by the approved account-key
flow. The social implementation and account-key replacement are deployed; current
account-key game-screen and hosted-path checks pass. Standard file-picker
interaction and user presentation/input review remain open.
Signing in after joining a lobby as a guest currently requires leaving and
rejoining that lobby to attach the account. Physical multi-PC acceptance
remains with the user.


## Create and invite check, 2026-10-19

The native friend profile now opens the existing lobby form when the sender
has no lobby. The form identifies the recipient and uses Create & invite for
confirmation. Pending invitations clear on cancellation, sign-out, account
change, explicit leave/join, Quick Play entry or creation failure. Sending
waits for authenticated owner membership and the service's listing response;
the existing request nonce keeps retries associated with the same invitation.

The full local native social review passes four integration checks, including
five social UI checks and five invitation UI checks. The invitation review
covers backing out without delivery, creating a named lobby and automatically
sending exactly one invitation, Inbox receipt, joining from another lobby,
verified profiles and sign-out. Eight invitation PNGs include the new form.
Evidence: `.local/create-invite-social-1/receipt.json` and
`.local/create-invite-social-1/invitations/receipt.json`. The form screenshot
was visually reviewed. This is two clients on one PC using a local service;
physical multi-PC acceptance and network-failure injection remain separate.
The earlier account-key hosted receipt still describes that earlier build.
