# Interpreted model animation component checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Ness's stock Yo-yo installs a material animation with NULL joint and shape
arguments in `itnessyoyo.c`. Its `Spin material 88` declaration cannot be read
as a consecutive three-pointer bundle. The direct interpreter now uses the
existing generic typed DAT importer for explicit joint/material/shape components,
optional channels and indexed pointer arrays. Shorthand triplets remain a
convenient declaration of those same three components.

The source importer, compiler declaration resolver, prepared asset identity and
literal root/model callers all use `native_script_model_metadata` directly. The
former root-only parser is removed, with build and private fixture callsites
updated. No stock Ness callback delegation or per-fighter parser rule is added.
Offsets and indices are retained in the immutable prepared records and checked
against the exact source declaration before publication. Parsing allocates no
memory and publishes output only on success.

See [article models](scripting/article_models.md) for the language and runtime
operations. This changes descriptor selection, not the supported live animation
channel/callback families.

## Validation

- Seven Windows targets rebuilt successfully; script and parameter CTests 2/2.
- Descriptor/component batch: 552 checks, 52 cases, seven entity preparations,
  40 native scalar comparisons. Real owned Link/Young Link triplets still select
  their original descriptors. Ness selects the exact material-only graph from
  its actual copied retail assets. Invalid syntax, duplicates, indices, extents,
  relocations and changed declaration/asset identities are rejected.
- Real Item actor/VM batch: both Link forms passed 1,465 checks/48 cases each,
  with 110 native graph comparisons, eight playback frames and two page restores
  each. Primary-only cases passed 129 checks/four cases each and 12 comparisons.
  The authored fixture uses an explicit component declaration for one real
  bundle and shorthand for the other, exercising both through actual compile,
  publication, Add/Request/Advance/Remove/rate and native controller comparisons.
- Ness's full current source audit now reaches all 418 indexed function bodies:
  metadata and body coverage complete, 56 first-errors-per-body, package rejected.
  Unsupported bindings and typed receiver/context differences are the next batch;
  passing this metadata prerequisite is not complete Ness activation.

Receipts:

- `A:/FreakMelee-test-archives/source-animation-components-oct10-a/receipt.json`
  SHA-256 `35a8aa1b97cc400b23694d9e13a3260a6655d1eb0a50fc1d5a989d95c517edcd`.
- `A:/FreakMelee-test-archives/source-item-animation-components-oct10-b/receipt.json`
  SHA-256 `04dc12cfdedda416c19e1688fb5dcb7ba090bcc65c80d54f0b1f4672579cb754`.
- `A:/FreakMelee-test-archives/source-animation-components-oct10-final/receipt.json`
  independently checks both frozen receipt pin sets and all current code, assets,
  libraries and fixture inputs. Only the two fixture READMEs changed afterward
  to document results; those documentation changes are recorded separately.

Descriptor tests do not initialize HSD live controllers. Item VM tests use
controlled private candidates, Item userdata and runtime tables; they do not
prove production constructor/scheduler scenes, renderer/material output, Ness
live Yo-yo behavior, whole-match rollback, audio, workshop or network acceptance.
