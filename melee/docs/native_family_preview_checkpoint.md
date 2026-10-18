# Frozen family previews — 2026-10-17

Frozen editing now shows the chosen family member. Switching between Popo and
Nana displays and poses that member alone; both actors stay paused. Test
Character recreates normal practice with the complete family and Sandbag.
Zelda/Sheik retain their normal active/dormant form handling during live tests.

## Implementation

The two fighter render callbacks consult the frozen-preview visibility policy
before drawing models, collision overlays or accessories. This leaves authored
mesh visibility and ordinary inactive/invisible handling intact.

The preview already held the selected actor's processes. It now also holds its
active companion. The fixed process-reference array records only pauses added
by the Workshop. Release resolves those exact pool generations and checks the
retained fighter lifetime before resuming them. Earlier pauses are preserved;
recycled process slots and invalidated owners are left untouched. No runtime
allocation or fighter-specific rule was added.

The developer inspector `ui-inspect workshop-preview` reports the selected
member, live/frozen state, actor identities, draw permission, process counts,
animation frames and positions. The batch uses it to check the actual actors.

## Verification

- `.local/native-family-preview-build-1.log`: native game, content helper and
  control tool built successfully.
- `A:/FreakMelee-test-archives/workshop-family-holds-oct17-3/receipt.json`:
  **2,704 checks passed** using extracted production pose/release/hold/visibility
  bodies, the production lifetime guard and actual fixed-pool implementation.
  This includes prior pauses, member changes, process-slot reuse and an
  invalidated companion owner. Family mapping and rendering are controlled
  boundaries in this focused fixture.
- `.local/native-family-preview-2/receipt.json`: the hidden, muted recipient
  batch passed with source-only Zelda/Sheik, Popo/Nana and Fox projects. Each
  selected member was the sole actor permitted to draw. All active actors had
  all fifteen processes paused, and repeated samples 0.7 seconds apart had
  identical frames and positions. The live Ice Climbers test had both actors
  active with all processes resumed. Returning to editing restored the chosen
  member and both process holds.
- All five members still passed source validation, owned payload comparison
  and unchanged package-ID rebuilds. Header navigation at four window sizes
  also passed. Popo-only, Nana-only and full-family live PNGs were reviewed.
  No fighter VM failure diagnostic was present. The test client was stopped;
  no signaling service was needed for this authoring pass.

## Fixture correction and remaining review

The earlier content batch pressed B repeatedly after adding an unconditional
item pull to each fixture's neutral special. The first pull worked; later
pulls correctly failed because the hand was occupied. Its live smoke checked
process survival and rendering, without checking VM health. The extended batch
uses one pull and explicitly checks fighter VM diagnostics and actor state.
The first extended run exposed that fixture error; the corrected run passed.

These checks cover frozen/live family presentation and short source-owned item
exercises. Full move behavior, item combat, standalone costume dependencies
and the user's broader Workshop usability review remain separate acceptance.
