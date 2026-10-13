# Interpreted auxiliary-item and jab schedulers — 2026-10-10

Pikachu and Pichu now pass complete package preparation: all **210 bodies each**
compile without diagnostics and all declared hooks admit. Private roster
preparation is **24/26 forms**, excluding Kirby. Ness retains an animation-resource
metadata prerequisite; Samus lacks an independent draft in this census.
Preparation is not complete fighter gameplay acceptance.

## Connected contracts

- Auxiliary attachment/reset run at the actual shared bunny-hood/flower callsites.
  Preserve the stock OR gate: skip only when both auxiliary slots are occupied.
  Assign slots/timers before attachment callbacks. Source actors never fall back
  to native/stock handlers, including absent hooks. Detached actors retain those paths.
- `jab_start` returns integer motion flags before shared jab selection. Source can
  register a state-change helper and return `Transition.skip_attack_count`.
  Existing entry lifetime checks stop on faults/removal; no stock special delegation.
- Attachment continuations retain fighter/item identities, reject changed slots
  and stop on faults/removal before common attribute/model work. Reset callers
  also stop on VM faults. Paired transfer already validates programs/item identities.
- Rename the private unnamed attachment inline directly. No adapter, compatibility
  alias or tick allocation. [Hook reference](scripting/common_policies.md#on-jab_start).

## Verification

- Combined actual compiler/program/actor/VM/scheduler fixture: **3,126 checks**.
  New cases cover four slot occupancies, assignment ordering, both attachment paths,
  reset, native/stock dispatch, absent source hooks, continuation on success,
  fault/removal suppression and jab flags. Existing input/CPU/callback checks remain.
  Item/sound/model leaves are controlled; actual attribute reload runs with fixture data.
- Seven Windows targets build; script/parameter CTests pass **2/2**.
- Fresh 26-form audit passes **24/26**, using private current Pikachu/Pichu imports.
  Independent rehash verifies **1,708/201 pins**, shipping templates and additional
  production/build-log inputs. Initial fixture failures remain archived: wrong leaf
  signatures, missing shared attributes/header, incorrect fractional timer expectation
  and an unset bunny-hood attribute table. No production guard was relaxed.

| Evidence | Receipt SHA-256 |
| --- | --- |
| `A:/FreakMelee-test-archives/script-fighter-auxiliary-oct10-final/receipt.json` | `c71085096c4d223cc13f682d44b135093acd2c93c1e05fbe0b92b0f48892a22d` |
| `A:/FreakMelee-test-archives/source-full-roster-audit-oct10-scheduler-final/receipt.json` | `261e64c5ad9dc26f5794c74ae34e672e4e964d05b282c22fe17bc1f0981ef179` |
| Independent rehash | `A:/FreakMelee-test-archives/auxiliary-scheduler-independent-oct10.json` |

No game window/audio or normal content/draft changes. Next: connected interpreted
Pikachu/Pichu gameplay comparisons, Ness metadata and Samus independent draft.
Goal remains active.

Follow-up: the [connected interpreter gameplay checkpoint](native_script_pikachu_gameplay_checkpoint.md)
records actual package publication and replay comparisons, including the generic
empty-visibility-selector correction and authored Skull Bash effect bookkeeping.
