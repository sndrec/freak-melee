#ifndef NATIVE_FIGHTER_SCRIPT_CODE_H
#define NATIVE_FIGHTER_SCRIPT_CODE_H

#include "native_fighter_script.h"
#include "native_fighter_script_state.h"
#include "native_fighter_script_types.h"
#include "native_fighter_script_article_refs.h"
#include "native_fighter_script_dependencies.h"

typedef struct ScriptFunctionProgram ScriptFunctionProgram;

typedef enum ScriptEntry {
    ScriptEntry_None,
    ScriptEntry_Stock,
    ScriptEntry_Authored,
} ScriptEntry;

typedef enum ScriptSpecial {
    ScriptSpecial_Unset,
    ScriptSpecial_Neutral,
    ScriptSpecial_Side,
    ScriptSpecial_Up,
    ScriptSpecial_Down,
} ScriptSpecial;

typedef struct ScriptScalar {
    uint16_t declared_offset_plus_one;
    uint16_t parameter_index_plus_one;
    bool declared_int;
    bool declared_bool;
    NativeFighterParameterType parameter_type;
    float number;
} ScriptScalar;

typedef struct ScriptCodeLine {
    char text[512];
    char generated[1024];
    uint8_t depth;
    bool opens;
} ScriptCodeLine;

enum { SCRIPT_CODE_LINES = 64, SCRIPT_CODE_LOCALS = 16 };

typedef struct ScriptCodeBlock {
    ScriptCodeLine lines[SCRIPT_CODE_LINES];
    char locals[SCRIPT_CODE_LOCALS][32];
    uint8_t local_depth[SCRIPT_CODE_LOCALS];
    ScriptExpressionType local_types[SCRIPT_CODE_LOCALS];
    unsigned count;
    unsigned local_count;
    uint8_t last_depth;
    bool last_opens;
    const ScriptStateTable* state;
    const NativeScriptArticleCatalog* articles;
    ScriptFunctionProgram* functions;
    ScriptFunctionSet function_calls;
    bool may_invalidate_source;
    bool model_effect;
    ScriptSpecial special;
    bool entry;
    bool special_program;
    bool air_entry;
    bool collision;
    bool item_event;
    bool predicate;
    bool integer_result;
    bool strict_integer_result;
    bool strict_boolean_result;
    bool cpu_prepare_event;
    bool floating_result;
    bool shield_transition_event;
    bool motion_event;
    bool victim_motion_event;
    bool held_motion_event;
    bool air_resume_event;
    bool neutral_enter_event;
    bool landing_action_event;
    bool item_swing_event;
    bool demo_event;
    bool action_metadata_event;
    bool scalar_event;
    bool smash_angle_event;
    bool target_geometry_event;
    bool cpu_projectile_defense_event;
    bool cpu_projectile_hit_event;
    bool other_fighter_event;
    bool dynamics_set_event;
    bool trail_event;
    bool grabbed_event;
    bool grab_catch_event;
    bool landing;
    bool landing_interrupt;
    ScriptScalar landing_lag;
    unsigned motion_state_first;
    unsigned motion_state_count;
} ScriptCodeBlock;

typedef struct ScriptMove {
    char name[64];
    ScriptSpecial special;
    unsigned special_line;
    float cue_speed;
    float cue_vertical_speed;
    bool ground;
    bool air;
    bool cue;
    ScriptCodeBlock ground_entry_code;
    ScriptCodeBlock air_entry_code;
    ScriptCodeBlock ground_animation;
    ScriptCodeBlock air_animation;
    ScriptCodeBlock ground_interrupt;
    ScriptCodeBlock air_interrupt;
    ScriptCodeBlock ground_physics;
    ScriptCodeBlock air_physics;
    ScriptCodeBlock ground_collision;
    ScriptCodeBlock air_collision;
    ScriptCodeBlock neutral_cue_code;
    ScriptCodeBlock accessory_code;
    bool cue_speed_set;
    bool cue_speed_replace;
    bool cue_vertical_speed_set;
    bool cue_vertical_speed_replace;
    bool stock_article;
    const char* fireball_bone;
    const char* effect_bone;
    unsigned effect_id;
    bool cue_once;
    bool explicit_entry;
    ScriptEntry ground_entry;
    ScriptEntry air_entry;
} ScriptMove;


bool nativeScriptCode_Contains(const ScriptCodeBlock* block,
                                const char* expression);
const char* nativeScriptCode_SpecialSuffix(ScriptSpecial special);
const char* nativeScriptCode_SpecialEnum(ScriptSpecial special);
const char* nativeScriptCode_SpecialField(ScriptSpecial special, bool air);
bool nativeScriptCode_Fail(NativeFighterScriptError* error, unsigned line,
                 const char* message);
bool nativeScriptCode_Identifier(const char* text, size_t size, size_t maximum);
bool nativeScriptCode_ValidKey(const char* key);
const char* nativeScriptCode_BoneSymbol(const char* name, size_t size);
bool nativeScriptCode_ParseScalar(const char* text, const ScriptStateTable* table,
                         ScriptScalar* scalar);
bool nativeScriptCode_AddAttribute(ScriptStateTable* table, const char* line);
void nativeScriptCode_EmitScalar(const ScriptScalar* scalar, FILE* output);
bool nativeScriptCode_AddLine(ScriptCodeBlock* block, const char* line);
void nativeScriptCode_EmitLines(const ScriptCodeBlock* block, FILE* output);
void nativeScriptCode_EmitHostChecks(const ScriptCodeBlock* block, FILE* output);
void nativeScriptCode_EmitBlock(const ScriptCodeBlock* block, bool air,
                            const char* phase, const char* suffix,
                            FILE* output);
void nativeScriptCode_EmitCollision(const ScriptMove* move, bool air, FILE* output);
bool nativeScriptCode_Expression(const ScriptCodeBlock* block,
                                 const char* source, char* output,
                                 size_t capacity);
ScriptExpressionType nativeScriptCode_ExpressionType(const ScriptCodeBlock*,
                                                      const char*);
bool nativeScriptCode_ParseArguments(const char* source, char arguments[][192],
                                     unsigned expected, const char** after);
const char* nativeScriptPrimitives_Property(const char* name);
bool nativeScriptPrimitives_Expression(const ScriptCodeBlock* block,
                                       const char* name, const char** source,
                                       char* output, size_t capacity);
bool nativeScriptPrimitives_Action(ScriptCodeBlock* block, const char* source,
                                    char* output, size_t capacity);
bool nativeScriptTerrain_Action(const ScriptCodeBlock* block, const char* source,
                                char* output, size_t capacity);
bool nativeScriptCpu_Action(const ScriptCodeBlock* block, const char* source,
                            char* output, size_t capacity);
const char* nativeScriptCpu_Property(const char* name);
int nativeScriptCpu_Expression(const ScriptCodeBlock*, const char*, const char**, char*, size_t);

#endif
