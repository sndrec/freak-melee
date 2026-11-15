# Interpreted package-owned root parameters

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

`program.attrs.NAME` explicitly reads the executing package's immutable root
parameter from a fighter, article, item or model-effect scope. Local `attrs`
and `item.params` keep their exact owner scope. Mutable root state remains
inaccessible to another actor.

The Game & Watch audit found 32 reads at 21 Article source sites, covering 27
root parameters. The reference exporter emits these from the module's published
`script_parameters` array, not from a live creator's Fighter data. Substituting
`creator.attrs` would therefore add a lifetime dependency and change surviving
article behavior. The canonical template now uses the explicit shared namespace.

The compiler resolves root declaration IDs and scalar types at preparation.
Runtime reads use the executing actor's pinned storage layout and immutable
parameter table; no owner lookup, string lookup, allocation or copied root actor
view is needed. The new semantic remains a parameter read and cannot collide
with the bytecode helper/local marker bits. Parameter publication still accepts
only canonical declaration identities, preventing aliases from bypassing
duplicate-supply validation. Writes, unresolved imports, invalid types and
mismatched/retired storage fail without publishing a value.

## Integration evidence

Native client, content tool, C-reference exporter and selected test targets build.
All 40 selected integration and three reference CTests pass. The actual Execution
test reads both root and shadowing local parameters, retires the physical root
Fighter, and then runs the still-live Article against its pinned root values.
Compiler tests reject shared writes, mutable root state, local-only parameters
and scalar subscripts.

Focused receipts, both passing with unchanged input fingerprints:

- `A:/FreakMelee-test-archives/script-program-parameters-final/receipt.json`:
  294 checks, 38 cases and 71 VM calls across two real Item actors and independent
  banks. Actual source preparation, actors, Execution/VM, typed storage, GObj
  pools and packed Item snapshots execute. Source-extracted native creator
  capture/getter bodies execute without semantic rewriting. Typed bit reads,
  physical creator retirement, actor faults, immutable identities, alias-supply
  rejection and snapshot recovery are covered. Metadata/assets/import supply
  and unrelated engine leaves are controlled; DAT decoding, full Item
  construction, rendering and whole-match rollback are not proved here.
- `A:/FreakMelee-test-archives/script-program-attrs-reference-oct09-final/receipt.json`:
  72 checks and 11 cases through actual reference expression parsers, complete
  GenerateC and the parameter importer with a synthetic DAT. Root reads stay
  shared module-array reads, names retain their full supported width, and import
  preserves the explicit namespace while qualifying local values. This is a
  test-only oracle, not ordinary runtime execution.

The existing rejection of cross-owner executable helper calls and imported
`attribute uint` remains explicit. Shared immutable reads grant neither root
helper ownership nor an unspecified word-width DAT import contract.

`A:/FreakMelee-test-archives/source-program-parameters-census-oct09-final/receipt.json`
records the rebuilt tool and all ten full-template preparation results. Mario,
Dr. Mario and Zelda continue to prepare. Game & Watch now passes the Article
root-parameter boundary and stops at line1596: writing `item.airborne` is not yet
admitted. This is a real remaining operation contract, not evidence of whole
fighter completion. Peach still requires recipient exports; Link/Young Link,
Sheik and Climbers retain their secondary-resource metadata boundaries.

See [Program parameters](scripting/program.md) for author-facing usage and
[recipient exports](native_source_recipient_export_plan.md) for the next dispatch
contract. Full imported-asset, rendering, match and final rollback acceptance
remain separate. The overall goal stays active; commits stay local.

## Remaining Game & Watch Article batch

A read-only census of all ten Article bodies after line1349 identified four
remaining API gaps, rather than relying only on the first preparation error:

| Operation | Native contract to preserve |
| --- | --- |
| `item.airborne = …` | Panic pickup copies native ground/air before state selection and animation advance. Ordinary Article assignment writes `GA_Air`/`GA_Ground`; the Workshop holdable's state-based getter requires an explicit separate policy. |
| `item.joint_world_position(0, "0", x, y, z)` | Manhole reads the primary root's first child through `lb_8000B1CC`. Use prepared primary selectors, staged three-F32 outputs and source/graph guards. The reference lookup publishes zero outputs when the joint is absent. |
| `effects.wind_at_position(…)` from Article | `lb_800119DC` copies a supplied Vec3 into global wind data and needs no Fighter. Admit an actual Article caller, not merely broader catalog flags around Fighter-only capture. |
| `targets.smash_charge_active(handle)` | Native `ftLib_800876D4` tests `smash_attrs.state == 2`. Missing targets return false; foreign physical Fighters are permitted. Keep captured target and caller validation. |

Evidence: `itgamewatchpanic.c:61`, `itgamewatchmanhole.c:93`,
`itgamewatchfire.c:85`, `lb_00F9.c:1067` and `native_fighter_target.c:97`.
This bounded API census does not prove their runtime behavior or full source
parity. Preserve separate creator, attachment-owner and current damage-owner
relations when implementing the batch.
