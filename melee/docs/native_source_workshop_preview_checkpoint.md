# Source-owned Workshop member and pose preview

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The Workshop no longer uses `nativeFighterRuntime_Definition` as a runtime
package discriminator. Ordinary interpreted packages intentionally have no
C-backed definition. Those obsolete checks rejected member one, blocked
fighter-specific motion lookup and left the retained pose's definition check
comparing NULL with NULL.

`native_home.c` now resolves the selected immutable package member through the
existing family lookup and live Fighter registry. It captures the caller and
selected actor with `NativeFighterLifetime`. Stock fallback is restricted to a
real stock member-zero preview; an unavailable interpreted package does not
silently become a stock preview. Simultaneous members select an already-live
actor. Initial member ordering and explicit dormant recreation remain restricted
to genuine registered alternate pairs with distinct source owners and matching
published program banks and identities.

Action lookup validates the owned common/special motion pointers and imported
common count/first-state contract before scanning the source's special-motion
count. The paused pose cache retains the actual source package, program/storage
lifetime, actor generation and existing costume/rig/tree identity. Motion entry
rechecks that lifetime before accepting the pose. Demo restoration, hitbox
cleanup and process resume only modify the retained current owner.

## Verification

The exact live object compiler flags pass a syntax check with no diagnostics.
The full native client, content tool and selected source integration executables
build successfully. Source SHA-256 is
`d3a1243b8c2a812702693fdaaf110d6641c3e648597aa3730b5490f1647caaff`.
Syntax receipt:
`A:/FreakMelee-test-archives/workshop-source-identity-syntax-oct10/receipt.json`
(SHA-256 `0c1ff12ce2b52dc307516f8ad93bd41e38bf3b4bf541b59a13e574e5c2963030`).

A private LOCALAPPDATA directory held a copied full Popo/Nana draft. The actual
hidden, muted, rendered client was driven through its native control channel,
without mouse/keyboard input on the user's desktop. The checks were:

- Open the draft through Collection and Workshop; freeze Popo down-air at zero.
- Select Nana and scrub down-air to frame 11, with active preview hitboxes.
- Select Nana's SpecialN and scrub to frame 11, exercising owned special-motion lookup.
- Return to Popo's down-air at zero, then return to drafts and discard only the
  temporary private session edits.

Screenshots and logs are retained with
`A:/FreakMelee-test-archives/workshop-source-companion-preview-oct10/receipt.json`
(SHA-256 `89f6d16556608ba4d15fda4cb1336f0f8853a5ea4609fb3b63d235e725039cb6`).
The task-owned process was stopped forcibly after testing. This is an actual
UI/pose smoke, not a deterministic state comparison, injected callback-failure
proof or normal application-exit test.

The updated Action/Demo fixture passes 2,106 checks using the extracted actual
Pose/Release/Invalidate/tree-validation and figatree loader bodies, the actual
`NativeFighterLifetime` header and the real fixed-pool implementation. Its two
fixed GObj slots cover same-address pool reuse. Changed source, program, bank,
storage, layout, parameters, userdata and scene lifetime during motion entry,
Demo evaluation and release stop the native continuation without modifying or
resuming a replacement. Existing scrubbing, malformed tree and restoration cases
remain. Root independently rehashed all 1,905 recorded inputs unchanged.

Receipt: `A:/FreakMelee-test-archives/workshop-animation-preview-source-oct10-dev/receipt.json`
(SHA-256 `09a13b82898c8ba8619639db4b8546fbbeeb155a85155221236ae5e79d0420b4`).
The output name contains `dev`, but this is the frozen successful receipt.
Compilation emitted the existing SDK `MTXPerspective` array-bound warning.

## Boundaries

That focused fixture controls registry/package/program publication, actor view
and status, callback generation, assets/rig validation, motion entry, HSD
evaluation and camera leaves. It executes no interpreter behavior or family
mapping. The actual companion UI smoke exercises member selection separately.
Alternate dormant recreation and adversarial replacement during real game
callbacks require their own connected checks.

Both companion meshes remain visible in frozen preview. Showing only the
selected member is separate presentation work. Existing font-face warnings also
remain; this patch changes no Workshop layout or fonts. Full linked-special
behavior and compact whole-match rollback state remain outside this checkpoint.
