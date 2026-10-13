# Common idle, statistics and jump-sound scheduling

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the [current plan](../../PROJECT_PLAN.md) and
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

Mewtwo's body audit reached three unsupported final Fighter hooks after the
presentation/policy cohort. This batch connects all three together:

- `idle_variants`: Bool query after the current idle animation ends and only
  when a choice table exists. Weighted selection/restart remains shared code.
- `motion_is_angled`: Bool query with the exact I32 `event.motion`, replacing
  the stock classification at the player-statistics decision point.
- `worn_item_jump_sound`: Bool query before shared ItemScrew aerial launch and
  native multiple-jump entry. An explicit authored `jump.begin` argument or
  earlier `aerial_jump` replacement still owns its own sound policy.

Each shared caller retains Fighter/program/storage lifetime and stops after
fault or retirement. The multiple-jump caller also retains its cached jump
attributes before using them after a callback or jump entry. Missing hooks
preserve the existing common/native policy; failed declared hooks never
delegate to donor fighter handlers. No new language syntax, fighter-specific
API or forwarding wrapper was added. See [query hooks](scripting/language.md)
and [worn-item policy](scripting/common_policies.md#on-worn_item_jump_sound).

The native Shadow Ball explosion (`it_802C5E5C`) samples its grandchild's world
position, then calls the Position-ABI effect 0x40E. The template now uses the
existing `item.joint_world_position` and `item.spawn_effect_at_position`
operations instead of an unsupported joint-attached effect. Its explosion
charge clamp also now matches the native range checks. No effect-binding
extension was required. This corrects the previous checkpoint's inference
that a new joint-path effect operation was necessary.

## Evidence

- Shared scheduler cohort: **4,233 checks**, including **388 comparisons**
  against before-integration native decisions and **32 fault/retirement cases**
  across four call sites. Native comparisons cover motion IDs -2 through 375,
  idle-table/remaining-frame/policy combinations and both ItemScrew sound
  outcomes. Two additional multiple-jump cases check sound delivery and the
  post-entry path. Actual compiler, prepared hooks, actor storage and interpreter
  execute; scheduler bodies are mechanically extracted from production source.
  Before-integration bodies are extracted from local commit `5a9b7e468`.
  `A:/FreakMelee-test-archives/script-common-policy-schedulers-oct10-final/receipt.json`
  SHA-256 `58c28fa172c0efb4105b16bf317a5cfe45631d73db6c91384e65264f2606dd39`,
  1,711 independently rehashed inputs.
- Fresh full-roster audit: **19/26 forms prepare**, excluding Kirby and counting
  Sheik/Nana separately. Mewtwo's **182 bodies now compile**, and the complete
  package prepares with 47 imports and 3,942 instructions. Jigglypuff still
  reports ten failing bodies; Fox/Falco and Pikachu/Pichu stop at prerequisite
  entity/import metadata, Ness at animation resources, and Samus has no valid
  independent draft in this audit. Preparation is not finished clone gameplay.
  `A:/FreakMelee-test-archives/source-full-roster-audit-oct10-common-policies-final/receipt.json`
  SHA-256 `709bcb992431bba53e8340cf1d6c4af402c3345866d79666fae5409d6adeb1fb`,
  189 independently rehashed inputs.
- Seven Windows targets build; `native_fighter_script` reference CTest and
  diff check pass. Independent pins are recorded in
  `A:/FreakMelee-test-archives/common-policy-independent-pins-oct10.json`.

Animation data/pose, RNG and inner jump/physics leaves are controlled in the
scheduler fixture; Item actors and particle rendering are absent there. The
template's effect sampling and clamp have source/preparation evidence in this
batch, not a new full-article runtime comparison. No complete Mewtwo activation,
live match, particle appearance or whole-fighter parity is claimed. No game
window, audio, installed package or user-draft changes were needed.

Continue the [grouped inventory](native_script_multi_error_audit.md), then
whole-fighter/paired behavior acceptance and compact whole-match rollback.
Goal active.
