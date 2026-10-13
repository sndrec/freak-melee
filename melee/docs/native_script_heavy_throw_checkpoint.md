# Interpreted heavy-item input and entry

`controls.try_heavy_item_throw()` now invokes the common heavy-item selector.
It retains the live source Fighter, exact program/storage/layout and scene,
requires common input data, and publishes its Bool only after a final check.
It is a generic carrying-state operation, not a DK-specific special handler.

The native throw initializer now executes the prepared `heavy_throw_enter`
event on attached interpreter actors. The script can install its own movement
callbacks; common animation/throw bookkeeping remains in the engine. An attached
source never consults a native fighter Definition for that event. Retained
Fighter/program guards after motion initialization, animation initialization
and the event stop continuation on retirement, including same-actor program
replacement. Healthy unoverridden carrying behavior is unchanged.

Bowser, Ness and Pikachu/Pichu templates also replace seven obsolete
`item.child_visible(true)` calls with the existing generic joint interface:
test `item.joint(0, "0").exists`, then set its `local_visible` to true. This
preserves the original first-child-only flag change and missing-child no-op;
it does not add another shorthand API or show the whole model.

## Verification and limits

The connected ground-controls fixture passes **115,191 checks / 1,846 cases**.
Its new heavy-item cohort has **1,648 cases**, including **1,600 healthy
comparisons** against common selector/entry bodies pinned at `6e985dc6a`.
Those comparisons cover all eight throw directions, both facing signs,
main-stick/button and c-stick selection, input ages, held/no-item outcomes,
damage multipliers and ordinary carrying callback selection. The authored event
executes through the production Fighter actor/VM. Forty retirement/fault cases
cover native motion, animation, authored hook and initial accessory boundaries;
seven invalid entry cases preserve the result and Fighter.

Held-item multiplier lookup, animation initialization, motion delivery and
accessory placement/release are controlled engine leaves. Full heavy-item
release, DK cargo damage/pickup policy, collision and rendered gameplay remain
unverified. Source preparation advancing past an operation is not whole-fighter
acceptance.

- `A:/FreakMelee-test-archives/script-heavy-throw-oct10-final/receipt.json`
  SHA-256 `209562b895f9829747f13614221821e3313bc4250fa0bb590f11ea8816e26e6f`,
  **1,741 independently rehashed input pins**.
- Ten-source preparation regression:
  `A:/FreakMelee-test-archives/source-heavy-model-oct10-a/receipt.json`
  SHA-256 `772227bceb888946fd8bbaa76255dbb9e1ac84e0e0ae1bd13e98c1d8cba446c3`,
  **27 pins**. DK now reaches `heavy_item_damage`; Bowser reaches vector
  normalization. Six previously preparing sources continue to prepare.
- Full current roster preparation:
  `A:/FreakMelee-test-archives/source-full-roster-oct10-heavy-b/receipt.json`
  SHA-256 `6745bf8aed64139ce7d5c90c9803c72021d64fe61d2e6e034e5339be88788d11`,
  **51 pins**. See the current roster table below.
- Independent rehash record:
  `A:/FreakMelee-test-archives/heavy-model-independent-pins-oct10.json`.
- Seven configured Windows targets build; `native_fighter_script` CTest passes;
  `git diff --check` passes. No game opened or installed/user assets changed.

## Roster preparation audit at this checkpoint

The later [callback integration checkpoint](native_script_callback_integration_checkpoint.md)
records the next complete roster audit.

There are 26 forms in this audit, including Sheik and Nana separately and
excluding Kirby. Every form has a script template. **12 pass current source and
owned-asset preparation**; this is not a count of gameplay-accepted clones.
Each rejected row reports only its first diagnostic, not its full remaining
work inventory. A fresh Samus `clone-stock` attempt also failed to create an
independent draft; no valid preparation result is inferred for it.

| Forms | Current result |
| --- | --- |
| Mario, Dr. Mario, Peach, Zelda, Popo, Nana | Preparation passes. |
| Captain Falcon, Ganondorf, Luigi, Marth, Roy, Yoshi | Preparation passes. |
| Link / Young Link | Unknown `Callback.death1` member in callback cleanup. |
| Game & Watch / Sheik | Unknown `Callback.death2` member in callback cleanup. |
| Jigglypuff | Accessory tracking operation is not bound. |
| Fox / Falco | Owned entity lifetime/data schema or matching asset binding. |
| DK | `heavy_item_damage` event scheduler is not connected. |
| Bowser | Vector normalization operation is not bound. |
| Pikachu / Pichu | Matching owned entity asset binding. |
| Samus | Independent draft import still fails. |
| Mewtwo | `reflector.keep_owner` is not bound; reflect-hit tracking already exists. |
| Ness | Model-animation bundle declaration alignment/triplet metadata. |

Remaining work includes admitting every complete source and owned resource
graph, completing native callback delivery, activating real clones, and comparing
whole-fighter behavior with native stock. Shared primitive comparisons and older
C-export comparisons are useful but do not prove the final interpreter roster.
Compact whole-match rollback remains a separate open part of the project goal.
