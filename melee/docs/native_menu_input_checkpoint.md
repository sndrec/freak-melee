# Menu input ownership: 2026-10-20

Holding A activates a hub control once. Release A before activating another
control. Moving the hand onto a control while A is already held does not click
it. B first dismisses text editing, then backs out on a new press. B takes
priority when A and B arrive together.

Holding B on character select still opens the hub. A B press used to close the
hub must be released before starting that gesture. Keyboard Enter/Space button
activation and Escape also act once per press. Repeated text editing, pointer
movement and arrow-key navigation stay available.

## Cause and changes

The native game loop clears `HSD_PadCopyStatus` while a menu owns input. Its next
renewal previously calculated button edges against that cleared status, making
held buttons appear newly pressed each tick. The scheduler now renews
`HSD_PadGameStatus`, which retains physical button history, and copies the
processed sample into `HSD_PadCopyStatus`. Menus can consume or remap that copy
for the current tick. The next tick starts from physical history again.

This keeps the existing clamp, scaling, profiles, sample cadence and snapshot
state. It adds no input-history fields or allocations. Results port remapping
also uses the same working copy and cannot change the next physical edges.

Additional fixes:

- The CSS cursor consumes A/B/X/Y trigger and repeat bits after a native cookie,
  outfit, delivery or toolbar handler owns them. Later CSS objects cannot use
  those edges again in the same tick. START keeps its separate readiness path.
- The B-hold return gesture starts from a new B press on CSS.
- The hub handles one controller action per sample, with B before A or START.
- Quick Play's practice countdown checks the freshly renewed input rather than
  the previous working copy. Unready consumes B before the hub/practice handlers.
- Both the GX hub and the RmlUi event entry point filter repeated keyboard
  activation/back events. Textarea newlines and editing/navigation retain repeat.

## Audit scope

| Path | Input ownership |
| --- | --- |
| Hub navigation, lobby forms/listing, Quick Play/stage pages, accounts, friends, letters and Workshop landing page | Shared hub trigger handling and queued control activation. |
| CSS cookies, outfit cycling, portrait delivery and play controls | Native cursor handler consumes handled button edges; mouse boundaries retain their existing queue. |
| Practice play controls and countdown cancellation | Pointer controls activate on press; countdown B runs on the current sample. |
| Controls, rules, costumes, collection, CSS layout and detailed Workshop | RmlUi mouse/keyboard events; keyboard activation/back repeat is filtered before dispatch. |
| Controller rebinding | Existing raw-device capture waits for neutral before accepting a held binding. Gameplay input remains blocked while the tool owns it. |

## Evidence

The native build passes in `.local/ui-input-audit-build-final.log`.
Three focused contracts pass in `.local/ui-input-audit-contracts.log`:
input scheduling/history, Quick Play input ownership and profile input.
The input regression covers four ports, held/released/repressed buttons, full
UI consumption and Results-style remapping. Compiling it with the previous
scheduler fails with `consumed or remapped input manufactured a new press`:
`.local/ui-input-audit-before.log`.

Seven hidden, muted live checks pass in `.local/ui-input-audit-4/receipt.json`:
listing toggle holds/represses, mouse clicks and movement during holds, text-field
cancellation and simultaneous A+B, nested Back navigation, close/reopen B holds,
SDL Enter auto-repeat on a Controls button, and Escape auto-repeat after binding
cancellation. Windows key messages target only the test's hidden window.

Executable SHA-256:
`8ccc11916a6bdd1ed8879e27eee5a249c7e342d203a0632ca22aceaffcb86df5`.
The first UI attempt needed hidden-hub inspection handling; a later attempt
needed to open the Bindings tab before clicking its button. The final complete
run includes both fixture corrections. Test-owned clients are stopped.

The live controller checks inject samples through the native input pipeline.
Physical GameCube adapter calibration and the user's wider visual/input review
remain deferred. CSS edge consumption has source review; the countdown handler
has focused ownership tests rather than a new networked-match run. Artwork and
layout are unchanged. The broader project goal remains paused.
