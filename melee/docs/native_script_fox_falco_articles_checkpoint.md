# Fox/Falco owned article source checkpoint

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Fox and Falco now have separate ordinary-interpreter templates:
[Fox](../examples/fox_wip.fms) and [Falco](../examples/falco_wip.fms).
Workshop selection, shipped UI assets and the existing comparison runners use
the matching file directly. The former shared template is removed.

Each source declares only its own gun, laser and side-special trail. Factory
calls name those declarations, and gun signals use the existing four-argument
`articles.signal_owned(handle, Article, "signal", value)` API. No new native
binding, stock-kind dispatcher, wrapper or compatibility path was added.
Separate sources intentionally let an author change one fighter's behavior
without also changing the other fighter's template.

Article identity no longer selects CPU, sound or held-item policies. Those are
plain editable parameters: CPU capture enablement, whether neutral-B can start
while holding an item, radio speaker and the four blaster sound choices. Stock
defaults differ per source. The two imported DAT kind fields are removed;
angle, speed and landing lag remain imported tuning. Fox keeps its presentation
gun callback; Falco has no such callback, matching the original fighter switches.

## Side-special construction

The fighter uses `articles.create_at_position` with its current XYZ position,
facing, zero collision group and a creation value of 0 (ground) or 1 (air).
The authored trail's `on created` initializes lifetime, disables surface
response, enables manual lifetime, chooses that state, inherits owner scale and
debug display, then refreshes physics. These steps correspond to stock
`ftFox_SpecialS_CreateGhostItem`, `it_8029CEB4` and `it_8029CFF0` in
`ftfoxspecials.c` / `itfoxillusion.c`. The initial collision flag stays enabled,
as in `Item_InitSpawn`. Failed creation retains the -1 handle sentinel.

This is a source-backed construction translation. A full rendered match with
the interpreted fighter is still pending package admission; it is not being
claimed as new gameplay parity evidence.

## Checks and remaining failures

- Seven Windows targets build. Script and parameter CTests pass.
- Generic source factory cohort: **67 cases / 478 checks**. Actual compiler,
  actor/VM, authored constructor and publication guards are exercised; model,
  collision and unrelated engine leaves remain controlled.
- Direct authored signal cohort: **195 checks**. Actual compiler, prepared
  signal identities, actor storage, VM, ownership and lifetime guards are used.
  It does not construct native DAT models or establish gameplay acceptance.
- Final isolated stock-copy/import/audit checks **256 Fox / 253 Falco bodies**;
  metadata and body traversal are complete. Each reports **16 failing bodies**,
  down from 27 before this batch. Neither package is admitted.

The audit still reports each body's first failure. The remaining groups include
rotated effects, reflector wind, stage/radio calls, bone segment-angle queries
for throw shots, gun/laser receiver and collision observations, trail model
duplication and hitlag-request clearing. Fixing one can expose another within
the same body; 16 is not an exhaustive count of missing operations.

Final evidence:

- `A:/FreakMelee-test-archives/fox-falco-owned-articles-oct10-c/receipt.json`
- `A:/FreakMelee-test-archives/fox-falco-article-factories-oct10-b/receipt.json`
- `A:/FreakMelee-test-archives/fox-falco-article-signals-oct10-c/receipt.json`
- `A:/FreakMelee-test-archives/fox-falco-owned-articles-independent-oct10-c/receipt.json`

Earlier factory `-a` and signal `-a` / `-b` runs failed because the fixture
runner had stale dependency lists (including a `.c.c` filename). Those are
retained as failed attempts. The new runs fix those lists and explicitly mark
unselected newer APIs unavailable rather than inventing successful behavior.
The first owned-source audit `-a` preceded the final source-owned policy edits;
`-b` added those policies; `-c` records the final bytes after trailing-blank-line
cleanup. The first independent receipt attempt checked for an `.exe`
where the script fixture target is a `.dll`; its CTests passed but no receipt
was produced. The final independent `-c` receipt records the actual target type,
rehashing 20 audit / 1,704 factory / 1,697 signal inputs. Normal
installed packages and user drafts were not modified.

The [older translation ledger](fox_falco_script_translation.md) records
generated-C comparisons; those are historical evidence, not acceptance of the
current interpreter sources. Full fighter and compact rollback acceptance
remain open. The project goal remains active.
