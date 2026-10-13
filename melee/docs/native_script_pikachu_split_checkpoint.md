# Pikachu/Pichu interpreter source preparation — 2026-10-10

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Pikachu and Pichu now have separate complete source templates. Each template
keeps its own three owned articles; neither needs the other fighter's assets.
The first separated-source audit visited all **209 bodies per fighter** and
reported 35 failing bodies each. A grouped source/API correction reduces that
to **21 per fighter**, with complete metadata and body traversal. Both packages
remain rejected until those failures and later integration gates are resolved.

## Source and runtime changes

- Article factories and signals identify their package-owned declaration by
  name, retaining native ItemKind constants for ordinary comparisons.
- Article reads of Fighter tuning use explicit owner parameter receivers;
  article-local parameters retain their existing scope.
- Bone-attached effects use the existing raw-part operation with semantic bone
  mapping. RNG calls, costume indices, integral ledge directions and the initial
  Skull Bash integer charge use the existing typed interpreter contracts.
- Generic `physics.clear_kinetics()` calls the original common engine helper
  directly. It resets self movement, ground movement, knockback, attacker shield
  knockback and the additional velocity channel, without changing animation or
  ground/air state. No allocation, callbacks, stock-special delegation or
  fighter-specific scripting API was introduced.
- Workshop template selection, shipping CMake inputs and comparison-tool source
  paths now select the appropriate fighter source. The obsolete combined source
  was removed; historical audit hashes remain historical. Existing generated-C
  comparison harnesses were only updated to locate the separated source and
  syntax checked, not certified as current interpreter gameplay tests.

[Pikachu source](../examples/pikachu_wip.fms) |
[Pichu source](../examples/pichu_wip.fms) |
[Movement reset](scripting/fighter.md#physicsclear_kinetics)

## Current remaining cohort

The audit reports the first failure in each body, so this is a grouped inventory,
not a count of every unsupported operation. Both fighters report the same sites:

| Capability | Exposed sites |
| --- | --- |
| Linked article relationships | Link existing Thunder segments; create/query reciprocal controller/model links; read linked positions; unlink at destruction/contact; synchronous linked signals. |
| Item terrain observations | Segment sweep/contact normal storage; ground checks. |
| Indexed model observations | Existing article bone world-position reads. |
| Typed Fighter observations | Action-category filtering in a Thunder collision callback. |

The [linked-article contract](scripting/article_links.md) is now explicitly marked
as pending interpreter migration, avoiding a claim that these operations already
work in the current backend. These generic operations are the next batch. No
Pikachu/Pichu package was installed or admitted, and no rendered gameplay parity
is claimed for these source changes.

## Regression and roster evidence

The movement fixture executes the actual selected native bodies and production
parser, typed lowering, binding dispatcher and VM: **26,094 checks, 7,104
native-body comparisons and 91 double-sensitive cases**. The reset adds 128
native-helper comparisons and appears in 64 VM runs. All reset channels begin
with nonzero values and are included in the observations. Unselected binding
implementations fail closed; the fixture has no published source Fighter actor,
renderer or complete gameplay state transition. Its empty GObj pool uses the
real pool-reference implementation. The older fixture dependency list was
updated, and sources, compiler and project/Dolphin headers are pinned.

A fresh read-only roster audit on the final build admits preparation for
**22/26 forms**, excluding Kirby and counting Sheik/Nana separately. Pikachu and
Pichu reach all 209 bodies with 21 failures each; Ness stops at animation-resource
metadata; Samus still lacks a valid independent draft in this census. Preparation
success does not establish full clone gameplay acceptance for those 22 forms.

All seven selected Windows targets build; script and parameter CTests pass
**2/2**. Shipped Pikachu/Pichu sources match their repository bytes. The isolated
CLI/console work opened no game window, played no audio and changed no normal
installed content or Workshop draft.

## Receipts

Under `A:/FreakMelee-test-archives/`:

| Receipt | SHA256 |
| --- | --- |
| `source-full-roster-audit-oct10-pikachu-split/receipt.json` | `6d0530961638f2a560f8ba1c2c6ad1d4c2fca9eb30ce60ad40b28d83fa448f5d` |
| `script-clear-kinetics-oct10-e/receipt.json` | `2aa1a485f33d8330f31eba2e58ff33e851d9c87320b8fee6b078005a94232595` |
| `pikachu-split-independent-oct10-a/receipt.json` | `ace4b1992f3bfc1cef4893b752e8b520e17401ede079a865e8bb1dc0e7145697` |

The independent closure receipt rehashes 182 and 1,658 input pins, confirms
shipping sources/Python syntax, and retains final build/CTest logs. Initial
failed audit/fixture attempts remain archived and are not passing evidence.
Goal remains active.

## Subsequent linked-article batch

The [generic link/signal checkpoint](native_script_article_links_checkpoint.md)
reduces the current failing-body count to **9 per fighter**, still with 209
visited bodies and complete metadata. The 21-failure receipts above describe
this earlier split checkpoint, not the latest build. Gameplay acceptance is
still pending.
