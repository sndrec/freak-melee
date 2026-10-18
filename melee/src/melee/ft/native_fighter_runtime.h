#ifndef MELEE_FT_NATIVE_FIGHTER_RUNTIME_H
#define MELEE_FT_NATIVE_FIGHTER_RUNTIME_H

#ifdef MELEE_NATIVE

#include <melee/ft/forward.h>
#include <melee/it/forward.h>
#include <melee/it/kinds/forward.h>
#include <melee/ef/forward.h>
#include <melee/lb/forward.h>

#include <stdbool.h>
#include <stdint.h>

#include <melee/gm/native_content_store.h>
#include "native_fighter_trail.h"
#include "ftcharacter_asset_layout.h"
#include "native_fighter_sound_policy.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NativeFighterBone {
    Fighter_Part part;
    uint8_t joint;
} NativeFighterBone;

typedef struct NativeFighterPartOmission {
    uint8_t joint;
    uint8_t parent;
    uint8_t insertion_mode; // Child/sibling insertion mode used by ftParts.
    uint8_t donor_joint; // Preorder index in the supplied descriptor; 0xFF uses root.
} NativeFighterPartOmission;

typedef struct NativeFighterArticleAsset {
    ItemKind kind;
    uint16_t slot;
    // -1 uses kind directly. Otherwise this is a decoded DAT integer slot.
    int16_t kind_attribute_offset;
} NativeFighterArticleAsset;

typedef enum NativeFighterShieldTransition {
    NativeFighterShield_Start,
    NativeFighterShield_Hold,
    NativeFighterShield_Release,
    NativeFighterShield_Hit,
    NativeFighterShield_ReflectContinue,
    NativeFighterShield_ReflectStart,
} NativeFighterShieldTransition;

typedef struct NativeFighterProjectile {
    int16_t appearance_state;
    uint16_t item_state;
    // Reads match-immutable speed, angle and lifetime from module parameters.
    void (*read_values)(const Article* source, float values[3]);
    bool auto_lifetime;
    bool initial_velocity;
    bool initial_update;
    HSD_GObjEvent on_launched;
} NativeFighterProjectile;

// Explicit world-space launch data, supplied at the moment of firing.
typedef struct NativeFighterProjectileLaunch {
    float angle;
    float speed;
    int32_t item_state;
    uint32_t hit_group;
    bool preserve_facing;
} NativeFighterProjectileLaunch;

enum { NATIVE_FIGHTER_ARTICLE_SIGNAL_LIMIT = 16 };
typedef struct NativeFighterArticleSignal {
    uint32_t name_hash;
    void (*callback)(Item_GObj*, float value);
} NativeFighterArticleSignal;

enum { NATIVE_FIGHTER_CALLBACK_EXPORT_LIMIT = 64 };
typedef enum NativeFighterReturnType {
    NativeFighterReturn_Void,
    NativeFighterReturn_Bool,
    NativeFighterReturn_Int,
    NativeFighterReturn_Float,
} NativeFighterReturnType;
typedef struct NativeFighterCallbackExport {
    uint32_t name_hash;
    union {
        HSD_GObjEvent callback;
        bool (*bool_callback)(HSD_GObj*);
        int32_t (*int_callback)(HSD_GObj*);
        float (*float_callback)(HSD_GObj*);
    };
    NativeFighterReturnType result_type;
} NativeFighterCallbackExport;

enum { NATIVE_FIGHTER_MODEL_EFFECT_LIMIT = 32 };
typedef struct NativeFighterModelEffect {
    uint32_t name_hash;
    int32_t gfx_id;
    EF_UpdateFn update;
} NativeFighterModelEffect;

enum { NATIVE_FIGHTER_POSE_GRAPH_LIMIT = 16 };
typedef struct NativeFighterPoseGraph {
    uint32_t name_hash;
    uint16_t slot;
    const char* path;
} NativeFighterPoseGraph;

typedef struct NativeFighterAttachmentSpawn {
    bool origin_at_owner;
    bool origin_at_center;
    bool previous_at_bone;
    bool flatten_z;
    bool initial_collision;
    bool register_as_held;
} NativeFighterAttachmentSpawn;

enum { NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT = 8 };
typedef struct NativeFighterArticleModelRoot {
    uint32_t name_hash;
    uint32_t attribute_offset;
} NativeFighterArticleModelRoot;

enum { NATIVE_FIGHTER_ARTICLE_MODEL_ANIMATION_LIMIT = 8 };
typedef struct NativeFighterArticleAnimationComponent {
    // Offset of a serialized pointer word in the article attributes. UINT32_MAX
    // omits the component. array_index == UINT32_MAX selects a direct graph;
    // otherwise the word references an array of graph pointers.
    uint32_t attribute_offset;
    uint32_t array_index;
} NativeFighterArticleAnimationComponent;

typedef struct NativeFighterArticleModelAnimation {
    uint32_t name_hash;
    NativeFighterArticleAnimationComponent components[3]; // Joint/material/shape.
} NativeFighterArticleModelAnimation;

typedef enum NativeFighterStateType {
    NativeFighterState_Bool,
    NativeFighterState_Float,
    NativeFighterState_Int,
} NativeFighterStateType;

// Immutable declarations describe the fixed per-instance storage. Arrays use
// one declaration rather than allocating a descriptor for every element.
typedef struct NativeFighterStateField {
    const char* name;
    uint16_t offset;
    uint16_t count;
    uint8_t type;
    uint8_t stride;
    bool array;
} NativeFighterStateField;

typedef struct NativeFighterArticleLogic {
    ItemKind kind;
    ItemLogicTable* logic;
    const NativeFighterProjectile* projectile;
    uint16_t state_count;
    uint16_t local_state_bytes;
    uint16_t state_field_count;
    const NativeFighterStateField* state_fields;
    uint16_t signal_count;
    const NativeFighterArticleSignal* signals;
    const NativeFighterAttachmentSpawn* attachment;
    bool standard_render;
    HSD_GObjEvent on_created;
    // Replaces stock-kind policy at an out-of-bounds collision notification.
    HSD_GObjEvent on_out_of_bounds;
    uint16_t model_root_count;
    const NativeFighterArticleModelRoot* model_roots;
    uint16_t model_animation_count;
    const NativeFighterArticleModelAnimation* model_animations;
    // Immutable collection limits. Capacity zero disables resources. A part
    // reservation of -1 publishes no external joint; otherwise it must lie
    // beyond this fighter's actual rig within MAX_FT_PARTS.
    uint16_t resource_capacity;
    int16_t resource_reserved_part;
    // Actual held-item policy. Numeric statuses/actions are authored data.
    int32_t (*on_held_status)(Item_GObj*);
    bool (*on_held_request)(Item_GObj*, int32_t action, float frame_rate, float duration);
    // Notification owned by the actual held article, independent of its creator.
    HSD_GObjEvent on_holder_neutral;
} NativeFighterArticleLogic;

typedef struct NativeFighterCostumeInfo {
    char key[NATIVE_FIGHTER_CODE_KEY_BYTES];
    char joint_name[128];
    char matanim_name[128];
    uint16_t joint_count;
} NativeFighterCostumeInfo;

typedef struct NativeFighterCostumeFamilyInfo {
    uint8_t member_count;
    NativeFighterCostumeInfo members[NATIVE_FIGHTER_FAMILY_MEMBERS];
} NativeFighterCostumeFamilyInfo;

typedef enum NativeFighterSpecial {
    NativeFighterSpecial_Neutral,
    NativeFighterSpecial_Side,
    NativeFighterSpecial_Up,
    NativeFighterSpecial_Down,
    NativeFighterSpecial_Count,
} NativeFighterSpecial;

// A fighter module owns immutable definitions. Mutable state belongs to each
// Item, so rollback can restore it without copying module memory.
typedef struct NativeItemDefinition {
    const char* name;
    uint16_t damage;
    uint16_t angle;
    uint16_t lifetime_frames;
    HSD_GObjEvent on_thrown;
    HSD_GObjEvent on_tick;
    HSD_GObjPredicate on_ground_collision;
    HSD_GObjPredicate on_hit;
    // Encoded package-owned PNG, decoded once when this fighter is activated.
    const uint8_t* billboard_png;
    uint32_t billboard_png_size;
    float billboard_size;
    float hitbox_size;
    uint16_t knockback_growth;
    uint16_t base_knockback;
} NativeItemDefinition;

typedef struct NativeFighterDefinition {
    const char* code_key;
    // Stock callback/part family inherited by an untouched clone.
    // Ft_Kind_None means all behavior and bone mapping are supplied here.
    FighterKind source_kind;
    const char* fighter_root;
    const char* costume_joint;
    const char* costume_matanim;
    uint16_t action_count;
    uint16_t demo_count;
    uint16_t motion_state_count;
    MotionState* motion_states;
    FtMotionId motion_state_first;
    // Named special states available to authored behavior. Zero means this
    // base has not exposed the corresponding state for script entry.
    FtMotionId special_ground_state[NativeFighterSpecial_Count];
    FtMotionId special_air_state[NativeFighterSpecial_Count];
    MotionFlags special_transition_flags[NativeFighterSpecial_Count];
    uint16_t joint_count;
    uint16_t state_bytes;
    uint16_t state_field_count;
    const NativeFighterStateField* state_fields;
    uint16_t callback_export_count;
    const NativeFighterCallbackExport* callback_exports;
    uint16_t bone_count;
    const NativeFighterBone* bones;
    HSD_GObjEvent on_load;
    HSD_GObjEvent on_load_special_attrs;
    // Destination-owned completion after the common alternate-form transfer.
    // Each dormant form retains its own authored state between activations.
    HSD_GObjEvent on_form_activated;
    // Runs once per simulated fighter frame, including rollback resimulation.
    HSD_GObjEvent on_tick;
    // Preserve the donor table's ordering inside the fighter update and
    // color-animation paths. These are separate from the early on_tick hook.
    HSD_GObjEvent on_late_update;
    HSD_GObjEvent on_color_animation_reset;
    HSD_GObjEvent on_death;
    HSD_GObjEvent on_landing;
    // Runs after entering a common landing action, distinct from the earlier
    // air-to-ground transition (which can also keep a special move active).
    HSD_GObjEvent on_landing_action;
    // Fighter-owned throw animation events, before common victim/release logic.
    HSD_GObjEvent on_throw_animation;
    // Return true after replacing the common throw's animation-end transition.
    HSD_GObjPredicate on_throw_finish;
    // Select the victim motion before the shared throw initializes its pose.
    int (*on_throw_victim_motion)(Fighter_GObj*, int motion, int victim_motion);
    // Return true after owning the end transition of a common heavy-item pickup.
    HSD_GObjPredicate on_heavy_pickup_finish;
    // Select an owned motion for the fourth held-item swing variant. Negative
    // means unsupported; the common entry retains its existing error boundary.
    int (*on_item_swing_motion)(Fighter_GObj*, int swing_type, int variant);
    // After common heavy-throw setup; can replace its movement callbacks.
    HSD_GObjEvent on_heavy_throw_enter;
    // After common forward/back roll entry; can replace animation/collision.
    HSD_GObjEvent on_roll_enter;
    // Own the damage reaction while holding a heavy item.
    HSD_GObjEvent on_heavy_item_damage;
    // Shared CPU planner policy points; declared hooks replace kind decisions.
    HSD_GObjPredicate on_cpu_pending_action;
    HSD_GObjEvent on_cpu_initialize;
    // True replaces mode update/final controller overlay; absence retains stock.
    HSD_GObjPredicate on_cpu_mode_update;
    HSD_GObjPredicate on_cpu_output;
    // Before mode initialization; return a live history source ID, or zero.
    int32_t (*on_cpu_prepare)(Fighter_GObj*, uint32_t history_hint);
    // After neutral/squat priority; true replaces the remaining drop commands.
    HSD_GObjPredicate on_cpu_drop_command;
    // Return 0..127; invalid results retain the corresponding common default.
    int32_t (*on_cpu_stick_step)(Fighter_GObj*);
    int32_t (*on_cpu_stick_limit)(Fighter_GObj*);
    void (*on_cpu_spacing_update)(Fighter_GObj*, float sample);
    HSD_GObjPredicate on_cpu_attack_motion;
    HSD_GObjPredicate on_cpu_grab_motion;
    HSD_GObjPredicate on_cpu_attack_decision;
    HSD_GObjPredicate on_cpu_capture_decision;
    // Select B instead of A for the shared grab planner's pummel command.
    HSD_GObjPredicate on_cpu_pummel_special;
    // Append fighter-owned ranged attack candidates after the common candidate.
    HSD_GObjEvent on_cpu_ranged_candidates;
    // Additional eligibility at the shared ranged planner's policy gate.
    HSD_GObjPredicate on_cpu_ranged_allowed;
    // Select a command index from target geometry; zero continues common logic.
    // Return true to consume this decision, even if no command is changed.
    bool (*on_cpu_special_candidate)(Fighter_GObj*, float angle, float distance);
    // Classify the first common predicted projectile contact: 0 ignores it.
    int (*on_cpu_projectile_hit)(Fighter_GObj*, int prediction_frames, bool unblockable);
    // Called on the potential attacker, with the observing defender explicitly.
    bool (*on_cpu_threat)(Fighter_GObj*, Fighter_GObj* observer);
    HSD_GObjPredicate on_cpu_charge_decision;
    HSD_GObjEvent on_cpu_charge_cancel;
    // A handled travel follow-up owns commands and travel-count changes.
    HSD_GObjPredicate on_cpu_travel_decision;
    // Filters only the size-dependent gate within ordinary falling recovery.
    HSD_GObjPredicate on_cpu_falling_recovery_allowed;
    HSD_GObjPredicate on_cpu_falling_destination_reached;
    // Queues an alternative available jump after the forced-forward branch.
    HSD_GObjPredicate on_cpu_available_jump;
    HSD_GObjEvent on_taunt;
    // Shared demo state is entered first; authored policy owns its presentation.
    void (*on_demo_enter)(HSD_GObj*, int mode);
    // After common death/sleep motion and flags; own held-item presentation.
    HSD_GObjEvent on_presentation_item;
    HSD_GObjPredicate on_wait_input;
    // After common entry/held-animation setup; own optional final setup.
    HSD_GObjEvent on_wait_enter;
    HSD_GObjEvent on_crouch_wait_enter;
    HSD_GObjEvent on_aerial_attack_enter;
    HSD_GObjPredicate on_aerial_action_manual;
    HSD_GObjPredicate on_aerial_action_automatic;
    // At grounded motion setup, before common grounded flags are reset.
    HSD_GObjEvent on_grounded_motion_setup;
    HSD_GObjPredicate on_action_status;
    int32_t (*on_action_metadata)(Fighter_GObj*, int32_t metadata);
    HSD_GObjPredicate on_idle_variants;
    bool (*on_motion_is_angled)(HSD_GObj*, int motion);
    // Whether a requested second/third jab should restart the ordinary first jab.
    bool (*on_jab_restart)(HSD_GObj*, int motion);
    HSD_GObjEvent on_shield_start;
    HSD_GObjEvent on_shield_reflect_start;
    // Opt dynamic bones into the shared wind override, independent of fighter kind.
    HSD_GObjPredicate on_dynamics_wind;
    // Enable floor constraints for dynamic bones independently of fighter kind.
    HSD_GObjPredicate on_dynamics_ground_collision;
    // First chain element affected by interaction forces, per dynamic set.
    int (*on_dynamics_interaction_start)(Fighter_GObj*, int set, int set_count);
    // Own aerial-jump entry after shared special-jump interception.
    HSD_GObjEvent on_aerial_jump;
    // Worn-item sound policy for shared item-induced and multi-jump entries.
    HSD_GObjPredicate on_worn_item_jump_sound;
    // Whether a contacting body attack may enter its ordinary rebound state.
    HSD_GObjPredicate on_body_rebound;
    // Describe this fighter's weapon ribbon, independent of donor kind/data.
    // Called only while the subaction-controlled trail is active; no ticking
    // or RNG consumption belongs in this rendering data callback.
    void (*on_trail)(Fighter_GObj*, NativeFighterTrail*);
    HSD_GObjEvent on_death1;
    HSD_GObjEvent on_user_data_remove;
    HSD_GObjEvent on_absorb;
    Fighter_ItemEvent on_item_pickup_ext;
    Fighter_ItemEvent on_item_drop_ext;
    Fighter_ItemEvent on_item_pickup;
    Fighter_ItemEvent on_item_drop;
    HSD_GObjEvent on_item_invisible;
    HSD_GObjEvent on_item_visible;
    HSD_GObjEvent on_knockback_enter;
    HSD_GObjEvent on_knockback_exit;
    HSD_GObjEvent special_n;
    HSD_GObjEvent special_air_n;
    // Optional original neutral-special article operation for readable stock
    // templates. An authored package may call it at its own action cue.
    HSD_GObjEvent neutral_article;
    HSD_GObjEvent special_s;
    HSD_GObjEvent special_air_s;
    HSD_GObjEvent special_hi;
    HSD_GObjEvent special_air_hi;
    HSD_GObjEvent special_lw;
    HSD_GObjEvent special_air_lw;
    // Source DAT tag identifying clips authored for this costume skeleton.
    // Independent of stock behavior inheritance (e.g. a custom rig may still
    // carry its original donor's tag). Other tags retain cross-rig retargeting.
    FighterKind animation_source_kind;
    const NativeItemDefinition* items;
    uint16_t item_count;
    // Replaces a spawned article's callbacks while retaining its own DAT data.
    const NativeFighterArticleLogic* article_logics;
    uint16_t article_logic_count;
    // Script-created costume graphs replace inherited model callbacks.
    bool owns_model_attachments;
    // Immutable dependencies on ISO sound banks, selected during scene loading.
    // Independent of the original fighter's behavior and animation families.
    uint64_t sound_banks;
    // Bits 0..6 select the sound channels suppressed by x2225_b6.
    // Ordinary stock defaults mute channels 2/6; authored policy owns this mask.
    uint8_t sound_mute_channels;
    // Optional immutable costume-dependent sound-ID routing, owned by package.
    const NativeFighterSoundPolicy* sound_policy;
    // Common ground acceleration/friction ignores the stage surface multiplier.
    bool ignore_surface_friction;
    // Common jab entry requests transition flags before changing motion.
    // An authored callback may bind a one-shot state-change helper here.
    int (*on_jab_start)(Fighter_GObj*);
    HSD_GObjEvent on_forward_smash_start;
    // Replace complete fighter-specific entry after common input/item priority.
    void (*on_forward_smash_enter)(Fighter_GObj*, float angle);
    HSD_GObjEvent on_up_smash_enter;
    HSD_GObjEvent on_down_smash_enter;
    HSD_GObjEvent on_auxiliary_item_attach;
    HSD_GObjEvent on_auxiliary_item_visibility_reset;
    // Supplies controller commands at the planner's up-special recovery point.
    HSD_GObjEvent on_cpu_recovery;
    // Own a complete shield transition before ordinary bubble setup.
    void (*on_shield_transition)(Fighter_GObj*, int mode, bool special_hit);
    // Query the shield joint/collision radius and effect-alpha policy.
    float (*on_shield_scale)(Fighter_GObj*);
    HSD_GObjPredicate on_shield_alpha;
    // Before ordinary neutral escape setup; common input/entry stays shared.
    HSD_GObjEvent on_spotdodge_enter;
    // Capture policies belong to the holder, with the caught fighter explicit.
    void (*on_capture_wait_setup)(Fighter_GObj* holder, Fighter_GObj* caught);
    void (*on_capture_wait_enter)(Fighter_GObj* holder, Fighter_GObj* caught);
    float (*on_catch_pull_frame)(Fighter_GObj* holder);
    // Grounded grab policy; ordinary input, motion and capture machinery stay shared.
    HSD_GObjPredicate on_catch_allowed;
    HSD_GObjEvent on_catch_start;
    HSD_GObjPredicate on_catch_frame;
    HSD_GObjEvent on_catch_exit;
    HSD_GObjEvent on_catch_wait_release;
    HSD_GObjPredicate on_catch_pull_anchor;
    HSD_GObjPredicate on_catch_pull_ready;
    HSD_GObjPredicate on_catch_pull_follow;
    HSD_GObjEvent on_throw_victim_setup;
    // Replaces the complete optional air-tether input predicate at its common
    // interrupt priority. False declines this input without stock fallback.
    HSD_GObjPredicate on_air_tether_input;
    // Immutable asset facts; no stock gameplay callbacks are selected here.
    FtCharacterAssetLayout asset_layout;
    uint16_t article_asset_count;
    const NativeFighterArticleAsset* article_assets;
    uint16_t omission_count;
    const NativeFighterPartOmission* omissions;
    uint8_t effect_bank; // 0xFF means no character-specific effect archive.
    // Archive roots for copied result/intro/ending/wait clips. These remain
    // available after a script replaces the complete stock callback family.
    const Fighter_DemoStrings* demo_motion_names;
    uint16_t model_effect_count;
    const NativeFighterModelEffect* model_effects;
    uint16_t pose_graph_count;
    const NativeFighterPoseGraph* pose_graphs;
    // After common shield-break launch setup, before effects/sound/hit status.
    // Owns the optional force-top-blast eligibility decision for this entry.
    HSD_GObjEvent on_shield_break;
    // Post shared projectile-defense gates. True consumes the response; false
    // continues common defense without a stock fighter-kind decision.
    bool (*on_cpu_projectile_defense)(Fighter_GObj*, int category);
    // Map a common held-item transition to an owned motion before publication.
    // The previous motion is captured before the common entry changes state.
    int32_t (*on_held_motion)(Fighter_GObj*, int32_t common_motion,
                              int32_t previous_motion);
    // After down/hammer/air-to-ground gates, before publishing ordinary Wait.
    void (*on_neutral_enter)(Fighter_GObj*, float blend);
    // Map the motion resumed after a common temporary airborne transformation.
    int32_t (*on_air_resume_motion)(Fighter_GObj*, int32_t previous_motion);
    // Called after a held-action timer reaches zero. True owns the transition.
    HSD_GObjPredicate on_held_timer_expired;
    // Replace full normal-attack entry after common input/item priority.
    HSD_GObjEvent on_jab_enter;
    HSD_GObjEvent on_rapid_jab_enter;
    HSD_GObjEvent on_down_tilt_enter;
    // Remap a selected common aerial before common startup, without changing
    // its input priority. Only the selected common or an owned motion is valid.
    int32_t (*on_aerial_attack_motion)(Fighter_GObj*, int32_t selected_motion);
} NativeFighterDefinition;

// Definitions compiled into this client for packages without a native module.
const NativeFighterDefinition* nativeFighterRuntime_FindDefinition(
    const char* code_key);
// Only stock fighters with an independent package definition are returned.
const NativeFighterDefinition* nativeFighterRuntime_FindStockDefinition(
    FighterKind source_kind);

// Returns the installed package title for an active match character.
const char* nativeFighterRuntime_Title(CharacterKind character_kind);

// Callback-bearing native definitions are absent for ordinary source packages.
const NativeFighterDefinition* nativeFighterRuntime_Definition(
    FighterKind fighter_kind);
struct NativeSourcePackage;
// The actual selected source candidate, with owned data-only facts and arenas.
// Native Definition is absent for these actors; no DLL-shaped substitute.
const struct NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind);
// Both kinds must be distinct members of the same active alternate family.
// Player role permutation is separate from this immutable membership query.
bool nativeFighterRuntime_AlternatePair(FighterKind first,FighterKind second);
// Pure owned channel policy; raw stock fighters retain their original defaults.
uint8_t nativeFighterRuntime_SoundMuteChannels(FighterKind fighter_kind);

// Install a validated owned DObj lookup. Failure preserves visibility state.
bool nativeFighterRuntime_InstallOutlineVisibility(Fighter_GObj*, int32_t slot);
// Resolve immutable member identity. Only Single source families are supported.
uint32_t nativeFighterRuntime_MemberId(Fighter_GObj*, unsigned member_index);
// Validated published counterpart; constructing source need not occupy a role.
uint32_t nativeFighterRuntime_CpuHistorySource(Fighter_GObj*);
uint64_t nativeFighterRuntime_CharacterSoundBanks(
    CharacterKind character_kind);

// Called when creating a fighter, after custom definitions are registered.
FighterKind nativeFighterRuntime_BehaviorKind(FighterKind kind);
// Animation compatibility tag owned by the active costume/animation definition.
FighterKind nativeFighterRuntime_AnimationFamily(Fighter* fighter);

// Returns true when this item kind belongs to a custom fighter. In that case,
// article is the fighter's asset, even if it is missing; the caller must not
// silently use the stock fighter's global article instead.
bool nativeFighterRuntime_ResolveArticleForItem(const Fighter* owner,
                                                ItemKind item_kind,
                                                Article** article);

// Returns the module-owned article definition for an article spawned by this
// custom fighter, or NULL when its stock item logic remains in use.
const NativeFighterArticleLogic* nativeFighterRuntime_ArticleLogic(
    const Fighter* owner, ItemKind item_kind);
// Resolve package assets by immutable author identity, independent of the
// item's current damage owner (which can change through reflection).
Article* nativeFighterRuntime_ArticleAssets(FighterKind author, ItemKind kind);

// Quiescent scene transition only, after actors, exact leases and retained
// rollback page history have retired. False preserves every accepted table.
bool nativeFighterRuntime_CanClear(void);
bool nativeFighterRuntime_Clear(void);

// Validate a cached fighter without replacing any registered fighter or
// scene-owned object. Consumes owned facts/FMS/DATs; no donor callback fallback.
bool nativeFighterRuntime_ValidateCached(
    const NativeContentId* id, const char* root);

// Read a cached fighter's costume graph contract without changing the active
// match registration. The returned names are copied out of the owned facts.
bool nativeFighterRuntime_CostumeInfoCached(
    const NativeContentId* id, const char* root,
    NativeFighterCostumeFamilyInfo* info);

// Synchronous borrowed views of the currently registered package and selected
// costume. Native pose validation never retains these across a callback.
typedef struct NativeFighterPoseAssets {
    const struct FtCharacterAssets* character;
    const struct FtCostumeAssets* costume;
    uint32_t action_count,demo_count,pose_graph_count;
    const uint8_t* animation_bytes;
    size_t animation_size;
    const void* animation_arena;
    size_t animation_arena_size;
} NativeFighterPoseAssets;
bool nativeFighterRuntime_PoseAssets(Fighter_GObj*, NativeFighterPoseAssets*);
HSD_Joint* nativeFighterRuntime_PoseGraph(Fighter_GObj*, int32_t index);
FigaTree* nativeFighterRuntime_PoseClip(Fighter_GObj*, int32_t index);

// Prepare all selected packages while the old scene and callbacks are still
// intact. No live fighter table changes until ActivateStaged at scene startup.
bool nativeFighterRuntime_StageCached(const NativeContentId* ids,
                                      unsigned count, const char* root,
                                      bool require_installed);
bool nativeFighterRuntime_ActivateStaged(unsigned count);

// Import and bind a Single source package. Preflight validates without
// publication. Final activation requires actual common data and no retained
// history; the caller assigns the same kind on every peer.
bool nativeFighterRuntime_Load(const NativeContentId* id,
                               CharacterKind character_kind,
                               FighterKind fighter_kind, const char* root,
                               bool require_installed, bool preflight);

#ifdef __cplusplus
}
#endif

#endif
#endif
