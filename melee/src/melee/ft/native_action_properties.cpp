#include "native_action_properties.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "ftaction_codec.h"
#include "native_fighter_action_labels.h"
#include "native_hitbox_lifetime.h"

namespace NativeActionProperties {
    namespace {
        using NativeAuthoring::add_string;
        using NativeAuthoring::Reply;
        void named(yyjson_mut_doc* doc, yyjson_mut_val* object,
                   const std::string& key, yyjson_mut_val* value)
        {
            yyjson_mut_obj_add(object, yyjson_mut_strcpy(doc, key.c_str()),
                               value);
        }
        enum Storage {
            U8,
            I8,
            U16,
            I16,
            U32,
            F32,
            Boolean,
            Flag
        };
        struct Field {
            const char* name;
            size_t offset;
            Storage storage;
            double minimum, maximum, scale;
            bool read_only;
            const char* description;
            unsigned launch_mode = NativeHitboxLaunch_Default;
        };
#define F(T, M, S, L, H, Q, R, D) { #M, offsetof(T, M), S, L, H, Q, R, D }
        const Field hitbox_fields[] = {
            { "duration", offsetof(NativeFighterHitbox, duration256), U32, 1,
              256000, 1. / 256., false,
              "How many animation frames this hitbox remains active." },
            F(NativeFighterHitbox, timed, Boolean, 0, 1, 1, true,
              "This hitbox owns its duration."),
            F(NativeFighterHitbox, id, U8, 0, 3, 1, true,
              "The attack hitbox slot. Choose it when creating the hitbox."),
            F(NativeFighterHitbox, refresh_hit_eligibility, Boolean, 0, 1, 1,
              false,
              "Allows this hitbox to hit opponents again when it replaces the "
              "previous hitbox."),
            F(NativeFighterHitbox, expire_all, Boolean, 0, 1, 1, false,
              "Clears all attack hitbox slots when this duration expires."),
            F(NativeFighterHitbox, hit_group, U8, 0, 7, 1, false,
              "Hitboxes in the same group share their hit opponents."),
            F(NativeFighterHitbox, only_hit_grabbed, Flag, 0, 1, 1, false,
              "Hits only the fighter held by this fighter."),
            F(NativeFighterHitbox, exclude_grabbed, Flag, 0, 1, 1, false,
              "Lets the hitbox pass through the fighter you are holding."),
            F(NativeFighterHitbox, bone, U8, 0, 255, 1, false,
              "Bone to which the hitbox is attached."),
            F(NativeFighterHitbox, use_common_bone_ids, Flag, 0, 1, 1, false,
              "Interpret Bone as an engine bone role instead of a skeleton "
              "joint index."),
            F(NativeFighterHitbox, damage, F32, 0, 1023, 1, false,
              "Damage added by the hit."),
            F(NativeFighterHitbox, radius, U16, 0, 65535, FT_ACTION_HITBOX_DISTANCE_SCALE, false,
              "Hitbox radius in game units."),
            F(NativeFighterHitbox, offset_x, I16, -32768, 32767, FT_ACTION_HITBOX_DISTANCE_SCALE,
              false, "Horizontal attachment offset in game units."),
            F(NativeFighterHitbox, offset_y, I16, -32768, 32767, FT_ACTION_HITBOX_DISTANCE_SCALE,
              false, "Vertical attachment offset in game units."),
            F(NativeFighterHitbox, offset_z, I16, -32768, 32767, FT_ACTION_HITBOX_DISTANCE_SCALE,
              false, "Depth attachment offset in game units."),
            F(NativeFighterHitbox, capsule, Boolean, 0, 1, 1,
              false, "Add a second endpoint to make this hitbox a capsule."),
            F(NativeFighterHitbox, end_x, I16, -32768, 32767, FT_ACTION_HITBOX_DISTANCE_SCALE,
              false, "Horizontal offset of the second endpoint in game units."),
            F(NativeFighterHitbox, end_y, I16, -32768, 32767, FT_ACTION_HITBOX_DISTANCE_SCALE,
              false, "Vertical offset of the second endpoint in game units."),
            F(NativeFighterHitbox, end_z, I16, -32768, 32767, FT_ACTION_HITBOX_DISTANCE_SCALE,
              false, "Depth offset of the second endpoint in game units."),
            F(NativeFighterHitbox, angle, U16, 0, 511, 1, false,
              "Knockback angle in degrees. 361 selects the Sakurai angle."),
            { "launch_mode", offsetof(NativeFighterHitbox, launch.mode), U32, 0, 2, 1,
              false, "Choose an angle, a point relative to the attacker, or a launch that follows the hitbox." },
            { "launch_forward", offsetof(NativeFighterHitbox, launch.target.forward), F32,
              -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1, false,
              "Target distance forward from the attacker, in world units.", NativeHitboxLaunch_Target },
            { "launch_up", offsetof(NativeFighterHitbox, launch.target.up), F32,
              -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1, false,
              "Target height above the attacker, in world units.", NativeHitboxLaunch_Target },
            { "launch_frames", offsetof(NativeFighterHitbox, launch.target.frames), F32,
              1, std::numeric_limits<float>::max(), 1, false,
              "Divides the distance to the target to set the launch speed.", NativeHitboxLaunch_Target },
            { "launch_momentum", offsetof(NativeFighterHitbox, launch.follow.momentum), F32,
              -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1, false,
              "Multiplier on the attacker's velocity for airborne opponents.", NativeHitboxLaunch_Follow },
            { "launch_attraction", offsetof(NativeFighterHitbox, launch.follow.attraction), F32,
              0, std::numeric_limits<float>::max(), 1, false,
              "Pull toward the hitbox center per world unit of separation.", NativeHitboxLaunch_Follow },
            { "launch_min_up", offsetof(NativeFighterHitbox, launch.follow.min_up), F32,
              -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 1, false,
              "Minimum vertical launch speed for airborne opponents.", NativeHitboxLaunch_Follow },
            { "launch_ground_angle", offsetof(NativeFighterHitbox, launch.follow.ground_angle), F32,
              0, 359.999969482421875, 1, false,
              "Angle in degrees for grounded opponents.", NativeHitboxLaunch_Follow },
            { "launch_override_ground_knockback", offsetof(NativeFighterHitbox, launch.follow.override_ground_knockback), Boolean,
              0, 1, 1, false, "Use separate growth and base knockback for grounded opponents.", NativeHitboxLaunch_Follow },
            { "launch_ground_growth", offsetof(NativeFighterHitbox, launch.follow.ground_growth), U16,
              0, 511, 1, false, "Knockback growth for grounded opponents.", NativeHitboxLaunch_Follow },
            { "launch_ground_base", offsetof(NativeFighterHitbox, launch.follow.ground_base), U16,
              0, 511, 1, false, "Base knockback for grounded opponents.", NativeHitboxLaunch_Follow },
            F(NativeFighterHitbox, knockback_growth, U16, 0, 511, 1, false,
              "How strongly knockback grows with opponent damage."),
            F(NativeFighterHitbox, weight_set_knockback, U16, 0, 511, 1, false,
              "Weight-based set knockback."),
            F(NativeFighterHitbox, item_hit_interaction, Flag, 0, 1, 1, false,
              "Enables the attack's item-hit interaction flag."),
            F(NativeFighterHitbox, ignore_thrown_fighters, Flag, 0, 1, 1,
              false, "Skips fighters launched by a throw."),
            F(NativeFighterHitbox, ignore_fighter_scale, Flag, 0, 1, 1, false,
              "Keeps the hitbox size independent of fighter scale."),
            F(NativeFighterHitbox, clank, Flag, 0, 1, 1, false,
              "Allows this hitbox to clash with another attack."),
            F(NativeFighterHitbox, rebound, Flag, 0, 1, 1, false,
              "Allows attack rebound on a clash."),
            F(NativeFighterHitbox, base_knockback, U16, 0, 511, 1, false,
              "Knockback added regardless of opponent damage."),
            F(NativeFighterHitbox, element, U8, 0, 31, 1, false,
              "Hit effect, such as fire, electric or sleep."),
            F(NativeFighterHitbox, shield_damage, I8, -128, 127, 1, false,
              "Additional shield damage."),
            F(NativeFighterHitbox, hit_sfx_severity, U8, 0, 7, 1, false,
              "Impact sound strength."),
            F(NativeFighterHitbox, hit_sfx_kind, U8, 0, 31, 1, false,
              "Impact sound kind."),
            F(NativeFighterHitbox, hit_grounded, Flag, 0, 1, 1, false,
              "Hits grounded fighters."),
            F(NativeFighterHitbox, hit_aerial, Flag, 0, 1, 1, false,
              "Hits airborne fighters."),
        };
        const Field sound_fields[] = {
            F(NativeFighterSoundEffect, behavior, U8, 0, 6, 1, false,
              "Fighter sound channel. Use 0 for one-shot sounds."),
            F(NativeFighterSoundEffect, package_clip, Boolean, 0, 1, 1, false,
              "Play a clip from this fighter's sound bank."),
            F(NativeFighterSoundEffect, sfx_id, U32, 0, 4294967295., 1, false,
              "Melee sound ID or the zero-based clip index in sounds.ssm."),
            F(NativeFighterSoundEffect, volume, U8, 0, 255, 1, false,
              "Playback volume."),
            F(NativeFighterSoundEffect, panning, U8, 0, 255, 1, false,
              "Stereo panning."),
        };
        const Field effect_fields[] = {
            F(NativeFighterSpawnEffect, bone, U8, 0, 255, 1, false,
              "Attachment bone."),
            F(NativeFighterSpawnEffect, use_common_bone_ids, Flag, 0, 1, 1,
              false, "Use an engine bone role for the attachment."),
            F(NativeFighterSpawnEffect, destroy_on_state_change, Flag, 0, 1, 1,
              false, "Removes the effect when the action changes."),
            F(NativeFighterSpawnEffect, use_unknown_bone, Flag, 0, 1, 1, false,
              "Enables the effect's alternate bone-selection flag."),
            F(NativeFighterSpawnEffect, gfx_id, U16, 0, 65535, 1, false,
              "Visual effect to spawn."),
            F(NativeFighterSpawnEffect, offset_x, I16, -32768, 32767, 1, false,
              "Effect X offset in command units (1/256 game unit)."),
            F(NativeFighterSpawnEffect, offset_y, I16, -32768, 32767, 1, false,
              "Effect Y offset in command units (1/256 game unit)."),
            F(NativeFighterSpawnEffect, offset_z, I16, -32768, 32767, 1, false,
              "Effect Z offset in command units (1/256 game unit)."),
            F(NativeFighterSpawnEffect, range_x, U16, 0, 65535, 1, false,
              "Effect X range in command units."),
            F(NativeFighterSpawnEffect, range_y, U16, 0, 65535, 1, false,
              "Effect Y range in command units."),
            F(NativeFighterSpawnEffect, range_z, U16, 0, 65535, 1, false,
              "Effect Z range in command units."),
        };
#undef F
        double number(const void* object, const Field& field)
        {
            const auto* p = (const uint8_t*) object + field.offset;
            switch (field.storage) {
            case U8:
                return *p;
            case I8:
                return *(const int8_t*) p;
            case U16: {
                uint16_t v;
                std::memcpy(&v, p, 2);
                return v;
            }
            case I16: {
                int16_t v;
                std::memcpy(&v, p, 2);
                return v;
            }
            case U32: {
                uint32_t v;
                std::memcpy(&v, p, 4);
                return v;
            }
            case F32: {
                float v;
                std::memcpy(&v, p, sizeof(v));
                return v;
            }
            case Boolean:
                return *(const bool*) p;
            case Flag:
                return *p != 0;
            }
            return 0;
        }
        void assign(void* object, const Field& field, double value)
        {
            auto* p = (uint8_t*) object + field.offset;
            switch (field.storage) {
            case U8:
                *p = (uint8_t) value;
                break;
            case I8:
                *(int8_t*) p = (int8_t) value;
                break;
            case U16: {
                uint16_t v = (uint16_t) value;
                std::memcpy(p, &v, 2);
                break;
            }
            case I16: {
                int16_t v = (int16_t) value;
                std::memcpy(p, &v, 2);
                break;
            }
            case U32: {
                uint32_t v = (uint32_t) value;
                std::memcpy(p, &v, 4);
                break;
            }
            case F32: {
                float v = (float) value;
                std::memcpy(p, &v, sizeof(v));
                break;
            }
            case Boolean:
                *(bool*) p = value != 0;
                break;
            case Flag:
                *p = value != 0;
                break;
            }
        }
        void schema_field(Reply& r, yyjson_mut_val* row, const Field& f,
                          bool readonly)
        {
            add_string(r, row, "type",
                       (f.storage == Boolean || f.storage == Flag) ? "boolean"
                       : f.scale == 1 && f.storage != F32           ? "integer"
                                                                   : "number");
            add_string(r, row, "description", f.description);
            yyjson_mut_obj_add_bool(r.document, row, "readOnly", readonly);
            if ((f.storage != Boolean && f.storage != Flag)) {
                yyjson_mut_obj_add_real(r.document, row, "minimum",
                                        f.minimum * f.scale);
                yyjson_mut_obj_add_real(r.document, row, "maximum",
                                        f.maximum * f.scale);
                if (f.storage != F32)
                    yyjson_mut_obj_add_real(r.document, row, "quantum", f.scale);
            }
            if (f.launch_mode)
                yyjson_mut_obj_add_uint(r.document, row, "launchMode", f.launch_mode);
            if (!std::strcmp(f.name, "element")) {
                auto* options = yyjson_mut_arr(r.document);
                yyjson_mut_obj_add_val(r.document, row, "choices", options);
                for (unsigned i = 0; i < 32; ++i) {
                    auto* item = yyjson_mut_obj(r.document);
                    yyjson_mut_arr_append(options, item);
                    yyjson_mut_obj_add_uint(r.document, item, "value", i);
                    add_string(r, item, "label", element_label(i));
                }
            } else if (!std::strcmp(f.name, "launch_mode")) {
                auto* options = yyjson_mut_arr(r.document);
                yyjson_mut_obj_add_val(r.document, row, "choices", options);
                const char* labels[] = { "Angle", "Toward point", "Follow hitbox" };
                for (unsigned i = 0; i < std::size(labels); ++i) {
                    auto* item = yyjson_mut_obj(r.document);
                    yyjson_mut_arr_append(options, item);
                    yyjson_mut_obj_add_uint(r.document, item, "value", i);
                    add_string(r, item, "label", labels[i]);
                }
            }
        }
        void describe(const void* object, const Field* fields, size_t count,
                      Reply& r, yyjson_mut_val* out, bool creating,
                      bool timed = true, unsigned launch_mode = NativeHitboxLaunch_Default)
        {
            auto* values = yyjson_mut_obj(r.document);
            auto* definitions = yyjson_mut_obj(r.document);
            yyjson_mut_obj_add_val(r.document, out, "properties", values);
            yyjson_mut_obj_add_val(r.document, out, "fields", definitions);
            for (size_t i = 0; i < count; ++i) {
                const auto& f = fields[i];
                auto* def = yyjson_mut_obj(r.document);
                yyjson_mut_obj_add_val(r.document, definitions, f.name, def);
                bool readonly =
                    f.read_only && !(creating && !std::strcmp(f.name, "id"));
                readonly |= !timed &&
                            (!std::strcmp(f.name, "duration") ||
                             !std::strcmp(f.name, "refresh_hit_eligibility") ||
                             !std::strcmp(f.name, "expire_all"));
                schema_field(r, def, f, readonly);
                if (f.launch_mode && f.launch_mode != launch_mode) continue;
                if (f.launch_mode == NativeHitboxLaunch_Follow &&
                    !static_cast<const NativeFighterHitbox*>(object)->launch.follow.override_ground_knockback &&
                    (!std::strcmp(f.name, "launch_override_ground_knockback") ||
                     !std::strcmp(f.name, "launch_ground_growth") ||
                     !std::strcmp(f.name, "launch_ground_base"))) continue;
                if (fields == hitbox_fields && !static_cast<const NativeFighterHitbox*>(object)->capsule &&
                    std::strncmp(f.name, "end_", 4) == 0) continue;
                auto* value =
                    (f.storage == Boolean || f.storage == Flag)
                        ? yyjson_mut_bool(r.document, number(object, f) != 0)
                    : f.scale == 1 && f.storage != F32
                        ? yyjson_mut_sint(r.document,
                                          (int64_t) number(object, f))
                        : yyjson_mut_real(r.document,
                                          number(object, f) * f.scale);
                if (!creating || !readonly) {
                    yyjson_mut_obj_add_val(r.document, values, f.name, value);
                }
            }
        }
        bool patch(void* object, const Field* fields, size_t count,
                   yyjson_val* properties, bool creating, std::string& error,
                   bool timed = true, unsigned launch_mode = NativeHitboxLaunch_Default)
        {
            size_t index, n;
            yyjson_val *key, *value;
            yyjson_obj_foreach(properties, index, n, key, value)
            {
                const char* name = yyjson_get_str(key);
                const Field* field = nullptr;
                for (size_t i = 0; i < count; ++i) {
                    if (!std::strcmp(fields[i].name, name)) {
                        field = fields + i;
                        break;
                    }
                }
                if (!field) {
                    error = std::string("This event has no property named ") +
                            name + '.';
                    return false;
                }
                if (field->launch_mode && field->launch_mode != launch_mode) {
                    error = std::string(name) + " belongs to another launch mode.";
                    return false;
                }
                bool readonly = field->read_only &&
                                !(creating && !std::strcmp(name, "id"));
                readonly |=
                    !timed && (!std::strcmp(name, "duration") ||
                               !std::strcmp(name, "refresh_hit_eligibility") ||
                               !std::strcmp(name, "expire_all"));
                if (readonly) {
                    error = std::string(name) + " is read-only on this event.";
                    return false;
                }
                if (((field->storage == Boolean || field->storage == Flag) &&
                     !yyjson_is_bool(value)) ||
                    ((field->storage != Boolean && field->storage != Flag) &&
                     !yyjson_is_num(value)))
                {
                    error = std::string("Invalid type for ") + name + '.';
                    return false;
                }
                double v =
                    (field->storage == Boolean || field->storage == Flag)
                        ? double(yyjson_get_bool(value))
                        : yyjson_get_num(value) / field->scale;
                if (!std::isfinite(v) || v < field->minimum ||
                    v > field->maximum ||
                    (field->storage != F32 && field->scale == 1 && std::floor(v) != v))
                {
                    error =
                        std::string("Out-of-range value for ") + name + '.';
                    return false;
                }
                assign(object, *field, field->storage == F32 ? v : std::round(v));
            }
            return true;
        }
        bool patch_hitbox(NativeFighterHitbox& hitbox, yyjson_val* properties,
                          bool creating, std::string& error)
        {
            if (auto* mode = yyjson_obj_get(properties, "launch_mode")) {
                const double value = yyjson_get_num(mode);
                if (!yyjson_is_num(mode) || value < 0 || value > 2 || std::floor(value) != value) {
                    error = "Choose launch_mode 0 (Angle), 1 (Toward point), or 2 (Follow hitbox).";
                    return false;
                }
                if (hitbox.launch.mode != (uint32_t) value)
                    hitbox.launch = nativeHitboxLaunch_Defaults((uint32_t) value);
            }
            const bool was_capsule = hitbox.capsule;
            const bool separate_ground_knockback = hitbox.launch.mode == NativeHitboxLaunch_Follow &&
                                                   hitbox.launch.follow.override_ground_knockback;
            if (!patch(&hitbox, hitbox_fields, std::size(hitbox_fields),
                       properties, creating, error, hitbox.timed, hitbox.launch.mode))
                return false;
            if (hitbox.launch.mode == NativeHitboxLaunch_Follow) {
                auto& follow = hitbox.launch.follow;
                const bool has_growth = yyjson_obj_get(properties, "launch_ground_growth") != nullptr;
                const bool has_base = yyjson_obj_get(properties, "launch_ground_base") != nullptr;
                if (!follow.override_ground_knockback && (has_growth || has_base)) {
                    error = "Enable separate grounded knockback to change its growth or base.";
                    return false;
                }
                if (follow.override_ground_knockback && !separate_ground_knockback) {
                    if (!has_growth) follow.ground_growth = hitbox.knockback_growth;
                    if (!has_base) follow.ground_base = hitbox.base_knockback;
                }
            }
            if (!nativeHitboxLaunch_IsValid(&hitbox.launch)) {
                error = "Launch parameters are out of range.";
                return false;
            }
            if (hitbox.capsule && !was_capsule) {
                if (!yyjson_obj_get(properties, "end_x")) hitbox.end_x = hitbox.offset_x;
                if (!yyjson_obj_get(properties, "end_y")) hitbox.end_y = hitbox.offset_y;
                if (!yyjson_obj_get(properties, "end_z")) hitbox.end_z = hitbox.offset_z;
            } else if (!hitbox.capsule &&
                       (yyjson_obj_get(properties, "end_x") ||
                        yyjson_obj_get(properties, "end_y") ||
                        yyjson_obj_get(properties, "end_z"))) {
                error = "Enable the capsule to edit its second endpoint.";
                return false;
            }
            return true;
        }
        const char* known_field_name(unsigned opcode, unsigned field)
        {
            static const char* throws[] = { "slot",
                                            "damage",
                                            "angle",
                                            "knockback_growth",
                                            "weight_set_knockback",
                                            "base_knockback",
                                            "element",
                                            "hit_sfx_severity",
                                            "hit_sfx_kind" };
            static const char* variable[] = { "variable", "value" };
            static const char* hurt[] = { "bone", "state" };
            static const char* visibility[] = { "group", "variant" };
            if (ftAction_IsThrowHitbox(opcode) && field < std::size(throws)) {
                return throws[field];
            }
            if (opcode == 19 && field < 2) {
                return variable[field];
            }
            if (opcode == 28 && field < 2) {
                return hurt[field];
            }
            if (opcode == 31 && field < 2) {
                return visibility[field];
            }
            if ((opcode >= 21 && opcode <= 27) || opcode == 15) {
                return field == 0 ? "value" : nullptr;
            }
            return nullptr;
        }
        std::string property_name(unsigned opcode, unsigned index)
        {
            if (const char* known = known_field_name(opcode, index)) {
                return known;
            }
            const char* label =
                NativeFighterActionLabels::field(opcode, index);
            if (!label) {
                return "field_" + std::to_string(index);
            }
            if (!std::strcmp(label, "Reserved")) {
                return "reserved_" + std::to_string(index);
            }
            std::string name;
            for (const unsigned char* p = (const unsigned char*) label; *p;
                 ++p)
            {
                if ((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9')) {
                    name += (char) *p;
                } else if (*p >= 'A' && *p <= 'Z') {
                    name += (char) (*p + 'a' - 'A');
                } else if (!name.empty() && name.back() != '_') {
                    name += '_';
                }
            }
            while (!name.empty() && name.back() == '_') {
                name.pop_back();
            }
            return name;
        }
        struct Generic {
            NativeFighterActionField fields[48];
            size_t count = 0;
            std::string names[48];
        };
        bool generic(const NativeFighterProject& p, unsigned action,
                     uint32_t source, unsigned opcode, Generic& g)
        {
            if (opcode == 1 || opcode == 2) {
                g.count = 1;
                g.fields[0] = { 0, 0, 0, 26, false };
                g.names[0] = "value";
                return true;
            }
            if (!nativeFighterAction_Fields(&p, action, source, g.fields, 48,
                                            &g.count))
            {
                return false;
            }
            for (size_t i = 0; i < g.count; ++i) {
                g.names[i] = property_name(opcode, (unsigned) i);
            }
            return true;
        }
        void bone_range(const NativeFighterProject& p, Reply& r,
                        yyjson_mut_val* out, bool common, bool ignored = false)
        {
            auto* field =
                yyjson_mut_obj_get(yyjson_mut_obj_get(out, "fields"), "bone");
            if (!field) {
                return;
            }
            const unsigned count =
                common ? FT_COMMON_PART_COUNT
                       : p.facts.joints + p.facts.omission_count;
            yyjson_mut_obj_remove_str(field, "maximum");
            yyjson_mut_obj_add_uint(
                r.document, field, "maximum",
                ignored ? 255 : std::min(255u, count ? count - 1 : 0));
        }
        NativeFighterHitbox initial_hitbox()
        {
            NativeFighterHitbox h{};
            h.timed = true;
            h.duration256 = 256;
            h.refresh_hit_eligibility = true;
            h.damage = 1;
            h.radius = 896;
            h.angle = 361;
            h.knockback_growth = 100;
            h.clank = true;
            h.hit_grounded = h.hit_aerial = true;
            return h;
        }
    }

    const char* element_label(unsigned i)
    {
        static const char* names[] = {
            "Normal",  "Fire",  "Electric",     "Slash",    "Coin",  "Ice",
            "Nap",     "Sleep", "Catch",        "Ground",   "Cape",  "Inert",
            "Disable", "Dark",  "Screw Attack", "Lipstick", "ReDead"
        };
        return i < std::size(names) ? names[i] : "Unknown";
    }
    const std::vector<Type>& types()
    {
        static const auto values = [] {
            std::vector<Type> result;
            for (unsigned op = 1; op <= 59; ++op) {
                if (op >= 3 && op <= 8 || op == 11) {
                    continue;
                }
                const char* name = op == 1    ? "wait"
                                   : op == 2  ? "wait_until"
                                   : op == 10 ? "effect"
                                   : op == 15 ? "clear_hitbox"
                                   : op == 16 ? "clear_hitboxes"
                                   : op == 17 ? "sound"
                                   : op == 19 ? "command_variable"
                                   : op == 20 ? "release_throw"
                                   : op == 23 ? "interrupt"
                                   : op == 24 ? "neutral_cue"
                                   : op == 25 ? "ground_air"
                                   : op == 26 ? "body_collision"
                                   : op == 27 ? "hurt_state"
                                   : op == 28 ? "bone_hurt_state"
                                   : op == 34 ? "throw"
                                   : op == 59 ? "hitbox"
                                              : nullptr;
                result.push_back(
                    { name ? name : "opcode_" + std::to_string(op),
                      NativeFighterActionLabels::command(op),
                      NativeFighterActionLabels::command_description(op), op,
                      0 });
                if (op == 20) {
                    result.push_back(
                        { "reverse_facing", "Reverse facing",
                          NativeFighterActionLabels::command_description(op),
                          op, 1 });
                }
            }
            result.push_back({ "execute_script", "Execute Script",
                NativeFighterActionLabels::command_description(FT_ACTION_SCRIPT_OPCODE),
                FT_ACTION_SCRIPT_OPCODE, 0 });
            return result;
        }();
        return values;
    }
    const Type* type(unsigned op, unsigned selector)
    {
        if (nativeHitbox_IsCreate(op)) {
            op = 59;
        } else if (ftAction_IsThrowHitbox(op)) {
            op = FT_ACTION_THROW_OPCODE;
        }
        for (const auto& t : types()) {
            if (t.opcode == op &&
                (op != 20 || t.selector == (selector & 0x3ffffff)))
            {
                return &t;
            }
        }
        return nullptr;
    }
    const Type* type(const char* name)
    {
        for (const auto& t : types()) {
            if (t.name == name) {
                return &t;
            }
        }
        return nullptr;
    }
    const Type* type_for_event(unsigned kind)
    {
        static const char* names[] = { "wait",
                                       "wait_until",
                                       "hitbox",
                                       "opcode_12",
                                       "opcode_13",
                                       "opcode_14",
                                       "command_variable",
                                       "body_collision",
                                       "hurt_state",
                                       "bone_hurt_state",
                                       "sound",
                                       "effect",
                                       "throw",
                                       "interrupt",
                                       "neutral_cue",
                                       "clear_hitbox",
                                       "clear_hitboxes",
                                       "release_throw",
                                       "reverse_facing",
                                       "ground_air", nullptr, "execute_script" };
        return kind < std::size(names) && names[kind] ? type(names[kind]) : nullptr;
    }

    static void script_properties(Reply& r, yyjson_mut_val* out, const char* name)
    {
        auto* props = yyjson_mut_obj(r.document);
        auto* fields = yyjson_mut_obj(r.document);
        auto* function = yyjson_mut_obj(r.document);
        add_string(r, props, "function", name);
        add_string(r, function, "type", "string");
        add_string(r, function, "description", "Name of a fighter script function that takes no arguments.");
        yyjson_mut_obj_add_val(r.document, fields, "function", function);
        yyjson_mut_obj_add_val(r.document, out, "properties", props);
        yyjson_mut_obj_add_val(r.document, out, "fields", fields);
    }

    void inspect(const NativeFighterProject& p, unsigned action,
                 const NativeFighterActionCommand& cmd, Reply& r,
                 yyjson_mut_val* out)
    {
        if (cmd.opcode == FT_ACTION_SCRIPT_OPCODE) {
            char name[FT_ACTION_SCRIPT_NAME_MAX + 1];
            if (nativeFighterAction_GetScript(&p, action, cmd.source_offset, name))
                script_properties(r, out, name);
            return;
        }
        NativeFighterHitbox h;
        NativeFighterSoundEffect sound;
        NativeFighterSpawnEffect effect;
        if (nativeHitbox_IsCreate(cmd.opcode) &&
            nativeFighterAction_GetHitbox(&p, action, cmd.source_offset, &h))
        {
            describe(&h, hitbox_fields, std::size(hitbox_fields), r, out,
                     false, h.timed, h.launch.mode);
            bone_range(p, r, out, h.use_common_bone_ids);
            return;
        }
        if (cmd.opcode == 17 && nativeFighterAction_GetSoundEffect(
                                    &p, action, cmd.source_offset, &sound))
        {
            describe(&sound, sound_fields, std::size(sound_fields), r, out,
                     false);
            if (sound.behavior > 6) {
                auto* fields = yyjson_mut_obj_get(out, "fields");
                for (const auto& field : sound_fields) {
                    auto* row = yyjson_mut_obj_get(fields, field.name);
                    yyjson_mut_obj_remove_str(row, "readOnly");
                    yyjson_mut_obj_add_bool(r.document, row, "readOnly", true);
                }
            }
            return;
        }
        if (cmd.opcode == 10 && nativeFighterAction_GetSpawnEffect(
                                    &p, action, cmd.source_offset, &effect))
        {
            describe(&effect, effect_fields, std::size(effect_fields), r, out,
                     false);
            bone_range(p, r, out, effect.use_common_bone_ids,
                       effect.use_unknown_bone);
            return;
        }
        Generic g;
        if (!generic(p, action, cmd.source_offset, cmd.opcode, g)) {
            return;
        }
        if (cmd.opcode < 9) {
            auto* at = p.fighter_dat + 32 + cmd.source_offset;
            g.fields[0].value =
                ((uint32_t(at[0]) << 24) | (uint32_t(at[1]) << 16) |
                 (uint32_t(at[2]) << 8) | at[3]) &
                0x3ffffff;
        }
        auto* props = yyjson_mut_obj(r.document);
        auto* fields = yyjson_mut_obj(r.document);
        yyjson_mut_obj_add_val(r.document, out, "properties", props);
        yyjson_mut_obj_add_val(r.document, out, "fields", fields);
        for (size_t i = 0; i < g.count; ++i) {
            auto* def = yyjson_mut_obj(r.document);
            named(r.document, fields, g.names[i], def);
            const bool signed_variant = cmd.opcode == 31 && i == 1;
            const auto& field = g.fields[i];
            int64_t value = (int64_t) field.value;
            if (signed_variant && (value & (1u << 18))) {
                value -= 1u << 19;
            }
            named(r.document, props, g.names[i],
                  field.fractional ? yyjson_mut_real(r.document, field.value)
                                   : yyjson_mut_sint(r.document, value));
            add_string(r, def, "type", field.fractional ? "number" : "integer");
            const char* label =
                NativeFighterActionLabels::field(cmd.opcode, (unsigned) i);
            add_string(r, def, "description",
                       label ? label
                             : "Original command operand; its bit width "
                               "defines the available range.");
            yyjson_mut_obj_add_sint(r.document, def, "minimum",
                                    signed_variant ? -(1 << 18) : 0);
            yyjson_mut_obj_add_uint(
                r.document, def, "maximum",
                signed_variant
                    ? (1u << 18) - 1
                    : ftActionData_SourceFieldMaximum(cmd.opcode, (unsigned) i,
                                                      g.fields[i].width));
            yyjson_mut_obj_add_bool(r.document, def, "readOnly",
                                    label && !std::strcmp(label, "Reserved"));
            yyjson_mut_obj_add_bool(r.document, def, "expert",
                                    g.names[i].find("field_") == 0);
        }
    }

    bool update(NativeFighterProject& p, unsigned action, uint32_t& source,
                yyjson_val* props, std::string& error)
    {
        NativeFighterHitbox h;
        NativeFighterSoundEffect sound;
        NativeFighterSpawnEffect effect;
        if (nativeFighterAction_GetHitbox(&p, action, source, &h)) {
            return patch_hitbox(h, props, false, error) &&
                   nativeFighterAction_SetHitbox(&p, action, source, &h, nullptr);
        }
        if (nativeFighterAction_GetSoundEffect(&p, action, source, &sound)) {
            if (sound.behavior > 6 && yyjson_obj_size(props)) {
                error = "This canned sound is read-only.";
                return false;
            }
            return patch(&sound, sound_fields, std::size(sound_fields), props,
                         false, error) &&
                   nativeFighterAction_SetSoundEffect(&p, action, source,
                                                      &sound);
        }
        if (nativeFighterAction_GetSpawnEffect(&p, action, source, &effect)) {
            return patch(&effect, effect_fields, std::size(effect_fields),
                         props, false, error) &&
                   nativeFighterAction_SetSpawnEffect(&p, action, source,
                                                      &effect);
        }
        char old_function[FT_ACTION_SCRIPT_NAME_MAX + 1];
        if (nativeFighterAction_GetScript(&p, action, source, old_function)) {
            if (!yyjson_obj_size(props)) return true;
            auto* value = yyjson_obj_get(props, "function");
            const char* name = yyjson_get_str(value);
            if (yyjson_obj_size(props) == 1 && name && std::strlen(name) == yyjson_get_len(value) &&
                nativeFighterAction_SetScript(&p, action, source, name, &source)) return true;
            error = "Choose a valid fighter script function name.";
            return false;
        }
        size_t n = 0;
        if (!nativeFighterAction_Commands(&p, action, nullptr, 0, &n)) {
            return false;
        }
        std::vector<NativeFighterActionCommand> cmds(n);
        if (!nativeFighterAction_Commands(&p, action, cmds.data(), n, &n)) {
            return false;
        }
        auto at = std::find_if(cmds.begin(), cmds.end(), [&](const auto& c) {
            return c.source_offset == source;
        });
        if (at == cmds.end()) {
            return false;
        }
        Generic g;
        if (!generic(p, action, source, at->opcode, g)) {
            return false;
        }
        size_t i, count;
        yyjson_val *key, *value;
        yyjson_obj_foreach(props, i, count, key, value)
        {
            size_t field = 0;
            for (; field < g.count; ++field) {
                if (g.names[field] == yyjson_get_str(key)) {
                    break;
                }
            }
            if (field == g.count) {
                error = "Choose a property returned by the event view.";
                return false;
            }
            const bool signed_variant = at->opcode == 31 && field == 1;
            int64_t v = yyjson_get_sint(value);
            const int64_t minimum = signed_variant ? -(1 << 18) : 0;
            uint64_t maximum =
                signed_variant
                    ? (1u << 18) - 1
                    : ftActionData_SourceFieldMaximum(
                          at->opcode, (unsigned) field, g.fields[field].width);
            const char* label =
                NativeFighterActionLabels::field(at->opcode, (unsigned) field);
            if (label && !std::strcmp(label, "Reserved")) {
                error = "Reserved fields are read-only.";
                return false;
            }
            if (g.fields[field].fractional) {
                if (!yyjson_is_num(value) ||
                    !nativeFighterAction_SetField(
                        &p, action, source, (unsigned) field,
                        yyjson_get_num(value), &source)) {
                    error = "Throw damage is outside its allowed range.";
                    return false;
                }
                continue;
            }
            if (!yyjson_is_int(value) || v < minimum || v > (int64_t) maximum)
            {
                error = "Command field is outside its integer range.";
                return false;
            }
            const uint32_t encoded = signed_variant
                                         ? (uint32_t) v & ((1u << 19) - 1)
                                         : (uint32_t) v;
            bool ok =
                at->opcode < 9
                    ? nativeFighterAction_SetValue(&p, action, source, encoded)
                    : nativeFighterAction_SetField(&p, action, source,
                                                   (unsigned) field, encoded, nullptr);
            if (!ok) {
                return false;
            }
        }
        return true;
    }

    bool create(NativeFighterProject& p, unsigned action, unsigned frame,
                const Type& t, yyjson_val* props, uint32_t& source,
                std::string& error)
    {
        if (t.opcode == FT_ACTION_SCRIPT_OPCODE) {
            auto* value = yyjson_obj_get(props, "function");
            const char* name = yyjson_get_str(value);
            uint32_t words[FT_ACTION_MAX_WORDS];
            const unsigned count = yyjson_obj_size(props) == 1 && name &&
                    std::strlen(name) == yyjson_get_len(value)
                ? ftActionData_EncodeScript(name, words) : 0;
            if (!count) {
                error = "Choose a valid fighter script function name.";
                return false;
            }
            return nativeFighterAction_CreateEvent(&p, action, frame, words, count, &source);
        }
        uint32_t words[NATIVE_HITBOX_MAX_WORDS]{};
        words[0] = t.opcode << 26 | t.selector;
        size_t width = ftActionData_SourceCommandWidth(words[0]);
        if (t.opcode == 59) {
            auto h = initial_hitbox();
            if (!patch_hitbox(h, props, true, error))
            {
                return false;
            }
            if ((h.use_common_bone_ids && h.bone >= FT_COMMON_PART_COUNT) ||
                (!h.use_common_bone_ids &&
                 h.bone >= p.facts.joints + p.facts.omission_count))
            {
                error = "Choose a bone in this fighter's rig.";
                return false;
            }
            const bool chosen = yyjson_obj_get(props, "id") != nullptr;
            for (unsigned slot = 0; slot < (chosen ? 1u : 4u); ++slot) {
                if (!chosen) {
                    h.id = (uint8_t) slot;
                }
                const auto count = nativeFighterAction_EncodeHitbox(&h, words);
                if (count &&
                    nativeFighterAction_CreateEvent(&p, action, frame, words,
                                                    count, &source))
                {
                    return true;
                }
            }
            error = "No compatible hitbox slot or insertion point is "
                    "available at this frame.";
            return false;
        }
        if (t.opcode == 17) {
            words[2] = 127u << 8 | 64u;
        }
        if (!nativeFighterAction_CreateEvent(&p, action, frame, words, width,
                                             &source))
        {
            return false;
        }
        return update(p, action, source, props, error);
    }

    void defaults(const NativeFighterProject& p, const Type& t, Reply& r,
                  yyjson_mut_val* out)
    {
        if (t.opcode == FT_ACTION_SCRIPT_OPCODE) {
            script_properties(r, out, "");
            return;
        }
        if (t.opcode == 59) {
            auto h = initial_hitbox();
            describe(&h, hitbox_fields, std::size(hitbox_fields), r, out,
                     true);
            bone_range(p, r, out, false);
            return;
        }
        if (t.opcode == 17) {
            NativeFighterSoundEffect s{ 0, 127, 64, 0, false };
            describe(&s, sound_fields, std::size(sound_fields), r, out, true);
            return;
        }
        if (t.opcode == 10) {
            NativeFighterSpawnEffect e{};
            describe(&e, effect_fields, std::size(effect_fields), r, out,
                     true);
            bone_range(p, r, out, false);
            return;
        }
        auto* props = yyjson_mut_obj(r.document);
        auto* defs = yyjson_mut_obj(r.document);
        yyjson_mut_obj_add_val(r.document, out, "properties", props);
        yyjson_mut_obj_add_val(r.document, out, "fields", defs);
        // Field widths come directly from the game's command codec, including
        // operand words with no packed-field layout.
        size_t field = 0;
        for (unsigned word = 0;
             word < ftActionData_SourceCommandWidth(t.opcode << 26); ++word)
        {
            uint8_t widths[8];
            unsigned n =
                ftActionData_SourceFieldWidths(t.opcode, word, widths);
            if (!n) {
                n = 1;
                widths[0] = 32;
            }
            for (unsigned bit = 0; bit < n; ++bit) {
                if (!word && !bit) {
                    continue;
                }
                std::string name = property_name(t.opcode, (unsigned) field);
                if (t.opcode < 9) {
                    name = "value";
                }
                const char* label = NativeFighterActionLabels::field(
                    t.opcode, (unsigned) field);
                const bool reserved = label && !std::strcmp(label, "Reserved");
                if (!reserved) {
                    named(r.document, props, name,
                          yyjson_mut_uint(r.document,
                                          word == 0 ? t.selector : 0));
                }
                auto* def = yyjson_mut_obj(r.document);
                named(r.document, defs, name, def);
                add_string(r, def, "type",
                           t.opcode == FT_ACTION_THROW_OPCODE && field == 1
                               ? "number" : "integer");
                yyjson_mut_obj_add_sint(
                    r.document, def, "minimum",
                    t.opcode == 31 && field == 1 ? -(1 << 18) : 0);
                yyjson_mut_obj_add_uint(
                    r.document, def, "maximum",
                    t.opcode == 31 && field == 1
                        ? (1u << 18) - 1
                        : ftActionData_SourceFieldMaximum(
                              t.opcode, (unsigned) field, widths[bit]));
                yyjson_mut_obj_add_bool(r.document, def, "readOnly", reserved);
                ++field;
            }
        }
        (void) p;
    }

    std::string schema()
    {
        auto* doc = yyjson_mut_doc_new(nullptr);
        Reply r(doc);
        auto* root = yyjson_mut_obj(doc);
        yyjson_mut_doc_set_root(doc, root);
        add_string(r, root, "type", "object");
        auto* fields = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_val(doc, root, "properties", fields);
        yyjson_mut_obj_add_bool(doc, root, "additionalProperties", false);
        for (auto span :
             { std::pair{ hitbox_fields, std::size(hitbox_fields) },
               std::pair{ sound_fields, std::size(sound_fields) },
               std::pair{ effect_fields, std::size(effect_fields) } })
        {
            for (size_t i = 0; i < span.second; ++i) {
                const auto& f = span.first[i];
                auto* row = yyjson_mut_obj(doc);
                schema_field(r, row, f, false);
                yyjson_mut_obj_remove_str(row, "quantum");
                yyjson_mut_obj_remove_str(row, "choices");
                yyjson_mut_obj_remove_str(fields, f.name);
                yyjson_mut_obj_add_val(doc, fields, f.name, row);
            }
        }
        // Shared names can have different widths/units between event types.
        // The event's own descriptors enforce its exact range after schema
        // validation.
        for (const char* name :
             { "damage", "element", "angle", "knockback_growth",
               "weight_set_knockback", "base_knockback", "hit_sfx_severity",
               "hit_sfx_kind", "offset_x", "offset_y", "offset_z", "value",
               "variable", "state", "slot", "group", "variant" })
        {
            auto* row = yyjson_mut_obj(doc);
            add_string(r, row, "type", "number");
            yyjson_mut_obj_remove_str(fields, name);
            yyjson_mut_obj_add_val(doc, fields, name, row);
        }
        auto* function = yyjson_mut_obj(doc);
        add_string(r, function, "type", "string");
        yyjson_mut_obj_add_val(doc, fields, "function", function);
        for (const auto& t : types()) {
            unsigned field = 0;
            for (unsigned word = 0;
                 word < ftActionData_SourceCommandWidth(t.opcode << 26);
                 ++word)
            {
                uint8_t widths[8];
                unsigned n =
                    ftActionData_SourceFieldWidths(t.opcode, word, widths);
                if (!n) {
                    n = 1;
                    widths[0] = 32;
                }
                for (unsigned bit = 0; bit < n; ++bit) {
                    if (!word && !bit) {
                        continue;
                    }
                    const std::string name = property_name(t.opcode, field++);
                    if (yyjson_mut_obj_get(fields, name.c_str())) {
                        continue;
                    }
                    auto* row = yyjson_mut_obj(doc);
                    add_string(r, row, "type", "integer");
                    named(doc, fields, name, row);
                }
            }
        }
        for (unsigned i = 0; i < 48; ++i) {
            auto* row = yyjson_mut_obj(doc);
            add_string(r, row, "type", "integer");
            yyjson_mut_obj_add_uint(doc, row, "minimum", 0);
            yyjson_mut_obj_add_uint(doc, row, "maximum", UINT32_MAX);
            named(doc, fields, "field_" + std::to_string(i), row);
        }
        size_t size = 0;
        char* bytes = yyjson_mut_write(doc, 0, &size);
        std::string result(bytes ? bytes : "{}", size);
        std::free(bytes);
        yyjson_mut_doc_free(doc);
        return result;
    }
}
