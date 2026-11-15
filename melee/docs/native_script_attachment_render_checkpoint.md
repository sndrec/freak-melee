# Owned attachment rendering checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The ordinary interpreter's costume attachments now participate in the shared
fighter render and visibility paths. The earlier attachment checkpoint connected
loading, cloth and cleanup, but `nativeFighterAttachment_Draw` and
`nativeFighterAttachment_Visibility` had no callers. Independent source fighters
correctly have no stock costume callbacks, so their imported hats never received
the accessory draw pass. This caused the blue-hat Rollout digest mismatch recorded
in the [owned-parameter checkpoint](native_script_owned_parameter_checkpoint.md).

Both common fighter drawing paths now draw owned attachments immediately after
the body, at the same location as the stock accessory callback. They retain the
existing view matrix, material/pass selection and fighter visibility gating.
The shared part-visibility operation also updates owned accessory groups; its
combined group-2 operation visits groups 2 and 3 as the stock path does. Binding
matrices, cloth setup and script APIs are unchanged. Empty fixed attachment slots
do nothing; there is no per-frame allocation or stock fighter delegation.

## Verification

- The seven Windows targets build. The script and parameter CTests pass.
- Four hidden, muted stock-versus-ordinary-interpreter pairs pass: both facing
  directions of the five-air-jump sequence, worn Bunny Hood, and blue-hat Rollout.
  The three jump cases each cover 900 controller samples; Rollout covers 1,200.
  Coverage checks require all five jump motions and the worn hood where relevant.
- A separate source-package batch compares all five Jigglypuff costumes through
  Rollout startup, travel, contact and recovery. Each pair matches 1,200 controller
  samples, 2,400 fighter states, 26 actions, two damage events and 1,200 RNG states.
  Nondefault costumes must actually produce an attachment. Pose, collision and
  accessory digests agree, including the cloth-hat variants. Each ordinary actor
  reports 1,200 requested ticks and no faults.
- Both batches create copied assets, source packages and accepted content in
  isolated test stores. No generated fighter DLL, normal collection or user draft
  is involved. This is focused runtime trace evidence, not screenshot comparison
  or acceptance of every special, shield-break case or whole-match rollback.

The costume runner now exercises the production source-package workflow rather
than generated modules, explicitly selects each skin's model-table variant and
checks actor health and unchanged inputs. Roster preparation remains at the prior
20/26 checkpoint; this render change does not resolve other fighters' metadata.

## Evidence

Both batches and their input/output hashes are independently checked in
`A:/FreakMelee-test-archives/attachment-render-independent-pins-oct10.json`, SHA-256
`509e48f16a3b5a120556fc2c639ce2893ca6498496b40222d891efac96bfd58b`.

- `purin-interpreted-attachment-oct10-a`: four passing pairs, 21 input pins.
- `purin-interpreted-costumes-oct10-a`: five passing pairs, 34 input pins, including
  the actual common render/visibility source and attachment implementation.

After the first batch finished, the costume runner's main workflow was migrated.
The first batch imports only its two unchanged asset helper functions. The
independent report retains the exact historically pinned helper file and verifies
both function ASTs against the current file; it does not claim that the entire
file stayed unchanged across that later migration. The second batch verifies all
its current inputs unchanged directly.

The overall goal remains active. Continue grouped preparation/resource work for
the remaining forms, full clone behavior coverage and compact rollback acceptance.
