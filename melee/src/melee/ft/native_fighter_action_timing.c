#include "native_fighter_action_internal.h"

#ifdef MELEE_NATIVE
#include <stdlib.h>
#include <string.h>

#include "ftaction_data.h"
#include "native_hitbox_lifetime.h"

enum {
    TIMING_NODES = 2048,
    TIMING_OCCURRENCES = 4096,
    TIMING_BYTES = 65536
};
typedef struct TimingNode {
    u32 source, output, repeat;
    int parent, frame, end;
    bool loop, omit, refresh_known;
    u32 lifetime;
} TimingNode;
typedef struct TimingOrder {
    int parent, frame, node;
} TimingOrder;
typedef struct TimingPlan {
    HSD_Dat dat;
    NativeFighterActionOccurrence occurrences[TIMING_OCCURRENCES];
    TimingNode nodes[TIMING_NODES];
    TimingOrder order[TIMING_NODES];
    int first[TIMING_NODES], children[TIMING_NODES];
    u8 code[TIMING_BYTES];
    size_t count, cursor, node_count;
    u32 bytes, selected, moved;
    int delta;
    uint32_t duration_edit;
    bool found, converting;
} TimingPlan;

static void put_word(u8* out, u32 word)
{
    out[0] = word >> 24;
    out[1] = word >> 16;
    out[2] = word >> 8;
    out[3] = word;
}

// Parse the first iteration of each structured loop. Later iterations are
// checked against the compiled result before publishing; nonperiodic timing
// cannot silently become a different loop.
static bool parse_block(TimingPlan* plan, int parent, unsigned depth,
                        u32* close)
{
    if (depth > 16) {
        return false;
    }
    while (plan->cursor < plan->count) {
        const NativeFighterActionOccurrence current =
            plan->occurrences[plan->cursor++];
        if (current.opcode == 0 || current.opcode == 4) {
            plan->nodes[parent].end = current.frame;
            *close = current.source_offset;
            return current.opcode == (parent == 0 ? 0 : 4);
        }
        if (current.opcode != 3 && current.opcode < 9) {
            continue;
        }
        if (plan->node_count == TIMING_NODES) {
            return false;
        }
        const int index = (int) plan->node_count++;
        TimingNode* node = &plan->nodes[index];
        *node = (TimingNode){ .source = current.source_offset,
                              .parent = parent,
                              .frame = current.frame,
                              .loop = current.opcode == 3 };
        if (node->loop) {
            u32 raw, closing;
            if (!HSD_DatReadU32(&plan->dat, node->source, &raw)) {
                return false;
            }
            node->repeat = raw & 0x3ffffff;
            if (node->repeat == 0 || node->repeat > TIMING_OCCURRENCES ||
                !parse_block(plan, index, depth + 1, &closing))
            {
                return false;
            }
            const int duration = node->end - node->frame;
            if (duration < 0) {
                return false;
            }
            for (u32 iteration = 1; iteration < node->repeat; ++iteration) {
                while (plan->cursor < plan->count &&
                       !(plan->occurrences[plan->cursor].opcode == 4 &&
                         plan->occurrences[plan->cursor].source_offset ==
                             closing))
                {
                    ++plan->cursor;
                }
                if (plan->cursor == plan->count) {
                    return false;
                }
                ++plan->cursor;
            }
        } else if (node->source == plan->selected) {
            node->frame += plan->delta;
            plan->found = true;
        }
    }
    // A null call/goto terminates a script as well as an explicit End.
    if (parent != 0 || plan->count == 0) {
        return false;
    }
    plan->nodes[0].end = plan->occurrences[plan->count - 1].frame;
    return true;
}

static int compare_order(const void* a, const void* b)
{
    const TimingOrder* x = a;
    const TimingOrder* y = b;
    if (x->parent != y->parent) {
        return (x->parent > y->parent) - (x->parent < y->parent);
    }
    if (x->frame != y->frame) {
        return (x->frame > y->frame) - (x->frame < y->frame);
    }
    return (x->node > y->node) - (x->node < y->node);
}
static bool emit_word(TimingPlan* plan, u32 word)
{
    if (plan->bytes > TIMING_BYTES - 4) {
        return false;
    }
    put_word(plan->code + plan->bytes, word);
    plan->bytes += 4;
    return true;
}
static bool emit_block(TimingPlan* plan, int parent, unsigned depth)
{
    if (depth > 16) {
        return false;
    }
    int frame = plan->nodes[parent].frame;
    for (int i = 0; i < plan->children[parent]; ++i) {
        TimingNode* node =
            &plan->nodes[plan->order[plan->first[parent] + i].node];
        if (node->omit) {
            continue;
        }
        if (node->frame < frame || node->frame > 1000) {
            return false;
        }
        if (node->frame > frame &&
            !emit_word(plan, (1u << 26) | (node->frame - frame)))
        {
            return false;
        }
        node->output = plan->bytes;
        if (node->loop) {
            if (!emit_word(plan, (3u << 26) | node->repeat) ||
                !emit_block(plan, (int) (node - plan->nodes), depth + 1) ||
                !emit_word(plan, 4u << 26))
            {
                return false;
            }
            frame = node->frame + (node->end - node->frame) * node->repeat;
        } else {
            u32 raw;
            if (!HSD_DatReadU32(&plan->dat, node->source, &raw)) {
                return false;
            }
            const u32 size = ftActionData_SourceCommandWidth(raw) * 4;
            const u8* source = HSD_DatBytes(&plan->dat, node->source, size);
            if (!source || size > TIMING_BYTES - plan->bytes) {
                return false;
            }
            memcpy(plan->code + plan->bytes, source, size);
            plan->bytes += size;
            if (node->source == plan->selected && plan->duration_edit &&
                (raw >> 26) == NATIVE_HITBOX_TIMED_OPCODE)
            {
                u32 original_life;
                HSD_DatReadU32(&plan->dat, node->source + 20, &original_life);
                put_word(plan->code + node->output + 20,
                         (original_life & ~NATIVE_HITBOX_DURATION_MASK) |
                             plan->duration_edit);
            }
            if (node->lifetime) {
                put_word(plan->code + node->output,
                         (raw & 0x3ffffff) |
                             (NATIVE_HITBOX_TIMED_OPCODE << 26));
                if (!emit_word(plan, node->lifetime)) {
                    return false;
                }
            }
            if (node->source == plan->selected && plan->moved == UINT32_MAX) {
                plan->moved = node->output;
            }
            frame = node->frame;
        }
    }
    const int end = plan->nodes[parent].end;
    if (parent != 0 && frame > end) {
        return false;
    }
    return end <= frame || emit_word(plan, (1u << 26) | (end - frame));
}

static int compare_slot(const void* a, const void* b)
{
    const u32 x = *(const u32*) a, y = *(const u32*) b;
    return (x > y) - (x < y);
}

static bool publish(NativeFighterProject* project, unsigned action, u32 table,
                    TimingPlan* plan, uint32_t* moved)
{
    const HSD_Dat* dat = &plan->dat;
    u32 copied_slots = 0;
    for (size_t n = 1; n < plan->node_count; ++n) {
        const TimingNode* node = &plan->nodes[n];
        if (node->loop || node->omit) {
            continue;
        }
        u32 raw;
        HSD_DatReadU32(dat, node->source, &raw);
        const u32 size = ftActionData_SourceCommandWidth(raw) * 4;
        for (u32 r = 0; r < dat->relocation_count; ++r) {
            u32 slot;
            if (!HSD_DatRelocation(dat, r, &slot)) {
                return false;
            }
            if (slot >= node->source && slot < node->source + size) {
                ++copied_slots;
            }
        }
    }
    const size_t new_size =
        project->fighter_size + plan->bytes + (size_t) copied_slots * 4;
    if (new_size > UINT32_MAX) {
        return false;
    }
    u8* bytes = malloc(new_size);
    u32* slots =
        malloc((dat->relocation_count + copied_slots) * sizeof(*slots));
    if (!bytes || !slots) {
        free(bytes);
        free(slots);
        return false;
    }
    const u8* source = project->fighter_dat;
    const u32 new_data = dat->data_size + plan->bytes;
    const u32 relocations = dat->relocation_count + copied_slots;
    memcpy(bytes, source, 32 + table);
    memcpy(bytes + 32 + table, plan->code, plan->bytes);
    memcpy(bytes + 32 + table + plan->bytes, source + 32 + table,
           dat->data_size - table);
    memcpy(bytes + 32 + new_data + relocations * 4,
           source + 32 + dat->data_size + dat->relocation_count * 4,
           project->fighter_size - 32 - dat->data_size -
               dat->relocation_count * 4);
    put_word(bytes, (u32) new_size);
    put_word(bytes + 4, new_data);
    put_word(bytes + 8, relocations);
    u32 next = 0;
    for (u32 i = 0; i < dat->relocation_count; ++i) {
        u32 slot, target;
        HSD_DatRelocation(dat, i, &slot);
        HSD_DatReadU32(dat, slot, &target);
        const u32 dest = slot >= table ? slot + plan->bytes : slot;
        slots[next++] = dest;
        put_word(bytes + 32 + dest,
                 target >= table ? target + plan->bytes : target);
    }
    for (size_t n = 1; n < plan->node_count; ++n) {
        const TimingNode* node = &plan->nodes[n];
        if (node->loop || node->omit) {
            continue;
        }
        u32 raw;
        HSD_DatReadU32(dat, node->source, &raw);
        const u32 size = ftActionData_SourceCommandWidth(raw) * 4;
        for (u32 r = 0; r < dat->relocation_count; ++r) {
            u32 slot, target;
            HSD_DatRelocation(dat, r, &slot);
            if (slot < node->source || slot >= node->source + size) {
                continue;
            }
            HSD_DatReadU32(dat, slot, &target);
            const u32 dest = table + node->output + slot - node->source;
            slots[next++] = dest;
            put_word(bytes + 32 + dest,
                     target >= table ? target + plan->bytes : target);
        }
    }
    qsort(slots, next, sizeof(*slots), compare_slot);
    for (u32 i = 0; i < next; ++i) {
        put_word(bytes + 32 + new_data + i * 4, slots[i]);
    }
    free(slots);
    for (u32 i = 0; i < dat->root_count; ++i) {
        u32 root;
        const char* name;
        HSD_DatSymbol(dat, false, i, &root, &name);
        put_word(bytes + 32 + new_data + relocations * 4 + i * 8,
                 root >= table ? root + plan->bytes : root);
    }
    put_word(bytes + 32 + table + plan->bytes + action * 24 + 12, table);
    NativeFighterProject candidate = *project;
    candidate.fighter_dat = bytes;
    candidate.fighter_size = new_size;
    NativeFighterActionOccurrence actual[TIMING_OCCURRENCES];
    size_t count;
    bool complete;
    HSD_Dat check;
    bool valid =
        HSD_DatOpen(&check, bytes, new_size) == HSD_DAT_OK &&
        nativeFighterAction_Timeline(&candidate, action, actual,
                                     TIMING_OCCURRENCES, &count, &complete) &&
        complete;
    bool matched[TIMING_OCCURRENCES] = { false };
    if (valid) {
        for (size_t i = 0; i < count; ++i) {
            if (actual[i].opcode < 9) {
                continue;
            }
            size_t node = 1;
            while (
                node < plan->node_count &&
                (plan->nodes[node].omit ||
                 table + plan->nodes[node].output != actual[i].source_offset))
            {
                ++node;
            }
            if (node == plan->node_count) {
                valid = false;
                break;
            }
            const u32 old = plan->nodes[node].source;
            size_t j = 0;
            for (; j < plan->count; ++j) {
                const NativeFighterActionOccurrence* before =
                    &plan->occurrences[j];
                if (!matched[j] && before->source_offset == old &&
                    (before->opcode == actual[i].opcode ||
                     (plan->converting && before->opcode == 11 &&
                      actual[i].opcode == NATIVE_HITBOX_TIMED_OPCODE)) &&
                    (int) before->frame +
                            (old == plan->selected ? plan->delta : 0) ==
                        actual[i].frame)
                {
                    break;
                }
            }
            if (j == plan->count) {
                valid = false;
                break;
            }
            matched[j] = true;
        }
    }
    if (valid) {
        for (size_t i = 0; i < plan->count; ++i) {
            if (plan->occurrences[i].opcode >= 9 && !matched[i] &&
                !(plan->converting && (plan->occurrences[i].opcode == 15 ||
                                       plan->occurrences[i].opcode == 16)))
            {
                valid = false;
                break;
            }
        }
    }
    if (valid) {
        valid = nativeFighterAction_HitboxIntervalsValid(&candidate, action);
    }
    if (!valid) {
        free(bytes);
        return false;
    }
    free(project->fighter_dat);
    project->fighter_dat = bytes;
    project->fighter_size = new_size;
    *moved = plan->moved == UINT32_MAX ? 0 : table + plan->moved;
    return true;
}

bool nativeFighterAction_EditEventTiming(NativeFighterProject* project,
                                         unsigned action, uint32_t source,
                                         int delta, uint32_t duration256,
                                         uint32_t* moved)
{
    if (!moved || delta < -1000 || delta > 1000 || duration256 > 256000) {
        return false;
    }
    TimingPlan* plan = calloc(1, sizeof(*plan));
    if (!plan) {
        return false;
    }
    u32 table, closing;
    bool complete;
    plan->selected = source;
    plan->delta = delta;
    plan->duration_edit = duration256;
    plan->moved = UINT32_MAX;
    plan->node_count = 1;
    bool ok = nativeFighterAction_OpenTable(project, &plan->dat, &table) &&
              plan->dat.extern_count == 0 &&
              nativeFighterAction_Timeline(project, action, plan->occurrences,
                                           TIMING_OCCURRENCES, &plan->count,
                                           &complete) &&
              complete && parse_block(plan, 0, 0, &closing) && plan->found;
    if (ok && duration256) {
        u32 raw;
        ok = HSD_DatReadU32(&plan->dat, source, &raw) &&
             (raw >> 26) == NATIVE_HITBOX_TIMED_OPCODE;
    }
    if (ok) {
        for (size_t i = 1; i < plan->node_count; ++i) {
            const TimingNode* node = &plan->nodes[i];
            const TimingNode* parent = &plan->nodes[node->parent];
            if (node->frame < parent->frame ||
                (node->parent != 0 && node->frame > parent->end))
            {
                ok = false;
                break;
            }
            plan->order[i - 1] =
                (TimingOrder){ node->parent, node->frame, (int) i };
        }
    }
    if (ok) {
        qsort(plan->order, plan->node_count - 1, sizeof(*plan->order),
              compare_order);
        for (size_t i = 0; i < plan->node_count - 1; ++i) {
            const int parent = plan->order[i].parent;
            if (plan->children[parent]++ == 0) {
                plan->first[parent] = (int) i;
            }
        }
        ok = emit_block(plan, 0, 0) && emit_word(plan, 0) &&
             publish(project, action, table, plan, moved);
    }
    free(plan);
    return ok;
}

bool nativeFighterAction_HitboxIntervalsValid(
    const NativeFighterProject* project, unsigned action)
{
    NativeFighterActionOccurrence occurrences[TIMING_OCCURRENCES];
    size_t count;
    bool complete;
    if (!nativeFighterAction_Timeline(project, action, occurrences,
                                      TIMING_OCCURRENCES, &count, &complete))
    {
        return false;
    }
    HSD_Dat dat;
    u32 table;
    if (!nativeFighterAction_OpenTable(project, &dat, &table)) {
        return false;
    }
    uint32_t ends[4] = { 0 };
    for (size_t i = 0; i < count; ++i) {
        const NativeFighterActionOccurrence* occurrence = &occurrences[i];
        if (occurrence->opcode != 11 &&
            occurrence->opcode != NATIVE_HITBOX_TIMED_OPCODE &&
            occurrence->opcode != 15 && occurrence->opcode != 16)
        {
            continue;
        }
        u32 raw;
        HSD_DatReadU32(&dat, occurrence->source_offset, &raw);
        if (occurrence->opcode == 16) {
            memset(ends, 0, sizeof(ends));
            continue;
        }
        const unsigned slot =
            occurrence->opcode == 15 ? raw & 0x3ffffff : (raw >> 23) & 7;
        if (slot >= 4) {
            return false;
        }
        if (occurrence->opcode == 15) {
            ends[slot] = 0;
            continue;
        }
        if (ends[slot] > occurrence->frame * 256u) {
            return false;
        }
        ends[slot] = 0;
        if (occurrence->opcode == NATIVE_HITBOX_TIMED_OPCODE) {
            u32 life;
            if (!HSD_DatReadU32(&dat, occurrence->source_offset + 20, &life) ||
                !(life & NATIVE_HITBOX_DURATION_MASK) ||
                (life & NATIVE_HITBOX_DURATION_MASK) > 256000)
            {
                return false;
            }
            ends[slot] = occurrence->frame * 256u +
                         (life & NATIVE_HITBOX_DURATION_MASK);
        }
    }
    return true;
}

bool nativeFighterAction_ConvertHitboxIntervals(NativeFighterProject* project,
                                                unsigned action,
                                                unsigned* converted)
{
    if (!converted) {
        return false;
    }
    *converted = 0;
    NativeFighterActionCommand commands[1024];
    size_t command_count;
    if (!nativeFighterAction_Commands(project, action, commands, 1024,
                                      &command_count))
    {
        return false;
    }
    bool has_hitbox = false;
    for (size_t i = 0; i < command_count; ++i) {
        if (commands[i].opcode == NATIVE_HITBOX_TIMED_OPCODE) {
            return true;
        }
        has_hitbox |= commands[i].opcode == 11;
    }
    if (!has_hitbox) {
        return true;
    }
    TimingPlan* plan = calloc(1, sizeof(*plan));
    if (!plan) {
        return false;
    }
    u32 table, closing;
    bool complete;
    plan->node_count = 1;
    plan->selected = plan->moved = UINT32_MAX;
    plan->converting = true;
    bool proven = nativeFighterAction_OpenTable(project, &plan->dat, &table) &&
                  !plan->dat.extern_count &&
                  nativeFighterAction_Timeline(
                      project, action, plan->occurrences, TIMING_OCCURRENCES,
                      &plan->count, &complete) &&
                  complete && parse_block(plan, 0, 0, &closing);
    bool active[4] = { false }, seen[4] = { false };
    unsigned group[4] = { 0 };
    int started[4] = { 0 }, cleared[4] = { -1, -1, -1, -1 };
    for (size_t i = 0; proven && i < plan->count; ++i) {
        const NativeFighterActionOccurrence* occurrence =
            &plan->occurrences[i];
        u32 raw;
        HSD_DatReadU32(&plan->dat, occurrence->source_offset, &raw);
        const unsigned slot =
            occurrence->opcode == 11 ? (raw >> 23) & 7 : raw & 0x3ffffff;
        if (occurrence->opcode == 15 || occurrence->opcode == 16) {
            if (occurrence->opcode == 15) {
                if (slot >= 4 || !seen[slot]) {
                    proven = false;
                    break;
                }
                active[slot] = false;
                cleared[slot] = occurrence->frame;
            } else {
                if (!seen[0] && !seen[1] && !seen[2] && !seen[3]) {
                    proven = false;
                    break;
                }
                for (unsigned s = 0; s < 4; ++s) {
                    active[s] = false;
                    cleared[s] = occurrence->frame;
                }
            }
        }
        if (occurrence->opcode != 11) {
            continue;
        }
        u32 third;
        HSD_DatReadU32(&plan->dat, occurrence->source_offset + 12, &third);
        if (slot >= 4 || (third & 8)) {
            proven = false;
            break;
        } // Conditional thrown-owner spawn.
        const unsigned new_group = (raw >> 20) & 7;
        const bool must_refresh = (active[slot] && group[slot] != new_group) ||
                                  (!active[slot] && seen[slot] &&
                                   cleared[slot] == occurrence->frame);
        const bool refresh_known = active[slot] || must_refresh;
        // Stock creation may inherit another slot's older group history. A
        // fresh duration-owned slot cannot reproduce that through its
        // checkbox.
        if (!active[slot] || must_refresh) {
            for (unsigned s = 0; s < 4; ++s) {
                if (s != slot && active[s] && group[s] == new_group &&
                    started[s] < occurrence->frame)
                {
                    proven = false;
                }
            }
        }
        if (!proven) {
            break;
        }
        int end = -1;
        bool expire_all = false;
        for (size_t j = i + 1; j < plan->count; ++j) {
            const NativeFighterActionOccurrence* next = &plan->occurrences[j];
            u32 value;
            HSD_DatReadU32(&plan->dat, next->source_offset, &value);
            if (next->opcode == 16 ||
                (next->opcode == 15 && (value & 0x3ffffff) == slot) ||
                (next->opcode == 11 && ((value >> 23) & 7) == slot))
            {
                end = next->frame;
                expire_all = next->opcode == 16;
                break;
            }
        }
        if (end <= occurrence->frame) {
            proven = false;
            break;
        }
        const u32 duration = (end - occurrence->frame) * 256u;
        for (size_t n = 1; n < plan->node_count; ++n) {
            TimingNode* node = &plan->nodes[n];
            if (node->source != occurrence->source_offset) {
                continue;
            }
            if ((node->lifetime &&
                 (node->lifetime & NATIVE_HITBOX_DURATION_MASK) != duration) ||
                (refresh_known && node->refresh_known &&
                 ((node->lifetime & NATIVE_HITBOX_REFRESH) != 0) !=
                     must_refresh))
            {
                proven = false;
                break;
            }
            node->lifetime =
                (node->lifetime & ~NATIVE_HITBOX_DURATION_MASK) | duration;
            if (expire_all) {
                node->lifetime |= NATIVE_HITBOX_EXPIRE_ALL;
            }
            if (refresh_known) {
                node->refresh_known = true;
                if (must_refresh) {
                    node->lifetime |= NATIVE_HITBOX_REFRESH;
                } else {
                    node->lifetime &= ~NATIVE_HITBOX_REFRESH;
                }
            }
        }
        if (!active[slot] || must_refresh) {
            started[slot] = occurrence->frame;
        }
        active[slot] = seen[slot] = true;
        group[slot] = new_group;
    }
    if (!proven) {
        free(plan);
        return true;
    } // Preserve the original action intact.
    for (size_t n = 1; n < plan->node_count; ++n) {
        TimingNode* node = &plan->nodes[n];
        u32 raw;
        HSD_DatReadU32(&plan->dat, node->source, &raw);
        node->omit = (raw >> 26) == 15 || (raw >> 26) == 16;
        plan->order[n - 1] =
            (TimingOrder){ node->parent, node->frame, (int) n };
    }
    qsort(plan->order, plan->node_count - 1, sizeof(*plan->order),
          compare_order);
    for (size_t i = 0; i < plan->node_count - 1; ++i) {
        const int parent = plan->order[i].parent;
        if (plan->children[parent]++ == 0) {
            plan->first[parent] = (int) i;
        }
    }
    uint32_t moved;
    const bool ok = emit_block(plan, 0, 0) && emit_word(plan, 0) &&
                    publish(project, action, table, plan, &moved);
    if (ok) {
        for (size_t n = 1; n < plan->node_count; ++n) {
            if (plan->nodes[n].lifetime) {
                ++*converted;
            }
        }
    }
    free(plan);
    return ok;
}

void nativeFighterAction_ConvertAllHitboxIntervals(
    NativeFighterProject* project, unsigned* converted)
{
    unsigned total = 0;
    for (unsigned action = 0; action < project->action_count; ++action) {
        unsigned count;
        if (nativeFighterAction_ConvertHitboxIntervals(project, action,
                                                       &count))
        {
            total += count;
        }
    }
    if (converted) {
        *converted = total;
    }
}
#endif
