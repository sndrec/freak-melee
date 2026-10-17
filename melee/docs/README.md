# Freak Melee documentation

Updated: 2026-10-17. Start with the current guides below. Dated checkpoint and
audit files preserve evidence for their recorded build; old "next"/"pending"
sentences are not the current work list. The [project plan](../../PROJECT_PLAN.md)
owns priorities and remaining acceptance. Optimization is currently deferred.

## Start here

| Need | Read |
| --- | --- |
| Build and launch | [Windows native build](native_build.md) |
| Current scope / what's next | [Project plan](../../PROJECT_PLAN.md) |
| This PC's tooling | [Local tooling](../../TOOLING.md) |
| Workshop capabilities and gaps | [Current Workshop status](native_character_workshop_progress.md) |
| Implement the approved editor | [Handoff](native_character_editor_handoff.md), [UI specification](native_character_workshop_ui.md), [Model specification](native_model_workspace_design.md) |
| Author a fighter | [Scripting guide](scripting/index.md) and [fighter-owned parameters](fighter_owned_parameters.md) |
| Try a custom fighter example | [Glim](../../characters/glim/README.md), [Workshop observations](../../characters/glim/WORKSHOP_NOTES.md) |
| Interpreter decisions | [Current contract](native_interpreter_transition.md) |
| Fighter translation coverage | [Roster status](SCRIPT_PORT_CHECKPOINT.md) |
| Production rollback | [Current architecture](native_rollback_current.md), [confirmed-checkpoint policy](native_confirmed_checkpoint_plan.md) |
| Home/practice and controls | [Home flow](native_home_flow.md), [profile UI](native_profiles_ui.md) |
| Rooms and content | [Room players](native_room_players.md), [netplay](native_netplay.md), [content packages](native_content_packages.md), [chat](native_chat.md) |
| Social interface and remaining work | [Social hub, roster and offline inbox](native_social_hub_design.md) |
| Native CSS cards and sharing | [Participant ordering, mouse input and delivery checkpoint](native_css_cards_checkpoint.md) |
| Quick Play progress | [Service allocation checkpoint](native_quick_play_checkpoint.md) |
| Early remote portraits | [Automatic lobby cache](native_lobby_cache_checkpoint.md) |
| Native CSS/practice controls | [Readiness, lobby access, delivery and tool return](native_play_menu_checkpoint.md) |
| Prepare a friend build | [Release guide](native_release.md) |
| Find recorded evidence | [Checkpoint index](checkpoint_index.md) |

Reference authors should follow the [documentation guidelines](documentation_guidelines.md).

## Reading implementation evidence

Checkpoint files establish exactly their recorded paths, sources, binaries and
test boundaries. A passing preparation fixture is not gameplay acceptance;
same-PC processes are not separate-network testing. Some roster cohorts combine
initial runs with corrected reruns rather than a sealed final-binary run.
Older generated-C, broad-page and import bring-up evidence is historical.

Large receipts, screenshots, binaries and profiling archives stay outside Git.
Their paths in reports may refer to another drive or a retained older baseline.
Do not assume every historical archive is currently installed or should be copied.

The private upstream's initial source commit preserves the documents before
this cleanup: [pre-cleanup source](https://github.com/sndrec/freak-melee/tree/4b04fe10b9495325f4ecbf434f4f9abd994d6dfa).
No useful recorded evidence was discarded to manufacture a completed milestone.

## Documentation responsibilities

- `PROJECT_PLAN.md`: current product contract, scope, priorities and explicit
  remaining acceptance. Keep it concise; no append-only session log.
- Current build/Workshop/interpreter/rollback guides: implementation readers'
  starting points. Update them when architecture changes.
- `native_character_workshop_ui.md` and `native_model_workspace_design.md`:
  approved authoring behavior/layout, separate from implemented capability.
- `scripting/`: linked author-facing language/API reference with reusable
  examples and clear context/lifetime/numeric contracts.
- Dated checkpoints/audits: evidence, with exact limitations. Preserve them
  rather than repeatedly restating their old TODO lists in current guides.

For new work update the relevant current guide and add a small evidence record
only when it adds useful proof. Do not duplicate the same status in every file.
Do not describe an old proposal as unfinished when newer code/checkpoints
implemented it. Check the live source before making implementation claims.

The inherited decomp contribution/tooling material remains historical reference;
the native build guide is authoritative for this Windows client. Publishing
friend builds and pushing donor repositories are separate actions from this
private source backup.
