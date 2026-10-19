# Accounts and passkey sign-in

Updated: 2026-10-19. The account service and native account client are
implemented. Real Windows passkey approval and recovery still need interactive
acceptance. The service has not been deployed.

## Account identity

An account has a unique name containing 3–24 letters, numbers or underscores.
Names are case-insensitive. The display name and bio can be changed separately.
The service assigns a permanent random account ID when registration succeeds.
Recovery preserves that ID, so friends and correspondence can remain attached
to the same person.

One account signs in to each client. Local guests remain separate match seats.
The existing room connection IDs and match ports retain their own purposes.
Connecting accounts to room and chat identity is part of the remaining work.

## Passkeys and recovery

Registration creates a passkey. Subsequent sign-ins prove possession of its
private key and require the authenticator's user-verification step. The
Windows client uses the native WebAuthn API and a Windows security prompt.
The account screens stay in Freak Melee; a browser window is not required.

Registration also returns a generated recovery code. The client must show it
with a way to save it. The service keeps its hash and cannot retrieve the code
later. A recently approved sign-in can generate a replacement recovery code.

Recovery uses the code to register a replacement passkey. It revokes the old
passkeys and device sessions and generates a new recovery code. Both account
identity and profile are preserved. Cancelling the registration prompt leaves
the existing recovery code usable. Finishing recovery consumes that code.

Accounts support up to eight passkeys and sixteen signed-in devices. Account
security changes require passkey approval within the last five minutes. A
player can revoke one device, sign out of every device, and remove an old
passkey after adding its replacement. Device sessions expire after thirty days.

## Stored information

The service stores the account ID, account name, display name, bio, passkey
credential IDs and public keys, signature counters, device labels, expiry times,
and hashes of session tokens and recovery codes. Private passkey material stays
with the authenticator. This flow does not collect passwords or email addresses.

Registration and sign-in challenges expire after two minutes and can be used
once. At most sixteen attempts can be pending for an account. Account requests
have a per-network rate limit. JSON bodies are bounded to 64 KiB. Public account
requests require HTTPS; loopback HTTP is available for local development.
Responses use `Cache-Control: no-store`. Credentials and tokens are excluded
from error logs. The native client protects its saved token with Windows DPAPI
for the signed-in Windows user. It sends the token in an authorization header.

## Native account screens

Open **Settings > Online account** to create an account, sign in or recover an
account. Enter your account name and approve the Windows passkey prompt.
The account page then offers profile editing, passkey management, signed-in
devices, recovery-code replacement and sign-out. Approve security changes with
your passkey before adding/removing keys or changing recovery settings.

Creating or recovering an account opens a recovery-code reminder. **Save recovery
code...** saves a private text file at a location you choose. Until that succeeds,
the code stays protected in the local session file and the account page keeps
the reminder. Signing out requires saving that pending code first.
Recovery-code entry is masked. Generated codes and session tokens are absent
from ordinary account views, screenshots and diagnostics.

The native client uses one I/O worker with fixed JSON buffers. HTTP and passkey
approval run outside the game tick. The account worker and session data stay
outside rollback state. Requests use the configured service origin, validate
the returned passkey origin/RP, retain normal TLS checks and reject redirects.
Development HTTP is restricted to `localhost`; use that same origin and RP in
the local service configuration. A cancelled prompt leaves the recovery code
usable. Once a finish request begins, the client collects and protects its
result because the server may already have committed the account change.

The session file is `account-session.bin` in the content folder. It contains
DPAPI-protected account/session data and is bound to the configured service
origin. Account state remains separate from fighter packages and local seats.
The client uses yyjson 0.12.0, pinned in CMake, for bounded JSON parsing. Its MIT
license is included in prepared releases.

## Service endpoints

Paths begin with `/v1/accounts/<account-name>/`. POST requests use JSON.
Protected requests use `Authorization: Bearer <session-token>`.

| Path | Method | Purpose |
| --- | --- | --- |
| `register/options`, `register/finish` | POST | Create an account and its first passkey. |
| `login/options`, `login/finish` | POST | Sign in on a device. |
| `reauth/options`, `reauth/finish` | POST | Approve account security changes with an existing passkey. |
| `add/options`, `add/finish` | POST | Register another passkey while signed in. |
| `recover/options`, `recover/finish` | POST | Replace lost passkeys using a recovery code. |
| `cancel` | POST | Discard the supplied pending attempt after cancelling a prompt. |
| `profile` | GET | Read the public display name and bio. |
| `profile` | POST | Update your display name and bio. |
| `me` | GET | Read your profile, passkeys and signed-in devices. |
| `logout` | POST | End the current device session. |
| `sessions/revoke` | POST | End a selected device session. |
| `sessions/revoke-all` | POST | End every device session and pending sign-in attempt. |
| `passkeys/remove` | POST | Remove a passkey while retaining at least one. |
| `recovery/rotate` | POST | Generate a replacement recovery code. |

Options return an `attempt`, WebAuthn `options` and the configured `origin`.
Finish requests include the attempt and a WebAuthn `credential`. Successful
registration, recovery and login return a session token and expiry. Registration
and recovery also return the new recovery code. Tokens and codes are secrets;
the client must not include them in diagnostics or screenshots.

The configured origin and RP ID are deployment settings, not values supplied
by a client or inferred from its HTTP Host header. The Windows client must check
that the RP matches the trusted account service before invoking WebAuthn.

## Implementation and evidence

Each account has its own SQLite-backed Durable Object. SimpleWebAuthn Server
14.0.3 verifies the WebAuthn responses. Registration requests ES256 and RS256,
user presence and user verification. Authentication verifies signatures, the
challenge, origin, RP ID, user handle and signature counter. Synchronous storage
transactions protect account creation and recovery; epoch and counter checks
also reject changes made while cryptographic verification was running.

The service tests use a software authenticator that produces actual P-256
signatures and CBOR attestation data. They exercise the production verifier,
without a test-only authentication bypass. They cover persistent identity,
hibernation, invalid proofs, replay, simultaneous registration, challenge and
session expiry, parallel attempt limits, recovery, key management and device
revocation. These tests do not establish Windows Hello integration or the
usability of the eventual account screens.

Evidence is in `.local/hub-account-typecheck-final.log`,
`.local/hub-account-all-tests-final.log`, `.local/hub-account-audit-final.json`
and `.local/hub-account-deploy-dry.log`. The dependency update also resolves the
previous development-tool advisories; the recorded audit covers production and
development dependencies. A dry run checks packaging and does not deploy it.
The final suite has 68 passing service tests. Two hidden, muted native clients
also pass the existing lobby flow against this service and updated development
runtime: `.local/native-lobby-browser-account-service-1/receipt.json`. Its
screenshots retain the stock ribbon artwork. This is a hub regression check,
not a native account sign-in check.

The native contract test checks URL policy, account-name normalization, strict
Base64 decoding, origin/RP validation and DPAPI round-trip, service binding and
tamper rejection. It confirms Windows WebAuthn API availability without opening
a prompt. `.local/native-account-contracts-verified.log` records its result.

`tests/native/account_client_service.mjs` exercises the native worker against
the real local account service with software-authenticator proofs. Eight checks
cover saved-session validation, profile changes and rejected edits, device and
passkey revocation, last-key protection, recovery-code persistence across
restart, acknowledgement and sign-out. Evidence is in
`.local/native-account-options-verified/receipt.json`. The test also validates
the service's real registration and authentication options against the native
passkey policy before supplying the saved session. It feeds
service-issued sessions through stdin and protects them using the normal store.
It does not substitute a verifier or add a production authentication bypass.

The hidden, muted native UI check passed five checks and recorded six PNGs in
`.local/native-account-ui-1/receipt.json`. It covers the Settings entry point,
duplicate account creation, unknown-account sign-in, masked recovery input and
diagnostics, invalid recovery-code entry and returning to the account page.
The screenshots show service errors in the shared stock footer and retain the
original ribbon artwork. Seven release-payload tests also pass with the
yyjson license included: `.local/native-account-release-payload.log`.

Remaining work includes real Windows passkey creation, sign-in and recovery
acceptance, authenticated social presence, friends, the offline inbox,
invitations and deployment. The account client does not complete the hub milestone.

References: [SimpleWebAuthn Server](https://simplewebauthn.dev/docs/packages/server),
[Windows WebAuthn APIs](https://learn.microsoft.com/en-us/windows/security/identity-protection/hello-for-business/webauthn-apis).
