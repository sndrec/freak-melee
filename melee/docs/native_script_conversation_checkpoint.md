# Generic stage events and action statistics — 2026-10-10

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Fox and Falco's owned sources now prepare the conversation and reflector-jump
bodies through the interpreter. Their final audit reports one failing body per
form, down from six. Neither fighter is admitted yet: the trail's
`item.clone_model(1)` still needs a real independent model-graph operation.

## Source and engine contract

- `controls.try_aerial_jump()` already executes the stock common jump primitive
  used by reflector IASA. The templates now use that spelling directly; there
  is no new alias or jump handler.
- `stage.event(channel, value)` dispatches the existing native `Stage_Event`
  provider. Channels are full UInt32 IDs, with finite Float payloads and guarded
  result publication. The generic operation is available to Fighter and Article
  sources; stock conversation channels are native constants in the catalog.
- `stats.action_count(slot, action_id)` reads the actual bounds-checked
  `pl_800386D8` counter. Conversation counter `0x72` lies beyond the decompiler's
  partial array, inside its containing statistics object; no guessed array
  bound or separate conversation counter is introduced.
- `stats.notify_taunt()` passes the source logical slot and secondary-member flag
  to common native statistics code. `fighter.player_slot` exposes that logical
  slot; it is not a controller port or actor handle.
- `input.down_press_age` reads the existing saturated byte timer. Stock
  `fighter.c` resets it on a D-pad-down **press**, not release. The sources keep
  the original condition: down no longer held and press age exactly one.
- The script itself installs `conversation_cancel` in `callbacks.death1` when
  the stage accepts the start event. That helper cancels the stage event and
  clears its own slot. This reproduces stock per-instance callback ownership;
  it does not install a fighter-wide unconditional death callback or call the
  stock Fox conversation handler. Radio speaker remains an editable owned
  parameter, independently defined for Fox and Falco.

Reference: `src/melee/ft/kinds/ftFox/ftfoxappeals.c`,
`src/melee/ft/kinds/ftFox/ftfoxspeciallw.c`, `src/melee/ft/fighter.c`,
`src/melee/gr/stage_events.c`, `src/melee/pl/plbonus.c` and
`src/melee/pl/plbonuslib.c`. Authoring reference:
[stage events](scripting/stage_queries.md),
[statistics](scripting/fighter_observations.md),
[callback ownership](scripting/callback_slots.md).

## Verification

All seven Windows targets built. Script and parameter CTests passed (2/2).
No game or Dolphin process was launched; installed content and user drafts were
not changed. These checks do not establish complete fighter/gameplay or compact
rollback acceptance.

- Stage cohort: **22,330 checks / 2,053 cases / 1,830 comparisons / 751 VM
  calls**, including full-width channels, exact payload bits, absent/Boolean
  provider results, source retirement and interpreted dispatch. It executes the
  actual stage dispatcher with a controlled provider, not a stock conversation
  stage. Existing Fighter/Article stage-query coverage remains included; the
  new event-specific VM case uses a Fighter.
- Observation cohort: **20,463 checks / 2,441 cases / 3,563 comparisons / 1,560
  VM calls**, plus 576 recipient calls. The new statistics cases execute the
  actual counter reader over controlled six-player tables, including `0x72`,
  bounds/types, all timer byte values and interpreted observations. Taunt
  notification is a controlled leaf checking exact slot/member arguments and
  retirement; this is not full bonus-accounting gameplay execution.
- Fresh isolated audits visit **256 Fox / 253 Falco bodies**, with complete
  metadata and body traversal. Each reports only the trail model duplication
  failure. A body audit still stops at its first error; subsequent errors in
  that body may appear when duplication is implemented.

Receipts are under `A:/FreakMelee-test-archives/`:

| Directory | Receipt SHA-256 | Rehashed inputs |
| --- | --- | --- |
| `fox-falco-stage-events-oct10-b` | `44d467d58727424935c04d25fa467a41f36310368e817ea6b64d9452003b6664` | 1,762 |
| `fox-falco-statistics-oct10-b` | `067a786cd36d4f4e3bded68045c1c6f903c6d370bafd941fd1cbf68f71a5d5d0` | 1,728 |
| `fox-falco-conversation-audit-oct10-a` | `9b754e2c24bca85840ed853c590f4e19e8961f546640e5696a1318734ccb7be7` | 20 |

`fox-falco-conversation-independent-oct10-a/receipt.json` independently rehashes
all three input sets, the seven binaries and copied build/CTest logs, and checks
that the shipping Fox/Falco source bytes match the templates. Initial fixture
link failures are retained in the `-a` stage/statistics directories: outdated
unavailable-family duplication and a missing held-kind leaf. The latter now
links the actual native reader, not a successful substitute.

Full fighter and compact rollback acceptance remain open. Goal active.
