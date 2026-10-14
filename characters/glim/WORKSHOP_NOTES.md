# Glim: Workshop observations

This log separates actual UI use from batch asset authoring. Batch tools call
the same project backend, but they do not prove the corresponding UI is useful.

Initial observations:

- Cloning through the command-line tool copies assets but supplies no behavior
  file. The Workshop supplies a starter template separately. A standalone
  authoring recipe needs to provide its source explicitly.
- Models can import one glTF primitive at a time. A complete character with
  several materials requires repeated import review and Apply operations.
- The outliner uses `Mesh 0`, `Joint 0`, and similar fallback names for the
  stock costume. Choosing attachments requires consulting the rig role map.
- Removing unused stock special states and assigning new action names are
  separate from editing their scripts. The clone leaves a crowded action list.

The completed source preparation accepts all 80 functions, checks every required
motion callback, and uses 889 interpreter instructions with no imported
attribute-offset declarations. The two aerial jumps and four specials use
owned parameters and state variables.

The offline animation helper rejected repeated stock clip names. Glim's recipe
now appends its own clips and updates only the authored action rows, preserving
every inherited offset and shared clip. New curves use sparse linear keys so
their handles remain practical to edit.

Timed-hitbox dogfooding exposed a runtime flag issue: retail reads the
victim-only flag from the next command. A duration word cannot safely serve
that role. Timed hitboxes now read their own header flag; stock conversion
copies the retail flag into that header. Ordinary retail commands keep their
original behavior.

## Live checks

- Opened Glim in the actual Workshop; scrubbed the authored down-air hitboxes.
- Used the Scripting file list and Build behavior button. The draft validated
  successfully, and Test Character entered live practice with Glim and Sandbag.
- Changed Ward's radius from 8 to 9 through the Parameters panel, saved its
  override and retained it in the reproducible source. Applied a hitbox edit
  using a radius of 5 game units and saved the draft.
- Ran a finite 1,500-input gameplay exercise with all four specials, both air
  jumps, neutral air and down air. Charged Flash dealt 11 damage to Sandbag;
  Skip dealt 7. The game completed the exercise with a normal exit.
- Inspected the new halo and costume in Model mode and frozen Action preview.
  Adding the halo exposed missing draw-pass flags when importing a mesh onto
  a transform-only joint. The importer now enables that joint and its ancestors.

The exercise proves the normal move path and those contacts. Projectile
reflection, the dash's wall bounce and competitive balance need further play.
The move scripts use ordinary fighter variables, parameters and engine calls.
No Glim-specific engine or language operation was added.

## Editing gaps

- Parameter defaults showed the declaration's `=` token in numeric fields.
  The panel now shows just the declared value, including correct boolean defaults.
- Hitbox radius and offsets showed packed integers such as `1280`. The inspector
  now shows and accepts distances in game units.
- A fresh script needs to distinguish grounded and aerial motion names, even
  after changing the fighter's ground/air state. The names should be linked
  from the move-entry documentation and examples.
- Editing charge damage in an animation callback can miss the first contact
  with a newly created hitbox. An accessory helper runs after the action events
  and before hit detection. The hitbox reference now explains that ordering.
- Several original facial expressions still choose textures through material
  animations. Replacing a material's displayed texture does not replace every
  texture used by its animated expressions. Editing those texture sequences
  would make whole-costume recoloring easier.
- New animation authoring still takes external setup to add an independently
  named action clip. The Workshop can edit the resulting keys, but a clear
  "Duplicate clip / New clip" workflow would make a fresh move easier to start.

The reproducible recipe did the bulk asset authoring. The UI checks above
cover the actual inspection, build and preview workflow; they are not evidence
that every asset-creation task can be completed entirely through the interface.

Final local evidence (2026-10-14):

- Editable project: `%LOCALAPPDATA%/Freak Melee/projects/Glim`.
- Installed package: `f7bf260b8c2c815aeff469120e830f0d9a9be98406677702bc70e6ebdc63dcf4`.
- Finite gameplay receipt: `C:/FreakMelee-review/glim/final-gameplay/receipt.json`.
- Screenshots: `C:/FreakMelee-review/glim/screenshots`.
- Native build and timed-hitbox lifetime checks passed. The final package
  verified with 80 script bodies and no imported attribute offsets.

The existing drafts and installed fighters were preserved. Glim updates replace
its prior installed revision; temporary preview packages remain in the cache.

## Follow-up authoring pass

The native Workshop now has New from rest pose / Duplicate selected clip,
multi-mesh import, saved bone/mesh/material/action names, action-list hiding and
animated texture-image replacement. See the
[authoring guide](../../melee/docs/native_workshop_asset_authoring.md).

A batch against disposable copies of Glim checked independent clip edits,
saved undo with an intervening attribute edit, rest-clip key insertion, invalid
input preservation, name persistence, two-mesh import, second-mesh failure,
single sequence-image replacement and preservation of sampler/UV settings.
Edited drafts also passed source preparation. Those checks cover data handling;
the user's next dogfooding pass will assess the new controls.

A live multiple-mesh import exposed stale material-animation and visibility
references after the mesh traversal changed. The importer now remaps those
references with the meshes and saves the fighter/costume pair together. Model
undo includes the reference edits; action/attribute undo preserves the current
costume references. No fighter-specific exception was added.

Follow-up evidence (2026-10-14):

- Backend receipt: `C:/FreakMelee-review/2026-10-14-authoring/batch/run-20261014-135626/receipt.json`
  records 34 successful operations, including reference identity checks and
  source-package preparation.
- Live controls exercised name/hide/show, model naming, independent rest/copy
  clips, Undo/Redo/Save, and two-mesh import with preview refresh and Undo/Redo.
- The saved UI test draft passed source preparation after the import. Live
  screenshots and logs are in `C:/FreakMelee-review/2026-10-14-authoring/ui`.
- The real Glim and Rook drafts and installed packages were left unchanged.
