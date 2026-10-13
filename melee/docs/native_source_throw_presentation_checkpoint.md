# Common throw presentation and native timer checkpoint

The generic interpreted `on throw_presentation` event now runs at the original
accessory slot in `ftCo_800DD398`: holder motion and initial animation, capture
mask, complete victim thrown setup, presentation, then common invincibility.
Its read-only payload contains the holder's Int motion and Float playback rate.
See the [capture reference](scripting/capture.md#on-throw_presentation).

This slot differs from the earlier victim-setup hook and subsequent per-frame
throw callbacks. Scripts own presentation decisions; an absent hook is a no-op
and never delegates to a stock fighter constructor. Unattached native holders
keep their original native policy. Entry and continuation retain the existing
capture-pair identity, including physical and program lifetimes and both
reciprocal links. Faults, retirement and broken links stop the common tail.
The event adds no tick allocation or raw pointer access.

## Confirmed native portability correction

The same batch exposed an incorrect duration field in the decompilation:
`Fighter.x1994` was declared `bool`, although its callers store and decrement
multi-frame invincibility durations. Console MSL's `src/MSL/stdbool.h` defines
bool as signed int; native C bool instead normalizes a duration such as 20 to 1.
The fixture observed that normalization before the fix.

The field is now explicitly `s32 throw_invincibility_frames`. Initialization,
maximum-duration grants, decrement, expiry and overlay predicates use that
field directly. The algorithm is unchanged. This restores integer countdown
semantics rather than introducing a gameplay clamp or workaround. Other bool
fields were not changed without evidence.

## Evidence and limits

Connected batch receipt:
`A:/FreakMelee-test-archives/throw-presentation-oct11-final/receipt.json`.
The capture scheduler cohort passes 106,625 checks across 9,764 cases, using
the actual source compiler, prepared actors, VM and capture/lifetime machinery.

The new 62 presentation cases cover all four throws, native and owned victims,
three rates, missing hooks, callback faults/retirement, reciprocal identity
rejection and retirement during holder/victim initialization. Callback
observation requires completed victim setup and precedes the invincibility
grant. Native Mario, Samus and Kirby holders are compared by whole Fighter
bytes and observer order against the exact pre-event entry body at `55124e067`.
That reference uses the corrected field type too; it verifies entry behavior,
not preservation of the old native bool bug.

Another 41 cases exercise zero, one and multi-frame countdowns, remaining
intangible conditions, overlay presence, expiry and longer existing grants.
The grant body and fighter tick slice are actual source. Only the tick slice's
overlay-clear call is redirected to an observer; overlay query, motion entry,
animation and stock Samus accessory construction are controlled leaves.
Full fighter ticking, retail graph construction and rendered accessories are
outside this fixture. New unselected statistics/rebound/shake leaves abort
if reached. The receipt verifies its pinned inputs did not change during the
batch.

Seven native targets build successfully. The focused `native_fighter_script`
and `native_fighter_parameters` CTests pass. No game was launched or installed
content changed for this checkpoint.

Samus remains an unregistered authoring candidate. Structured accessory asset
recipes, dynamic presentation controls, conditional non-looping joint rates,
complete constructor/solver/lifecycle closure and gameplay acceptance remain
open. The new event resolves its timing prerequisite, not the constructor.
The full project goal remains active.
