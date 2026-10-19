# Online accounts and account-key files

Updated: 2026-10-19. The user approved generated account keys after rejecting
Windows security prompts. Account creation, sign-in, key files and recovery
are implemented in the native client and deployed service. Native screen and
hosted-path checks pass; the standard file picker remains for user acceptance.

## Create an account

Open **Settings > Online account > Create account**. Choose an account name and
display name. Freak Melee generates an account key and saves it on this PC.
Future launches sign in automatically. Account names contain 3-24 letters,
numbers or underscores and are case-insensitive. You can edit your display
name and bio later.

Save the generated recovery code when prompted. It lets you recover your
account if you lose your key. The reminder remains until you save the code.

One account signs in to the client. Local guests still have separate match
seats. Friends, letters and lobby invitations belong to the signed-in account.
Signing out leaves your current lobby and reconnects global chat as a guest.

## Use an account on another PC

**Export account-key file...** saves a portable `.fmakey` file. Choose
**Sign in > Import account-key file...** on the other PC. Keep the file private:
anyone who has it can sign in to your account.

**New key for another PC** creates a separate key file for the same account.
Import that file on the other PC. Separate keys let you revoke one device's
key without replacing the key on your main PC. The file is saved before its
public key is registered, so a lost response cannot lose the new private key.
If registration fails, the file may need to be regenerated from this menu.

**Manage keys** lists registered keys and marks the current key. Removing a
key also signs out every session using it. Keep at least one registered key.
**Signed-in devices** lets you end an individual session. **Sign out all
devices** ends every session; the keys remain usable for a new sign-in.
Signing out on this PC retains its saved key and stays signed out after restart
until you choose **Sign in**.

## Recover an account

Choose **Recover**, enter the account name and saved recovery code. Freak Melee
generates a replacement key, revokes the old keys and sessions, and issues a new
recovery code. Your account ID, profile, friends and letters stay attached to
the same account. Save the replacement code; the old code expires after recovery.

**New recovery code** replaces the existing code while signed in. Generated
codes stay protected on this PC until exported. Account views and diagnostics
contain public account information, never private keys, tokens or recovery codes.

## Authentication and storage

The client generates a random 32-byte seed with the operating system's random
number generator. Monocypher 4.0.2's optional standard Ed25519 module derives
its key pair and signs one-use service challenges. The service stores the
public key and verifies signatures using the runtime's Node crypto module.
User-chosen passwords and OS authenticator enrollment are absent from this flow.

The service stores permanent random account IDs, handles, display names, bios,
public keys and labels, session expiry/verification times, and hashes of session
tokens and recovery codes. A database copy alone does not contain account
private keys. A stolen key file grants account access; a compromised client or
service remains a security risk.

Each account has a SQLite-backed Durable Object. Up to eight keys and sixteen
sessions are supported. Sessions expire after thirty days. Challenges last two
minutes, work once and are bound to the service origin, account handle, permanent
ID, purpose, proposed public key, random attempt ID and random challenge.
Pending attempts are capped at sixteen. Account requests have a per-network
rate limit, bounded 64 KiB JSON bodies and `Cache-Control: no-store` responses.
Transactions and account epochs protect creation, recovery and concurrent
changes. Sensitive operations require recent key verification, performed
automatically by the client using its local key.

The Windows client stores its seed, session token and pending recovery code in
`content/account-session.bin`, protected for the Windows user with DPAPI.
This protection is transparent and opens no Windows security prompt. The
portable file contains the application marker, service origin, account name
and seed as JSON; it is independent of Windows secret storage. A future Linux
client can use the same authentication protocol and exported file with its own
local secret storage.

The client validates the configured service origin, keeps normal HTTPS
certificate checks and rejects redirects. Local development permits HTTP only
on `localhost`. Account HTTP, signatures and file I/O run on a dedicated worker
outside the game tick and rollback state. JSON pools and secret temporaries are
cleared after requests. Registration and recovery save the new key locally
before sending the final proof, so an interrupted response can be followed by
sign-in with that key.

## Service protocol

Paths begin with `/v1/accounts/<account-name>/`. Protected requests use
`Authorization: Bearer <session-token>`. POST requests use JSON.

| Path | Method | Purpose |
| --- | --- | --- |
| `register/options`, `register/finish` | POST | Create an account with its first key. |
| `login/options`, `login/finish` | POST | Sign in using a registered key. |
| `reauth/options`, `reauth/finish` | POST | Verify a key before security changes. |
| `add/options`, `add/finish` | POST | Register an additional key. |
| `recover/options`, `recover/finish` | POST | Replace lost keys using a recovery code. |
| `cancel` | POST | Discard a pending attempt. |
| `profile` | GET / POST | Read a public profile / edit your profile. |
| `me` | GET | Read your profile, keys and sessions. |
| `logout` | POST | End the current session. |
| `sessions/revoke`, `sessions/revoke-all` | POST | End a selected session / all sessions. |
| `keys/remove` | POST | Remove a key and its sessions. |
| `recovery/rotate` | POST | Generate a replacement recovery code. |

Options requests supply `public_key` (32 bytes, canonical Base64url), with
`name` for registration, `key_name` for new keys, or `recovery_code` for recovery.
Responses contain `attempt`, `challenge`, `user_id`, `public_key`, `purpose`
and `origin`. Finish requests supply `attempt` and `signature` (64 bytes,
canonical Base64url). Login, registration and recovery return a token and
expiry; registration and recovery also return a recovery code.

Signed UTF-8 bytes are the following eight newline-separated fields, with no
trailing newline: `Freak Melee account key`, configured origin, canonical
account handle, permanent user ID, purpose, public key, attempt, challenge.
The origin comes from trusted configuration. The client checks that the origin,
purpose and public key match the request before signing.

## Verification

The service suite passes 68 tests in ten files. It exercises real Ed25519 proofs,
invalid signatures and field binding, replay/expiry, concurrent registration,
recovery, key/session limits, revocation, social delivery and account presence.
Evidence: `.local/account-key-typecheck.log`, `.local/account-key-tests.log`
and `.local/account-key-deploy-dry.log`.

The native contract checks RFC 8032's first test vector, portable key-file
round-trip and service binding, strict Base64url, protected-storage tamper
rejection and origin-limited token transport. Six integration checks exercise
native key generation, independent Node/native signatures, import/export,
automatic restart, intentional sign-out, device-key revocation and recovery
against the local service: `.local/account-key-integration-final/receipt.json`.
These checks use the production client paths without a verifier bypass.

The service is deployed at the existing private endpoint as version
`c59460c2-3719-49ab-9093-2db550ed5bf4` (`.local/account-key-deploy.log`).
Six native account-screen checks and six PNGs cover Settings navigation,
sign-in/import controls, duplicate registration, masked recovery entry and
successful in-game key generation: `.local/account-key-ui-1/receipt.json`.
Two hidden, muted clients pass hosted registration/native import and the
verified-profile/invitation/join/sign-out flow:
`.local/account-key-hosted-1/receipt.json` and its `ui/receipt.json`.
Seven hosted UI PNGs include the key-file overview and current-key listing.
The reviewed executable SHA-256 is
`fffdeba53009dd75b633cb081bcc31058ed79973885ab4bb77ab84a5e8116608`.
Four native contracts and seven release-payload checks also pass. Task-owned
clients were closed, hosted test sessions revoked and test key files removed.

The user confirmed in-game account creation on their PC on 2026-10-19.
The file worker paths are tested; interaction with the normal OS file picker
remains a user acceptance check. Multi-PC testing also remains with the user.
Earlier Windows WebAuthn receipts describe the replaced architecture.
The [friends and letters guide](native_social_accounts.md) covers social behavior.

References: [Monocypher Ed25519](https://monocypher.org/manual/optional/ed25519),
[RFC 8032 test vectors](https://www.rfc-editor.org/rfc/rfc8032#section-7.1).
