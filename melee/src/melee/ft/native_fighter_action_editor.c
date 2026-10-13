#include "native_fighter_action_editor.h"

#ifdef MELEE_NATIVE

#include "ftaction_data.h"
#include "ftdat.h"
#include <sysdolphin/baselib/dat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool action_table(const NativeFighterProject* project, HSD_Dat* dat,
                         u32* table)
{
    u32 root;
    const char* external;
    return project != NULL && project->action_count > 0 &&
           project->action_count <= 1024 &&
           HSD_DatOpen(dat, project->fighter_dat,
                       project->fighter_size) == HSD_DAT_OK &&
           HSD_DatFindRoot(dat, project->fighter_root, &root) &&
           HSD_DatResolve(dat, root + 12, table, &external) ==
               HSD_DAT_REF_LOCAL &&
           HSD_DatBytes(dat, *table,
                        project->action_count * 24) != NULL;
}

static bool action_source(const NativeFighterProject* project,
                          const HSD_Dat* dat, u32 table, unsigned action,
                          u32* source)
{
    const char* external;
    return action < project->action_count &&
           HSD_DatResolve(dat, table + action * 24 + 12, source,
                          &external) == HSD_DAT_REF_LOCAL &&
           (*source & 3) == 0 && *source < table;
}

bool nativeFighterAction_GetMetadata(const NativeFighterProject* project,
                                     unsigned action,
                                     NativeFighterActionMetadata* metadata)
{
    HSD_Dat dat;
    u32 table;
    u32 offset, size, flags;
    if (metadata == NULL || project == NULL ||
        action >= project->action_count ||
        !action_table(project, &dat, &table) ||
        !HSD_DatReadU32(&dat, table + action * 24 + 4, &offset) ||
        !HSD_DatReadU32(&dat, table + action * 24 + 8, &size) ||
        !HSD_DatReadU32(&dat, table + action * 24 + 16, &flags)) {
        return false;
    }
    *metadata = (NativeFighterActionMetadata){
        .animation_offset = (int32_t) offset,
        .animation_size = (int32_t) size,
        .animation_flags = flags,
    };
    return true;
}

bool nativeFighterAction_SetAnimationFlags(NativeFighterProject* project,
                                           unsigned action, uint32_t flags)
{
    NativeFighterActionMetadata metadata;
    if (!nativeFighterAction_GetMetadata(project, action, &metadata) ||
        ((flags ^ metadata.animation_flags) & 0x00C00000U) != 0) {
        return false;
    }
    if (flags == metadata.animation_flags) return true;
    HSD_Dat dat;
    u32 table;
    if (!action_table(project, &dat, &table)) return false;
    u8* bytes = project->fighter_dat + 32 + table + action * 24 + 16;
    bytes[0] = (u8) (flags >> 24);
    bytes[1] = (u8) (flags >> 16);
    bytes[2] = (u8) (flags >> 8);
    bytes[3] = (u8) flags;
    return true;
}

bool nativeFighterAction_List(const NativeFighterProject* project,
                              NativeFighterActionInfo* actions,
                              size_t capacity, size_t* count)
{
    HSD_Dat dat;
    u32 table;
    if (count == NULL || !action_table(project, &dat, &table)) {
        return false;
    }
    *count = project->action_count;
    if (actions == NULL) {
        return true;
    }
    if (capacity < *count) {
        return false;
    }
    for (unsigned i = 0; i < *count; ++i) {
        NativeFighterActionInfo* info = &actions[i];
        memset(info, 0, sizeof(*info));
        info->index = (uint16_t) i;
        u32 script;
        info->has_script = action_source(project, &dat, table, i, &script);
        snprintf(info->name, sizeof(info->name), "Action %u", i);
        u32 name_source;
        const char* external;
        if (HSD_DatResolve(&dat, table + i * 24, &name_source,
                           &external) != HSD_DAT_REF_LOCAL) {
            continue;
        }
        const char* name = (const char*) HSD_DatBytes(&dat, name_source, 1);
        if (name == NULL ||
            memchr(name, 0, dat.data_size - name_source) == NULL) {
            return false;
        }
        const char* action_name = strstr(name, "_ACTION_");
        if (action_name == NULL) {
            continue;
        }
        action_name += 8;
        const char* suffix = strstr(action_name, "_figatree");
        size_t length = suffix != NULL ? (size_t) (suffix - action_name)
                                       : strlen(action_name);
        if (length == 0 || length >= sizeof(info->name)) {
            continue;
        }
        memcpy(info->name, action_name, length);
        info->name[length] = 0;
    }
    return true;
}

static int compare_events(const void* a, const void* b)
{
    const NativeFighterActionEvent* left = a;
    const NativeFighterActionEvent* right = b;
    return (left->source_offset > right->source_offset) -
           (left->source_offset < right->source_offset);
}

static int compare_commands(const void* a, const void* b)
{
    const NativeFighterActionCommand* left = a;
    const NativeFighterActionCommand* right = b;
    return (left->source_offset > right->source_offset) -
           (left->source_offset < right->source_offset);
}

static bool collect_action(const NativeFighterProject* project,
                           unsigned action, NativeFighterActionEvent* events,
                           size_t event_capacity, size_t* event_count,
                           NativeFighterActionCommand* commands,
                           size_t command_capacity, size_t* command_count)
{
    HSD_Dat dat;
    u32 table;
    u32 entry;
    if ((event_count == NULL && command_count == NULL) ||
        (event_count != NULL && events == NULL) ||
        (command_count == NULL && commands != NULL) ||
        !action_table(project, &dat, &table) ||
        !action_source(project, &dat, table, action, &entry)) {
        return false;
    }
    if (event_count != NULL) *event_count = 0;
    if (command_count != NULL) *command_count = 0;
    size_t words = table / 4;
    u8* visited = calloc(words, 1);
    u32* pending = malloc(words * sizeof(*pending));
    if (visited == NULL || pending == NULL) {
        free(visited);
        free(pending);
        return false;
    }
    bool ok = true;
    size_t pending_count = 1;
    pending[0] = entry;
    while (pending_count != 0 && ok) {
        u32 at = pending[--pending_count];
        while (true) {
            if ((at & 3) != 0 || at >= table) {
                ok = false;
                break;
            }
            u32 index = at / 4;
            if (visited[index] == 1) {
                break;
            }
            u32 raw;
            if (visited[index] != 0 || !HSD_DatReadU32(&dat, at, &raw)) {
                ok = false;
                break;
            }
            u32 width = ftActionData_SourceCommandWidth(raw);
            if (width == 0 || width > (table - at) / 4) {
                ok = false;
                break;
            }
            visited[index] = 1;
            for (u32 word = 1; word < width; ++word) {
                if (visited[index + word] != 0) {
                    ok = false;
                    break;
                }
                visited[index + word] = 2;
            }
            if (!ok) break;
            u32 opcode = raw >> 26;
            if (command_count != NULL) {
                if (commands != NULL && *command_count == command_capacity) {
                    ok = false;
                    break;
                }
                if (commands != NULL) {
                    commands[*command_count] = (NativeFighterActionCommand){
                        at, (u8) opcode, (u8) width
                    };
                }
                ++*command_count;
            }
            if (event_count != NULL &&
                (opcode == 1 || opcode == 2 ||
                (opcode >= 10 && opcode <= 17) ||
                opcode == 19 || opcode == 34 || opcode == 25 ||
                (opcode == 20 && (raw & 0x3FFFFFF) <= 1) ||
                opcode == 26 || opcode == 27 || opcode == 28 ||
                opcode == 23 || opcode == 24)) {
                if (*event_count == event_capacity) {
                    ok = false;
                    break;
                }
                NativeFighterActionEvent* event = &events[(*event_count)++];
                event->source_offset = at;
                event->target_id = opcode == 10 ? (raw >> 18) & 0xFF
                                   : opcode >= 11 && opcode <= 13
                                       ? (raw >> 23) & 7
                                   : opcode == 14 ? (raw >> 2) & 0xFFFFFF
                                   : opcode == 15 ? raw & 0x3FFFFFF
                                   : opcode == 17 ? (raw >> 18) & 0xFF
                                   : opcode == 19 ? (raw >> 24) & 3
                                   : opcode == 28 ? (raw >> 18) & 0xFF
                                   : opcode == 34 ? (raw >> 23) & 7 : 0;
                if (opcode == 10 || opcode == 17) {
                    u32 detail;
                    if (!HSD_DatReadU32(&dat, at + 4, &detail)) {
                        ok = false;
                        break;
                    }
                    event->value = opcode == 10 ? detail >> 16 : detail;
                } else event->value = opcode == 11 ? raw & 0x3FF
                             : opcode == 12 || opcode == 13
                                 ? raw & 0x7FFFFF
                             : opcode == 14 ? raw & 3
                             : opcode == 19 ? raw & 0xFFFFFF
                             : opcode == 34 ? raw & 0x7FFFFF
                             : opcode == 28 ? raw & 0x3FFFF
                             : opcode == 15 || opcode == 16 ||
                               opcode == 23 || opcode == 24 ? 0
                                            : raw & 0x3FFFFFF;
                event->kind = opcode == 1 ? NativeFighterActionEvent_SyncTimer
                            : opcode == 2 ? NativeFighterActionEvent_AsyncTimer
                            : opcode == 10 ? NativeFighterActionEvent_SpawnEffect
                            : opcode == 11 ? NativeFighterActionEvent_CreateHitbox
                            : opcode == 12 ? NativeFighterActionEvent_SetHitboxDamage
                            : opcode == 13 ? NativeFighterActionEvent_SetHitboxScale
                            : opcode == 14 ? NativeFighterActionEvent_SetHitboxFlag
                            : opcode == 15 ? NativeFighterActionEvent_ClearHitbox
                            : opcode == 16 ? NativeFighterActionEvent_ClearAllHitboxes
                            : opcode == 19 ? NativeFighterActionEvent_SetCommandVar
                            : opcode == 26 ? NativeFighterActionEvent_SetBodyCollisionState
                            : opcode == 27 ? NativeFighterActionEvent_SetGlobalHurtState
                            : opcode == 28 ? NativeFighterActionEvent_SetBoneHurtState
                            : opcode == 17 ? NativeFighterActionEvent_SoundEffect
                            : opcode == 34 ? NativeFighterActionEvent_ThrowHitbox
                            : opcode == 20 ? ((raw & 0x3FFFFFF) == 0
                                ? NativeFighterActionEvent_ReleaseThrow
                                : NativeFighterActionEvent_ReverseFacing)
                            : opcode == 25 ? NativeFighterActionEvent_SetAirborneState
                            : opcode == 23 ? NativeFighterActionEvent_AllowInterrupt
                                           : NativeFighterActionEvent_NeutralCue;
            }
            if (opcode == 5 || opcode == 7) {
                u32 target;
                const char* external;
                HSD_DatReferenceKind kind = HSD_DatResolve(
                    &dat, at + 4, &target, &external);
                if (kind == HSD_DAT_REF_LOCAL && (target & 3) == 0 &&
                    target < table && pending_count < words) {
                    pending[pending_count++] = target;
                } else if (kind != HSD_DAT_REF_NULL) {
                    ok = false;
                    break;
                }
            }
            if (opcode == 0 || opcode == 6 || opcode == 7) break;
            at += width * 4;
        }
    }
    free(pending);
    free(visited);
    if (ok) {
        if (event_count != NULL)
            qsort(events, *event_count, sizeof(*events), compare_events);
        if (command_count != NULL && commands != NULL)
            qsort(commands, *command_count, sizeof(*commands),
                  compare_commands);
    }
    return ok;
}

bool nativeFighterAction_Events(const NativeFighterProject* project,
                                unsigned action, NativeFighterActionEvent* events,
                                size_t capacity, size_t* count)
{
    if (events == NULL || capacity == 0 || count == NULL) return false;
    return collect_action(project, action, events, capacity, count,
                          NULL, 0, NULL);
}

bool nativeFighterAction_Commands(const NativeFighterProject* project,
                                  unsigned action,
                                  NativeFighterActionCommand* commands,
                                  size_t capacity, size_t* count)
{
    if (count == NULL || (commands == NULL && capacity != 0)) return false;
    return collect_action(project, action, NULL, 0, NULL,
                          commands, capacity, count);
}

bool nativeFighterAction_Timeline(const NativeFighterProject* project,
                                  unsigned action,
                                  NativeFighterActionOccurrence* occurrences,
                                  size_t capacity, size_t* count,
                                  bool* complete)
{
    HSD_Dat dat;
    u32 table, at;
    if (occurrences == NULL || capacity == 0 || count == NULL ||
        complete == NULL || !action_table(project, &dat, &table) ||
        !action_source(project, &dat, table, action, &at)) return false;
    typedef struct TimelineReturn {
        u32 offset;
        u32 remaining;
        bool loop;
    } TimelineReturn;
    TimelineReturn stack[16];
    size_t depth = 0;
    u32 frame = 0;
    *count = 0;
    *complete = false;
    for (size_t steps = 0; steps < 8192; ++steps) {
        u32 raw;
        if ((at & 3) != 0 || at >= table ||
            !HSD_DatReadU32(&dat, at, &raw)) return false;
        const u32 opcode = raw >> 26;
        const u32 width = ftActionData_SourceCommandWidth(raw);
        if (width == 0 || width > (table - at) / 4) return false;
        if (opcode == 1) frame += raw & 0x3FFFFFF;
        else if (opcode == 2) frame = raw & 0x3FFFFFF;
        if (frame > 1000 || *count == capacity) return true;
        occurrences[(*count)++] = (NativeFighterActionOccurrence){
            at, (uint16_t) frame, (uint8_t) opcode
        };
        const u32 next = at + width * 4;
        if (opcode == 0) {
            *complete = true;
            return true;
        }
        if (opcode == 3) {
            if (depth == 16) return true;
            stack[depth++] = (TimelineReturn){ next, raw & 0x3FFFFFF, true };
        } else if (opcode == 4) {
            if (depth == 0 || !stack[depth - 1].loop) return false;
            if (--stack[depth - 1].remaining != 0) {
                at = stack[depth - 1].offset;
                continue;
            }
            --depth;
        } else if (opcode == 5 || opcode == 7) {
            u32 target;
            const char* external;
            HSD_DatReferenceKind kind = HSD_DatResolve(
                &dat, at + 4, &target, &external);
            if (kind == HSD_DAT_REF_NULL) {
                *complete = true;
                return true;
            }
            if (kind != HSD_DAT_REF_LOCAL || (target & 3) != 0 ||
                target >= table) return false;
            if (opcode == 5) {
                if (depth == 16) return true;
                stack[depth++] = (TimelineReturn){ next, 0, false };
            }
            at = target;
            continue;
        } else if (opcode == 6) {
            if (depth == 0 || stack[depth - 1].loop) return false;
            at = stack[--depth].offset;
            continue;
        } else if (opcode == 8) {
            return true;
        }
        at = next;
    }
    return true;
}

static void write_be32(u8* bytes, u32 value)
{
    bytes[0] = (u8) (value >> 24);
    bytes[1] = (u8) (value >> 16);
    bytes[2] = (u8) (value >> 8);
    bytes[3] = (u8) value;
}

static int compare_u32(const void* left, const void* right)
{
    u32 a = *(const u32*) left, b = *(const u32*) right;
    return (a > b) - (a < b);
}

static u32 copied_offset(u32 source, u32 range_begin, u32 table,
                         u32 edit_at, u32 inserted, u32 removed)
{
    u32 offset = table + source - range_begin;
    if (source >= edit_at + removed) offset -= removed;
    if (source >= edit_at) offset += inserted;
    return offset;
}

typedef enum ActionRewriteKind {
    ActionRewrite_Insert,
    ActionRewrite_Delete,
    ActionRewrite_Localize,
} ActionRewriteKind;

static bool rewrite_action_command(NativeFighterProject* project,
                                   unsigned action, u32 command_offset,
                                   ActionRewriteKind rewrite,
                                   unsigned template_action,
                                   u32 template_offset,
                                   uint32_t* inserted_offset)
{
    const bool removing = rewrite == ActionRewrite_Delete;
    const bool inserting = rewrite == ActionRewrite_Insert;
    HSD_Dat dat;
    u32 table, entry;
    if ((!removing && inserted_offset == NULL) ||
        !action_table(project, &dat, &table) ||
        !action_source(project, &dat, table, action, &entry) ||
        dat.extern_count != 0) return false;

    NativeFighterActionCommand commands[1024];
    size_t old_command_count;
    if (!nativeFighterAction_Commands(project, action, commands, 1024,
                                      &old_command_count) ||
        old_command_count == 0) return false;
    const u32 range_begin = commands[0].source_offset;
    const NativeFighterActionCommand* final =
        &commands[old_command_count - 1];
    const u32 range_end = final->source_offset + final->words * 4;
    if (range_begin >= range_end || range_end > table) return false;
    u32 selected_width = 0, edit_at = 0;
    u32 copied_relocation_count = 0;
    for (size_t i = 0; i < old_command_count; ++i) {
        const NativeFighterActionCommand* command = &commands[i];
        if (command->source_offset < range_begin ||
            command->source_offset + command->words * 4 > range_end)
            return false;
        if (command->source_offset == command_offset &&
            (rewrite == ActionRewrite_Localize ||
             (command->opcode != 0 && command->opcode != 5 &&
              command->opcode != 6 && command->opcode != 7))) {
            selected_width = command->words;
            edit_at = removing ? command_offset :
                inserting ? command_offset + selected_width * 4 :
                            command_offset;
        }
        if (command->opcode == 5 || command->opcode == 7) {
            u32 target;
            const char* external;
            HSD_DatReferenceKind kind = HSD_DatResolve(
                &dat, command->source_offset + 4, &target, &external);
            if (kind == HSD_DAT_REF_LOCAL && target >= range_begin &&
                target < range_end) { /* This branch can stay action-local. */ }
            else if (kind != HSD_DAT_REF_NULL) return false;
        }
    }
    if (selected_width == 0 ||
        (rewrite != ActionRewrite_Localize &&
         edit_at + (removing ? selected_width * 4 : 0) >= range_end))
        return false;
    u32 template_width = 0, template_opcode = 0;
    if (inserting) {
        NativeFighterActionCommand template_commands[1024];
        size_t template_count;
        if (!nativeFighterAction_Commands(project, template_action,
                                          template_commands, 1024,
                                          &template_count)) return false;
        for (size_t i = 0; i < template_count; ++i) {
            if (template_commands[i].source_offset != template_offset) continue;
            template_opcode = template_commands[i].opcode;
            if (template_opcode != 0 &&
                (template_opcode < 3 || template_opcode > 9))
                template_width = template_commands[i].words;
            break;
        }
        if (template_width == 0) return false;
    }
    unsigned free_hitbox_id = 8;
    if (inserting && template_opcode == 11) {
        NativeFighterActionEvent listed[256];
        size_t listed_count;
        bool occupied[8] = { false };
        if (!nativeFighterAction_Events(project, action, listed, 256,
                                        &listed_count)) return false;
        for (size_t i = 0; i < listed_count; ++i)
            if (listed[i].kind == NativeFighterActionEvent_CreateHitbox &&
                listed[i].target_id < 8)
                occupied[listed[i].target_id] = true;
        for (unsigned id = 0; id < 8; ++id)
            if (!occupied[id]) { free_hitbox_id = id; break; }
        if (free_hitbox_id == 8) return false;
    }
    const u32 inserted = template_width * 4;
    const u32 removed = removing ? selected_width * 4 : 0;
    for (u32 i = 0; i < dat.relocation_count; ++i) {
        u32 slot;
        if (!HSD_DatRelocation(&dat, i, &slot)) return false;
        if (slot >= range_begin && slot < range_end) {
            if (removing && slot >= edit_at && slot < edit_at + removed)
                continue;
            ++copied_relocation_count;
        }
        if (inserted != 0 && slot >= template_offset &&
            slot < template_offset + inserted) return false;
    }
    const u32 new_length = range_end - range_begin + inserted - removed;
    if (new_length > UINT32_MAX - dat.data_size ||
        copied_relocation_count > (UINT32_MAX - new_length) / 4 ||
        project->fighter_size > UINT32_MAX - new_length -
            copied_relocation_count * 4 ||
        dat.relocation_count > UINT32_MAX / 4 -
            copied_relocation_count)
        return false;
    const size_t new_size = project->fighter_size + new_length +
                            copied_relocation_count * 4;
    u8* bytes = malloc(new_size);
    u32* slots = malloc((dat.relocation_count + copied_relocation_count) *
                        sizeof(*slots));
    if (bytes == NULL || slots == NULL) {
        free(slots);
        free(bytes);
        return false;
    }
    const u8* source = project->fighter_dat;
    memcpy(bytes, source, 32 + table);
    memcpy(bytes + 32 + table, source + 32 + range_begin,
           edit_at - range_begin);
    if (inserted != 0)
        memcpy(bytes + 32 + table + edit_at - range_begin,
               source + 32 + template_offset, inserted);
    memcpy(bytes + 32 + table + edit_at - range_begin + inserted,
           source + 32 + edit_at + removed,
           range_end - edit_at - removed);
    memcpy(bytes + 32 + table + new_length,
           source + 32 + table, dat.data_size - table);
    memcpy(bytes + 32 + dat.data_size + new_length +
               (dat.relocation_count + copied_relocation_count) * 4,
           source + 32 + dat.data_size + dat.relocation_count * 4,
           project->fighter_size - 32 - dat.data_size -
               dat.relocation_count * 4);
    write_be32(bytes, (u32) new_size);
    write_be32(bytes + 4, dat.data_size + new_length);
    write_be32(bytes + 8, dat.relocation_count + copied_relocation_count);
    for (u32 i = 0; i < dat.relocation_count; ++i) {
        u32 slot, target;
        if (!HSD_DatRelocation(&dat, i, &slot) ||
            !HSD_DatReadU32(&dat, slot, &target)) {
            free(slots);
            free(bytes);
            return false;
        }
        u32 moved_slot = slot >= table ? slot + new_length : slot;
        u32 moved_target = target >= table ? target + new_length : target;
        write_be32(bytes + 32 + moved_slot, moved_target);
        slots[i] = moved_slot;
    }
    u32 next_slot = dat.relocation_count;
    for (u32 i = 0; i < dat.relocation_count; ++i) {
        u32 source_slot, target;
        if (!HSD_DatRelocation(&dat, i, &source_slot) ||
            source_slot < range_begin || source_slot >= range_end ||
            (removing && source_slot >= edit_at &&
             source_slot < edit_at + removed)) continue;
        if (!HSD_DatReadU32(&dat, source_slot, &target)) {
            free(slots);
            free(bytes);
            return false;
        }
        if (removing && target >= edit_at && target < edit_at + removed) {
            free(slots);
            free(bytes);
            return false;
        }
        const u32 slot = copied_offset(source_slot,
            range_begin, table, edit_at, inserted, removed);
        const u32 mapped_target = target >= range_begin && target < range_end
            ? copied_offset(target, range_begin, table, edit_at,
                            inserted, removed)
            : target >= table ? target + new_length : target;
        write_be32(bytes + 32 + slot, mapped_target);
        slots[next_slot++] = slot;
    }
    if (next_slot != dat.relocation_count + copied_relocation_count) {
        free(slots);
        free(bytes);
        return false;
    }
    qsort(slots, next_slot, sizeof(*slots), compare_u32);
    for (u32 i = 0; i < next_slot; ++i)
        write_be32(bytes + 32 + dat.data_size + new_length + i * 4,
                   slots[i]);
    free(slots);
    for (u32 i = 0; i < dat.root_count; ++i) {
        u32 root;
        const char* name;
        if (!HSD_DatSymbol(&dat, false, i, &root, &name)) {
            free(bytes);
            return false;
        }
        (void) name;
        if (root >= table) root += new_length;
        write_be32(bytes + 32 + dat.data_size + new_length +
                   (dat.relocation_count + copied_relocation_count) * 4 +
                   i * 8, root);
    }
    const u32 new_table = table + new_length;
    const u32 new_command = table + edit_at - range_begin;
    if (template_opcode == 11) {
        u32 raw = ((u32) bytes[32 + new_command] << 24) |
                  ((u32) bytes[32 + new_command + 1] << 16) |
                  ((u32) bytes[32 + new_command + 2] << 8) |
                  bytes[32 + new_command + 3];
        raw = (raw & ~(7u << 23)) | ((u32) free_hitbox_id << 23);
        write_be32(bytes + 32 + new_command, raw);
    }
    write_be32(bytes + 32 + new_table + action * 24 + 12,
               copied_offset(entry, range_begin, table, edit_at,
                             inserted, removed));
    NativeFighterProject candidate = *project;
    candidate.fighter_dat = bytes;
    candidate.fighter_size = new_size;
    HSD_Dat check;
    size_t count;
    const bool valid = HSD_DatOpen(&check, bytes, new_size) == HSD_DAT_OK &&
        nativeFighterAction_Commands(&candidate, action, NULL, 0, &count) &&
        count == (removing ? old_command_count - 1 :
                  inserting ? old_command_count + 1 : old_command_count);
    if (!valid) {
        free(bytes);
        return false;
    }
    free(project->fighter_dat);
    project->fighter_dat = bytes;
    project->fighter_size = new_size;
    if (inserted_offset != NULL) *inserted_offset = new_command;
    return true;
}

bool nativeFighterAction_InsertCommand(NativeFighterProject* project,
                                       unsigned action, uint32_t after_offset,
                                       unsigned template_action,
                                       uint32_t template_offset,
                                       uint32_t* inserted_offset)
{
    return rewrite_action_command(project, action, after_offset,
                                  ActionRewrite_Insert,
                                  template_action, template_offset,
                                  inserted_offset);
}

bool nativeFighterAction_DeleteCommand(NativeFighterProject* project,
                                       unsigned action, uint32_t source_offset)
{
    return rewrite_action_command(project, action, source_offset,
                                  ActionRewrite_Delete,
                                  0, 0, NULL);
}

bool nativeFighterAction_LocalizeCommand(NativeFighterProject* project,
                                         unsigned action,
                                         uint32_t source_offset,
                                         uint32_t* local_offset)
{
    return rewrite_action_command(project, action, source_offset,
                                  ActionRewrite_Localize, 0, 0,
                                  local_offset);
}

bool nativeFighterAction_SetValue(NativeFighterProject* project,
                                  unsigned action, uint32_t source_offset,
                                  uint32_t value)
{
    NativeFighterActionEvent events[256];
    size_t count;
    if (project == NULL || !nativeFighterAction_Events(
                               project, action, events, 256, &count)) {
        return false;
    }
    for (size_t i = 0; i < count; ++i) {
        const NativeFighterActionEvent* event = &events[i];
        if (event->source_offset != source_offset) continue;
        if (event->kind == NativeFighterActionEvent_AllowInterrupt ||
            event->kind == NativeFighterActionEvent_ReleaseThrow ||
            event->kind == NativeFighterActionEvent_ReverseFacing ||
            event->kind == NativeFighterActionEvent_NeutralCue ||
            event->kind == NativeFighterActionEvent_ClearHitbox ||
            event->kind == NativeFighterActionEvent_ClearAllHitboxes ||
            event->kind == NativeFighterActionEvent_SoundEffect ||
            event->kind == NativeFighterActionEvent_SpawnEffect) return false;
        if (event->value == value) return true;
        unsigned users;
        if (!nativeFighterAction_Users(project, source_offset, &users) ||
            users != 1) {
            // Shared source commands need an explicit make-local copy first.
            return false;
        }
        u32 mask = event->kind == NativeFighterActionEvent_CreateHitbox
                       ? 0x3FF
                   : event->kind == NativeFighterActionEvent_SetHitboxDamage ||
                     event->kind == NativeFighterActionEvent_SetHitboxScale ||
                     event->kind == NativeFighterActionEvent_ThrowHitbox
                       ? 0x7FFFFF : 0x3FFFFFF;
        if (event->kind == NativeFighterActionEvent_SetHitboxFlag) {
            if (event->target_id > 7) return false;
            mask = 3;
        }
        if (event->kind == NativeFighterActionEvent_SetCommandVar) {
            if (event->target_id > 3) return false;
            mask = 0xFFFFFF;
        }
        if (event->kind == NativeFighterActionEvent_SetBodyCollisionState ||
            event->kind == NativeFighterActionEvent_SetAirborneState ||
            event->kind == NativeFighterActionEvent_SetGlobalHurtState ||
            event->kind == NativeFighterActionEvent_SetBoneHurtState) {
            if (value > 2) return false;
            mask = event->kind == NativeFighterActionEvent_SetBoneHurtState
                ? 0x3FFFF : 0x3FFFFFF;
        }
        if (value > mask) return false;
        HSD_Dat dat;
        if (HSD_DatOpen(&dat, project->fighter_dat,
                        project->fighter_size) != HSD_DAT_OK) {
            return false;
        }
        u32 raw;
        if (!HSD_DatReadU32(&dat, source_offset, &raw)) return false;
        raw = (raw & ~mask) | value;
        u8* bytes = project->fighter_dat + 32 + source_offset;
        bytes[0] = (u8) (raw >> 24);
        bytes[1] = (u8) (raw >> 16);
        bytes[2] = (u8) (raw >> 8);
        bytes[3] = (u8) raw;
        return true;
    }
    return false;
}

bool nativeFighterAction_Users(const NativeFighterProject* project,
                               uint32_t source_offset, unsigned* count)
{
    if (count == NULL) return false;
    *count = 0;
    for (unsigned action = 0;
         action < project->action_count; ++action) {
        NativeFighterActionEvent events[256];
        size_t event_count;
        if (!nativeFighterAction_Events(project, action, events, 256,
                                        &event_count)) {
            return false;
        }
        for (size_t i = 0; i < event_count; ++i) {
            if (events[i].source_offset == source_offset) {
                ++*count;
                break;
            }
        }
    }
    return true;
}

static bool hitbox_source(const NativeFighterProject* project, unsigned action,
                          uint32_t source_offset, HSD_Dat* dat)
{
    NativeFighterActionEvent events[256];
    size_t count;
    bool reachable = false;
    if (!nativeFighterAction_Events(project, action, events, 256, &count)) {
        return false;
    }
    for (size_t i = 0; i < count; ++i) {
        if (events[i].source_offset == source_offset &&
            events[i].kind == NativeFighterActionEvent_CreateHitbox) {
            reachable = true;
            break;
        }
    }
    return reachable &&
           HSD_DatOpen(dat, project->fighter_dat,
                       project->fighter_size) == HSD_DAT_OK &&
           HSD_DatBytes(dat, source_offset, 20) != NULL;
}

bool nativeFighterAction_GetHitbox(const NativeFighterProject* project,
                                   unsigned action, uint32_t source_offset,
                                   NativeFighterHitbox* hitbox)
{
    HSD_Dat dat;
    u32 w[5];
    if (hitbox == NULL || !hitbox_source(project, action, source_offset, &dat)) {
        return false;
    }
    for (unsigned i = 0; i < 5; ++i) {
        if (!HSD_DatReadU32(&dat, source_offset + i * 4, &w[i])) return false;
    }
    *hitbox = (NativeFighterHitbox) {
        .id = (w[0] >> 23) & 7,
        .hit_group = (w[0] >> 20) & 7,
        .only_hit_grabbed = (w[0] >> 19) & 1,
        .bone = (w[0] >> 11) & 0xFF,
        .use_common_bone_ids = (w[0] >> 10) & 1,
        .damage = w[0] & 0x3FF,
        .radius = w[1] >> 16,
        .offset_z = (int16_t) w[1],
        .offset_y = (int16_t) (w[2] >> 16),
        .offset_x = (int16_t) w[2],
        .angle = (w[3] >> 23) & 0x1FF,
        .knockback_growth = (w[3] >> 14) & 0x1FF,
        .weight_set_knockback = (w[3] >> 5) & 0x1FF,
        .item_hit_interaction = (w[3] >> 4) & 1,
        .ignore_thrown_fighters = (w[3] >> 3) & 1,
        .ignore_fighter_scale = (w[3] >> 2) & 1,
        .clank = (w[3] >> 1) & 1,
        .rebound = w[3] & 1,
        .base_knockback = (w[4] >> 23) & 0x1FF,
        .element = (w[4] >> 18) & 0x1F,
        .shield_damage = (int8_t) (w[4] >> 10),
        .hit_sfx_severity = (w[4] >> 7) & 7,
        .hit_sfx_kind = (w[4] >> 2) & 0x1F,
        .hit_grounded = (w[4] >> 1) & 1,
        .hit_aerial = w[4] & 1,
    };
    return true;
}

static bool valid_hitbox(const NativeFighterHitbox* h)
{
    return h->id <= 7 && h->hit_group <= 7 &&
           (h->use_common_bone_ids ? h->bone < FT_COMMON_PART_COUNT
                                   : h->bone < NATIVE_FIGHTER_MARIO_JOINT_COUNT) &&
           h->only_hit_grabbed <= 1 && h->use_common_bone_ids <= 1 &&
           h->damage <= 1023 && h->angle <= 511 &&
           h->knockback_growth <= 511 && h->weight_set_knockback <= 511 &&
           h->item_hit_interaction <= 1 &&
           h->ignore_thrown_fighters <= 1 &&
           h->ignore_fighter_scale <= 1 && h->clank <= 1 &&
           h->rebound <= 1 && h->base_knockback <= 511 &&
           h->element <= 31 && h->hit_sfx_severity <= 7 &&
           h->hit_sfx_kind <= 31 && h->hit_grounded <= 1 &&
           h->hit_aerial <= 1;
}

bool nativeFighterAction_SetHitbox(NativeFighterProject* project,
                                   unsigned action, uint32_t source_offset,
                                   const NativeFighterHitbox* h)
{
    HSD_Dat dat;
    u32 original[5];
    if (h == NULL || !valid_hitbox(h) ||
        !hitbox_source(project, action, source_offset, &dat)) return false;
    for (unsigned i = 0; i < 5; ++i) {
        if (!HSD_DatReadU32(&dat, source_offset + i * 4, &original[i])) {
            return false;
        }
    }
    if (h->id != ((original[0] >> 23) & 7)) {
        // Other commands may refer to this ID; remapping is a separate edit.
        return false;
    }
    u32 next[5] = {
        (11u << 26) | ((u32) h->id << 23) |
            ((u32) h->hit_group << 20) |
            ((u32) h->only_hit_grabbed << 19) |
            ((u32) h->bone << 11) |
            ((u32) h->use_common_bone_ids << 10) | h->damage,
        ((u32) h->radius << 16) | (u16) h->offset_z,
        ((u32) (u16) h->offset_y << 16) | (u16) h->offset_x,
        ((u32) h->angle << 23) |
            ((u32) h->knockback_growth << 14) |
            ((u32) h->weight_set_knockback << 5) |
            ((u32) h->item_hit_interaction << 4) |
            ((u32) h->ignore_thrown_fighters << 3) |
            ((u32) h->ignore_fighter_scale << 2) |
            ((u32) h->clank << 1) | h->rebound,
        ((u32) h->base_knockback << 23) |
            ((u32) h->element << 18) |
            ((u32) (u8) h->shield_damage << 10) |
            ((u32) h->hit_sfx_severity << 7) |
            ((u32) h->hit_sfx_kind << 2) |
            ((u32) h->hit_grounded << 1) | h->hit_aerial,
    };
    if (memcmp(original, next, sizeof(next)) == 0) return true;
    unsigned users;
    if (!nativeFighterAction_Users(project, source_offset, &users) ||
        users != 1) return false;
    u8* bytes = project->fighter_dat + 32 + source_offset;
    for (unsigned i = 0; i < 5; ++i) {
        u32 word = next[i];
        bytes[i * 4] = (u8) (word >> 24);
        bytes[i * 4 + 1] = (u8) (word >> 16);
        bytes[i * 4 + 2] = (u8) (word >> 8);
        bytes[i * 4 + 3] = (u8) word;
    }
    return true;
}

bool nativeFighterAction_GetSoundEffect(const NativeFighterProject* project,
                                        unsigned action, uint32_t source_offset,
                                        NativeFighterSoundEffect* sound)
{
    if (sound == NULL) return false;
    NativeFighterActionEvent events[256];
    size_t count;
    if (!nativeFighterAction_Events(project, action, events, 256, &count))
        return false;
    bool found = false;
    for (size_t i = 0; i < count; ++i) {
        if (events[i].source_offset == source_offset &&
            events[i].kind == NativeFighterActionEvent_SoundEffect) {
            found = true;
            break;
        }
    }
    HSD_Dat dat;
    u32 first, id, tail;
    if (!found ||
        HSD_DatOpen(&dat, project->fighter_dat,
                    project->fighter_size) != HSD_DAT_OK ||
        !HSD_DatReadU32(&dat, source_offset, &first) ||
        !HSD_DatReadU32(&dat, source_offset + 4, &id) ||
        !HSD_DatReadU32(&dat, source_offset + 8, &tail) ||
        first >> 26 != 17) return false;
    *sound = (NativeFighterSoundEffect){
        .behavior = (first >> 18) & 0xFF,
        .sfx_id = id,
        .volume = (tail >> 8) & 0xFF,
        .panning = tail & 0xFF,
    };
    return true;
}

bool nativeFighterAction_SetSoundEffect(NativeFighterProject* project,
                                        unsigned action, uint32_t source_offset,
                                        const NativeFighterSoundEffect* sound)
{
    NativeFighterSoundEffect old;
    if (sound == NULL || !nativeFighterAction_GetSoundEffect(
                             project, action, source_offset, &old) ||
        sound->behavior != old.behavior) return false;
    if (sound->sfx_id == old.sfx_id && sound->volume == old.volume &&
        sound->panning == old.panning) return true;
    if (old.behavior > 6) return false;
    unsigned users;
    if (!nativeFighterAction_Users(project, source_offset, &users) ||
        users != 1) return false;
    u8* id = project->fighter_dat + 32 + source_offset + 4;
    id[0] = (u8) (sound->sfx_id >> 24);
    id[1] = (u8) (sound->sfx_id >> 16);
    id[2] = (u8) (sound->sfx_id >> 8);
    id[3] = (u8) sound->sfx_id;
    u8* tail = id + 4;
    tail[2] = sound->volume;
    tail[3] = sound->panning;
    return true;
}

bool nativeFighterAction_GetSpawnEffect(const NativeFighterProject* project,
                                        unsigned action, uint32_t source_offset,
                                        NativeFighterSpawnEffect* effect)
{
    if (effect == NULL) return false;
    NativeFighterActionEvent events[256];
    size_t count;
    if (!nativeFighterAction_Events(project, action, events, 256, &count))
        return false;
    bool found = false;
    for (size_t i = 0; i < count; ++i) {
        if (events[i].source_offset == source_offset &&
            events[i].kind == NativeFighterActionEvent_SpawnEffect) {
            found = true;
            break;
        }
    }
    HSD_Dat dat;
    u32 words[5];
    if (!found ||
        HSD_DatOpen(&dat, project->fighter_dat,
                    project->fighter_size) != HSD_DAT_OK)
        return false;
    for (unsigned i = 0; i < 5; ++i) {
        if (!HSD_DatReadU32(&dat, source_offset + i * 4, &words[i]))
            return false;
    }
    if (words[0] >> 26 != 10) return false;
    *effect = (NativeFighterSpawnEffect){
        .bone = (words[0] >> 18) & 0xFF,
        .use_common_bone_ids = (words[0] >> 17) & 1,
        .destroy_on_state_change = (words[0] >> 16) & 1,
        .use_unknown_bone = (words[0] >> 15) & 1,
        .gfx_id = words[1] >> 16,
        .offset_z = (int16_t) (words[2] >> 16),
        .offset_y = (int16_t) words[2],
        .offset_x = (int16_t) (words[3] >> 16),
        .range_z = (uint16_t) words[3],
        .range_y = words[4] >> 16,
        .range_x = (uint16_t) words[4],
    };
    return true;
}

bool nativeFighterAction_SetSpawnEffect(NativeFighterProject* project,
                                        unsigned action, uint32_t source_offset,
                                        const NativeFighterSpawnEffect* effect)
{
    NativeFighterSpawnEffect old;
    if (effect == NULL || effect->use_common_bone_ids > 1 ||
        effect->destroy_on_state_change > 1 ||
        effect->use_unknown_bone > 1 ||
        (!effect->use_unknown_bone &&
         (effect->use_common_bone_ids
              ? effect->bone >= FT_COMMON_PART_COUNT
              : effect->bone >= NATIVE_FIGHTER_MARIO_JOINT_COUNT)) ||
        !nativeFighterAction_GetSpawnEffect(project, action, source_offset,
                                            &old)) return false;
    HSD_Dat dat;
    u32 original[5];
    if (HSD_DatOpen(&dat, project->fighter_dat,
                    project->fighter_size) != HSD_DAT_OK) return false;
    for (unsigned i = 0; i < 5; ++i) {
        if (!HSD_DatReadU32(&dat, source_offset + i * 4, &original[i]))
            return false;
    }
    u32 next[5] = {
        (original[0] & 0x7FFF) | (10u << 26) |
            ((u32) effect->bone << 18) |
            ((u32) effect->use_common_bone_ids << 17) |
            ((u32) effect->destroy_on_state_change << 16) |
            ((u32) effect->use_unknown_bone << 15),
        ((u32) effect->gfx_id << 16) | (original[1] & 0xFFFF),
        ((u32) (u16) effect->offset_z << 16) | (u16) effect->offset_y,
        ((u32) (u16) effect->offset_x << 16) | effect->range_z,
        ((u32) effect->range_y << 16) | effect->range_x,
    };
    if (memcmp(original, next, sizeof(next)) == 0) return true;
    unsigned users;
    if (!nativeFighterAction_Users(project, source_offset, &users) ||
        users != 1) return false;
    u8* bytes = project->fighter_dat + 32 + source_offset;
    for (unsigned i = 0; i < 5; ++i) {
        u32 word = next[i];
        bytes[i * 4] = (u8) (word >> 24);
        bytes[i * 4 + 1] = (u8) (word >> 16);
        bytes[i * 4 + 2] = (u8) (word >> 8);
        bytes[i * 4 + 3] = (u8) word;
    }
    return true;
}

#endif
