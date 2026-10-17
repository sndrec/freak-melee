# Received fighter authoring — 2026-10-17

The native Workshop can open an installed fighter as an independent editable
draft, including a fighter received from another player. This pass fixes the
first edit of a received fighter and checks the complete native delivery flow.

## Fix

`nativeHubCatalog_MakeEditable` successfully copied the package into a project
and recorded its installed ID. The Workshop then opened its draft library
without selecting that project. `new_path` combined a backslash root with a
forward slash before the new directory name. Local draft enumeration returned
backslash paths, and the Workshop selected the requested draft by exact path.

The generated path now uses the same separator as the draft enumerator. This
fixes the producer of the mismatched path and keeps the existing selection
contract. Package contents and fighter behavior are unchanged.

## Local acceptance

Both native executables built successfully. The prepared
`2026-10-17-recipient-edit` release passed publisher dry-run validation and was
extracted into `.local/native-recipient-release-extracted`.

`review_native_css_room.py` used that extracted game with a development control
tool outside the installation, a minimal Windows/System32 PATH and an isolated
local signaling service. Two hidden, muted clients used separate profiles and
the private test lobby `UB3UQUSK`. Evidence is in
`.local/native-release-sharing-3`.

The pass verified early remote package caching and portraits without installing
or adding a roster tile; local-first CSS cards with canonical match identities;
provisional choices and free cookie placement; dragging a fighter onto a player;
cancelling an offer; accepting an uncached gift while practicing as a spectator;
roster insertion, role reconnection and duplicate-install prevention.

The recipient began with installed packages and no authoring drafts. After
delivery, the sender was stopped and its test project directory was renamed.
The recipient left the lobby, opened both installed fighters in the Workshop,
reached their frozen previews and validated their scripts and assets using the
bundled helper. Two new drafts were created, one matching the delivered ID.
Both were single-fighter Mario packages. This does not establish paired-family
or edited-resource coverage, which remains in the broader integration checklist.

The first run used an overly strict test assertion: startup creates the empty
projects directory. The check now verifies that it contains no drafts. The
second run exposed the actual path mismatch; the third passed after the fix.
No netplay failure or checksum-mismatch marker appeared. PNGs were reviewed;
the deliberately small client windows crowd the Workshop header, so these
captures establish the edit flow rather than final layout acceptance.

`review_native_hub.py --script-check-build` also passed against the fixed
extracted release in `.local/native-hub-canonical-path-1`. It covers empty first
launch, stock cloning, rename replacement, copying, frozen preview, CRLF source,
script check/build/install, profile saving and controller return. All clients
and the test-owned local service were stopped. No service was deployed.
