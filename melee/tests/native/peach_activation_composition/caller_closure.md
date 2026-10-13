# Frozen57 Peach caller closure

The machine census in the reproduction's `audit.json` lists23 original-source
files outside the Peach/Peach-article implementation units, with exact lines and
SHA-256 hashes. The generated candidate contains no `ftPe_*` call, Peach fighter
union access or Peach item-union access. All30 retail rows are covered; retail356
is wholly null, and pluck352/353 intentionally have no input callback.

| Original caller | Owned policy / disposition |
|---|---|
| `fighter.c:1220..1230` | Grounded motion setup calls authored `on_grounded_motion_setup` before the original `has_float` fallback. Authored state resets Float availability. |
| `ft_08A1.c:96..107` | `on_neutral_enter` runs after common down/hammer/ground gates; current held Parasol notification and resource restoration are authored. Accepted55 executes the whole original/current neutral entry against generated policies. |
| `ft_0D27.c:71..88` | `on_air_resume_motion` selects the ordinary held special-fall continuation rather than the Peach370 donor branch. Accepted55 includes the actual mushroom selector. |
| `ftcpuattack.c:825..845,1637` | The post-common attack predicate and ranged-candidate policy are authored (`>343`, generic range32/candidate38 data). Accepted44 actual source covers the predicate and exact ranged source slice plus shared queue/probe bodies. No additional Peach recovery/cancel/travel branch appears in this census. |
| `ftdynamics.c:409..434` | Authored `on_dynamics_interaction_start`: first-active3 except the final set0. The generic dynamics engine remains shared; accepted44 compares the exact original branch. |
| `ftCo_AerialAction.c:10..49` | Manual/automatic Float inputs are authored replacement predicates and consume donor fallback. Accepted50 compares Float entry, five attacks and both fall directions. |
| `ftCo_JumpAerial.c:122,245` | `on_aerial_jump` owns Peach forward/backward selection, command0 and velocity setup. Shared jump initialization is the documented boundary in accepted44. |
| `ftCo_AttackS4.c:176` | `on_forward_smash_enter` replaces the stock random three-weapon entry. The script owns rejection sampling and ordinary `last_smash_motion`; accepted48 compares all three smash phases. |
| `ftCo_Landing.c:68` | `on_landing_action` owns saved/resource Parasol decisions. Shared landing phases remain engine callbacks; accepted55 compares the complete original/current entry. |
| `native_fighter_held_motion.c:34..36` | `on_held_motion` owns369/370 mapping from saved handle and the requested common motion. The selected common144/146 callbacks are directly bound, with no stock Peach wrappers. |
| `native_fighter_contact.c:33` | `on_body_rebound` owns the365/367 permission. Neutral counter/guard-hit and Spore creation are authored, accepted48; the contact helper is a shared dispatch boundary. |
| `fighter.c` held timer (no fighter-kind mention) | Authored `on_held_timer_expired` implements the current held-item decision, reopen block and runtime freefall. Accepted55 includes the exact production timer block; this audit does not claim a whole fighter tick. |
| `ftCo_AppealS.c:105..106` | Only `DbLevel>=DebugRom` toggles Peach group2. Explicitly excluded from release-versus scope, as in accepted48. Ordinary common taunt remains. The upcoming generic debug-level query is sufficient; no Peach-native query is needed. |
| `ftCo_Throw.c:194` | This is the **thrower's** victim-animation policy, not a callback owned by the victim. Authored Bowser's current `on_throw_victim_motion` reads `capture.victim_animation_family` (Peach or Zelda), which uses copied animation-family metadata. An untouched stock Bowser fallback still compares raw victim kinds; custom-recipient handling through that stock route is an existing shared interoperability limit and is not silently claimed by this gate. |
| `ftdata.c` | Stock table registration. Independent generated Definition replaces every nonnull Peach lifecycle/special pointer; actual base metadata is retained. OnDeath2/damage/hitlag cleanup pointers are bound by authored producer/special helpers rather than inherited Definition members. |
| `ftcharacter_assets.c`, `native_fighter_article_bindings.c`, `native_fighter_runtime.c` | Import/clone source metadata. Owned five article rows and the copied animation family replace runtime donor-asset lookup. They remain normal import inputs. |
| `native_fighter_project.c`, `player.c` | Project/base name and roster registration, not live move decisions. |
| `native_semantic_state.c` | Diagnostic semantic projection, not behavior. Original comparison unions remain confined to source fixtures. |
| `ftkirby_copy_assets.c`, `ftKirby/*` | Explicit Kirby-copy exclusion. |

## Constructor and rendering ownership

Original constructors are `it_802BD158` (Explode), `it_802BD4AC` (Turnip),
`it_802BDA64` (Parasol), `it_802BDE18` (Toad) and `it_802BE214` (Spore).
The original/current defined decisions are already composed in the accepted full
candidate. Explode/Spore initialize on-created; Parasol/Toad perform ordinary
Spawn/Attach and initialize through the authored pickup table. Turnip owns its
random face, damage/life and later state phases. Rare ordinary item construction
is explicit `items.create_engine`, not an authored article falling back by kind.

`nativeArticle_Create`/child construction resolves logic and assets from immutable
author metadata, installs retained creator identity, then calls `on_created`.
Attachment/pickup occurs afterward through existing generic item operations.
Accepted source matrices isolate the allocator, HSD animation/model internals,
world probes, sound, debug/statistics and selected transition interiors. The new
gate does not claim those controls are actual rendered engine instances.

The original character GX table `it_3F2F.c:it_803F2F28` uses `it_8026EECC` for all
five Peach kinds. Explicit `render standard` selects that same shared callback
through the actual production `nativeItemBehavior_Select`. The renderer's
`it_266F` union access is conditional on the Zako-only overlay flag: Item creation
sets `xDCF_flag.b3=false`, and the source census finds its only true setter in
`itzako.c`. Ordinary Peach articles use the direct standard graph draw path;
this is source reasoning, not a rendered-output test.

No new constructor/parser/asset API gap is demonstrated in the supported57
release-versus composition. Remaining decisions are the optional debug cosmetic
policy; remaining validation is actual model construction/rendered gameplay and
mixed stock/custom thrower interoperability described above. No complete default
activation or game acceptance follows from this source-only checkpoint.
