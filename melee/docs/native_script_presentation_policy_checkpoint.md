# Presentation and engine-policy integration batch

> Dated evidence, not the current backlog. Results and limitations below apply
> to the recorded build. See the
> [evidence index](checkpoint_index.md) before treating an old pending item as new work.

The multi-body roster audit grouped Jigglypuff and Mewtwo's missing capabilities
before implementation. This cohort adds four ordinary interpreted operations:

- `color.clear_attack()` uses the native attack overlay clear primitive;
  normal/special overlays and unrelated timers remain unchanged.
- `capture.armed` exposes the common capture eligibility flag. It does not
  configure collision masks/callbacks or unlink a captured participant.
- `animation.hip_root_compensation` exposes the common hip/root animation
  policy, with semantic rig requirements documented in the presentation guide.
- `damage.force_top_blast_ko` exposes the shared top-blast decision policy.

All require the exact live source-owned Fighter, program/storage and scene
lifetime. The color primitive checks retained visual dependencies before and
after mutation. No donor special callback, fighter-specific API or new syntax
was introduced. See [colors](scripting/colors.md), [capture](scripting/capture.md),
[presentation](scripting/presentation.md) and [jump policies](scripting/jump_policies.md).

The same cohort migrates templates to existing generic APIs: positional effect
payloads, explicit native motion-transition flags, callback clearing with `None`,
typed article release signals, Bool reflector policy, and owner model/motion/
parameter observations. Shadow Ball retains the native float maximum-charge
denominator rather than using its truncated Item state value. Installed content
and user drafts are unchanged.

## Evidence

- Common visual fixture: **18,184 checks and 2,265 native comparisons**, including
  20 new complete-Fighter comparisons against the actual `ftCo_800BFFAC` and
  `lb_80014498` statements. Invalid arguments/lifetimes reject; eleven injected
  post-native dependency mutations withhold output. Those return-boundary
  injections do not imply the native primitive normally calls scripts.
  `A:/FreakMelee-test-archives/script-clear-attack-color-oct10-verified/receipt.json`
  SHA-256 `85108b786251251123f903229b502878d4ee261296844bece5bc62f2cd761fb4`,
  1,605 independently rehashed inputs.
- Connected observations fixture: **17,723 checks, 1,443 cases and 2,849 native
  pairs**. New policy cases include 48 whole-Fighter assignment comparisons,
  two VM helper deliveries, 18 invalid-lifetime/context cases and six malformed
  direct writes. These exercise the real frontend, preparation, actor storage,
  interpreter and property routing; full capture/pose/KO algorithms are not
  claimed by flag-assignment comparisons.
  `A:/FreakMelee-test-archives/script-engine-policies-oct10-final/receipt.json`
  SHA-256 `32984c99271990856cb90cebc7fe0a27925ed5c008257075fcdd6e7cae96b85e`,
  1,723 independently rehashed inputs.
- Fresh full-roster preparation audit remains **18/26 forms**, excluding Kirby
  and counting Sheik/Nana separately. Jigglypuff's first-per-body diagnostics
  fall **23 -> 10**, Mewtwo's **11 -> 1**: **34 -> 11 combined** across their
  374 visited bodies. These are reported failing bodies, not an exhaustive
  count of every error or a completed-fighter count.
  `A:/FreakMelee-test-archives/source-full-roster-audit-oct10-presentation-final/receipt.json`
  SHA-256 `fe4638f5230bddf6889d71fdb20d11bcf01530657756207099b3c4322a1e42bd`,
  188 independently rehashed inputs.
- Seven Windows targets build; `native_fighter_script` reference CTest and
  diff check pass. Independent verification is recorded in
  `A:/FreakMelee-test-archives/presentation-policy-independent-pins-oct10.json`.

## Remaining grouped work

Jigglypuff still needs collision-box/ledge policy, multijump configuration and
drift, attachment-root management, and part-animation selection/visibility.
Selection-only `ftAnim_80070FB4` must not be replaced with the existing operation
that immediately applies part animation; their native behavior differs.
Mewtwo's remaining body needs an owned Item joint-path effect operation. A root
effect or guessed bone index would not preserve its child-joint semantics.

Fox/Falco and Pikachu/Pichu still stop at prerequisite owned-entity import or
metadata; Ness at animation resource metadata; Samus has no valid independent
draft in this audit. Later hook preparation can reveal separate scheduler gaps.
Continue the [grouped inventory](native_script_multi_error_audit.md), then full
fighter/paired gameplay acceptance and compact whole-match rollback. Goal
active. No game window, audio or installed-content update was needed.
