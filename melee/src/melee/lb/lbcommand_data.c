#include "lbcommand_data.h"
#include "types.h"

#include <sysdolphin/baselib/dat_import_internal.h>

#include <stdlib.h>
#include <string.h>

enum {
    FLOW_STATE_LIMIT = 65536,
    FLOW_HASH_SIZE = 2 * FLOW_STATE_LIMIT,
    FLOW_PROBE_LIMIT = 64 * FLOW_STATE_LIMIT,
    COMMANDS_PER_BURST = 1024,
    BURST_ANALYSIS_LIMIT = 64 * FLOW_STATE_LIMIT,
    COMMAND_STACK_SIZE = sizeof(((CommandInfo*) 0)->event_return) /
                         sizeof(((CommandInfo*) 0)->event_return[0]),
};

static bool fail(HSD_DatImport* import, HSD_DatResult result, u32 offset)
{
    import->status = result;
    import->error_offset = offset;
    return false;
}

enum { SLOT_CALL = 1, SLOT_LOOP = 2, SLOT_COUNT = 3 };
typedef struct FlowState {
    const void* owner;
    u32 pc;
    u32 values[COMMAND_STACK_SIZE];
    u8 types[COMMAND_STACK_SIZE];
    u8 depth;
} FlowState;

enum FlowStep { STEP_NEXT, STEP_DELAY, STEP_SET_LOOP, STEP_END_LOOP };
typedef struct FlowEdges {
    u32 next;
    u32 repeat;
    u32 count;
    enum FlowStep step;
    bool starts_burst;
} FlowEdges;

typedef struct FlowWork {
    FlowState* states;
    FlowEdges* edges;
    u32* hash;
    u32 count;
    u32 probes_left;
} FlowWork;

static bool flow_add(HSD_DatImport* import, FlowWork* work,
                     const FlowState* state, u32* id)
{
    u32 hash = (state->pc / 4) * 16777619U + state->depth;
    for (u32 i = 0; i < state->depth; ++i)
        hash = (hash ^ (state->values[i] + state->types[i])) * 16777619U;
    for (u32 at = hash & (FLOW_HASH_SIZE - 1);;
         at = (at + 1) & (FLOW_HASH_SIZE - 1)) {
        if (work->probes_left == 0)
            return fail(import, HSD_DAT_CAPACITY, state->pc);
        --work->probes_left;
        u32 found = work->hash[at];
        if (found != 0) {
            const FlowState* old = &work->states[found - 1];
            if (old->pc == state->pc && old->depth == state->depth &&
                old->owner == state->owner &&
                memcmp(old->values, state->values,
                       state->depth * sizeof(state->values[0])) == 0 &&
                memcmp(old->types, state->types, state->depth) == 0) {
                *id = found;
                return true;
            }
            continue;
        }
        if (work->count == FLOW_STATE_LIMIT)
            return fail(import, HSD_DAT_CAPACITY, state->pc);
        work->states[work->count] = *state;
        *id = work->hash[at] = ++work->count;
        return true;
    }
}

// Check the authored spacing between waits. Animation loops can revisit the
// same absolute wait after a wrap. Playback bounds catch-up separately when a
// seek, high playback speed or absolute wait makes several groups due at once.
static bool check_burst(HSD_DatImport* import, const FlowWork* work, u32 id,
                        u32 loop_count, u32* analysis_left)
{
    for (unsigned length = 0; id != 0; ++length) {
        if (length == COMMANDS_PER_BURST || *analysis_left == 0)
            return fail(import, HSD_DAT_CAPACITY, work->states[id - 1].pc);
        --*analysis_left;
        const FlowEdges* edges = &work->edges[id - 1];
        if (edges->step == STEP_DELAY) break;
        if (edges->step == STEP_SET_LOOP) loop_count = edges->count;
        if (edges->step == STEP_END_LOOP && --loop_count != 0) {
            id = edges->repeat;
        } else {
            id = edges->next;
        }
    }
    return true;
}

static bool check_bursts(HSD_DatImport* import, const FlowWork* work)
{
    // A loop uses two of the three physical slots. At a wait inside a loop,
    // the next EndLoop can exit immediately or repeat the same body. Trying
    // the last and first iteration bounds every intermediate iteration too.
    _Static_assert(COMMAND_STACK_SIZE < 4, "revisit simultaneous loop counters");
    u32 analysis_left = BURST_ANALYSIS_LIMIT;
    for (u32 i = 0; i < work->count; ++i) {
        if (!work->edges[i].starts_burst) continue;
        const FlowState* state = &work->states[i];
        if (!check_burst(import, work, i + 1, 1, &analysis_left)) return false;
        for (u32 j = 0; j < state->depth; ++j) {
            if (state->types[j] == SLOT_COUNT && state->values[j] != 1 &&
                !check_burst(import, work, i + 1, state->values[j],
                             &analysis_left))
                return false;
        }
    }
    return true;
}

static bool validate_flow(HSD_DatImport* import, const u8* widths,
                          u32 word_count, const LbCommandEntry* entries,
                          u32 entry_count, bool animation_wait_yields,
                          LbCommandOperandCheck check, FlowWork* work)
{
    // Every action starts with an empty stack. Shared tails only need to be
    // checked again when their owner or stack changes.
    for (u32 i = 0; i < entry_count; ++i) {
        FlowState initial = { .pc = entries[i].source,
                              .owner = entries[i].owner };
        u32 id;
        if (!flow_add(import, work, &initial, &id)) return false;
        work->edges[id - 1].starts_burst = true;
    }
    for (u32 cursor = 0; cursor < work->count; ++cursor) {
        FlowState current = work->states[cursor];
        u32 at = current.pc, raw;
        if ((at & 3) != 0 || at / 4 >= word_count ||
            widths[at / 4] == 0 || widths[at / 4] == LB_COMMAND_OPERAND ||
            !HSD_DatReadU32(import->dat, at, &raw))
            return fail(import, HSD_DAT_DESCRIPTOR, at);
        if (check != NULL && !check(import, current.owner, at, raw))
            return false;
        u32 opcode = raw >> 26;
        FlowEdges* edges = &work->edges[cursor];
        current.pc += widths[at / 4] * 4;
        switch (opcode) {
        case 0: continue;
        case 1:
        case 2:
            if ((raw & 0x3FFFFFF) != 0) edges->step = STEP_DELAY;
            break;
        case 8:
            if (animation_wait_yields) edges->step = STEP_DELAY;
            break;
        case 3:
            if (current.depth + 2 > COMMAND_STACK_SIZE)
                return fail(import, HSD_DAT_DESCRIPTOR, at);
            current.values[current.depth] = current.pc;
            current.types[current.depth++] = SLOT_LOOP;
            edges->step = STEP_SET_LOOP;
            edges->count = raw & 0x3FFFFFF;
            current.values[current.depth] = edges->count;
            current.types[current.depth++] = SLOT_COUNT;
            break;
        case 4:
            if (current.depth < 2 ||
                current.types[current.depth - 1] != SLOT_COUNT ||
                current.types[current.depth - 2] != SLOT_LOOP)
                return fail(import, HSD_DAT_DESCRIPTOR, at);
            edges->step = STEP_END_LOOP;
            if (current.values[current.depth - 1] != 1) {
                FlowState repeat = current;
                repeat.pc = current.values[current.depth - 2];
                if (!flow_add(import, work, &repeat, &edges->repeat))
                    return false;
            }
            current.depth -= 2;
            break;
        case 5:
            if (current.depth == COMMAND_STACK_SIZE)
                return fail(import, HSD_DAT_DESCRIPTOR, at);
            current.values[current.depth] = current.pc;
            current.types[current.depth++] = SLOT_CALL;
            // Fall through: Call and Goto both consume a relocation operand.
        case 7: {
            const char* external;
            HSD_DatReferenceKind kind = HSD_DatResolve(
                import->dat, at + 4, &current.pc, &external);
            if (kind == HSD_DAT_REF_NULL) continue;
            if (kind != HSD_DAT_REF_LOCAL)
                return fail(import, HSD_DAT_RELOCATION, at + 4);
            break;
        }
        case 6:
            if (current.depth == 0 ||
                current.types[current.depth - 1] != SLOT_CALL)
                return fail(import, HSD_DAT_DESCRIPTOR, at);
            current.pc = current.values[--current.depth];
            break;
        default: break;
        }
        if (!flow_add(import, work, &current, &edges->next)) return false;
        if (edges->step == STEP_DELAY)
            work->edges[edges->next - 1].starts_burst = true;
    }
    return check_bursts(import, work);
}

bool lbCommandData_ValidateFlow(HSD_DatImport* import, const u8* widths,
                                u32 word_count, const LbCommandEntry* entries,
                                u32 entry_count, bool animation_wait_yields,
                                LbCommandOperandCheck check)
{
    if (entry_count == 0) return true;
    FlowWork work = { .probes_left = FLOW_PROBE_LIMIT };
    work.states = malloc(FLOW_STATE_LIMIT * sizeof(*work.states));
    work.edges = calloc(FLOW_STATE_LIMIT, sizeof(*work.edges));
    work.hash = calloc(FLOW_HASH_SIZE, sizeof(*work.hash));
    bool valid;
    if (work.states == NULL || work.edges == NULL || work.hash == NULL) {
        valid = fail(import, HSD_DAT_CAPACITY, entries[0].source);
    } else {
        valid = validate_flow(import, widths, word_count, entries, entry_count,
                              animation_wait_yields, check, &work);
    }
    free(work.hash);
    free(work.edges);
    free(work.states);
    return valid;
}
