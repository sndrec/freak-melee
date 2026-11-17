#include "native_shell_workshop_practice.h"
#include "native_shell_workshop.h"
#include "native_shell_workshop_preview_agent.h"
#include <melee/gm/native_feedback.h>
#include <melee/gm/native_match_launch.h>
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_workshop_practice.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <optional>

using NativeAuthoring::Reply;
using NativeAuthoring::add_string;

namespace {
unsigned number(yyjson_val* object, const char* key, unsigned fallback = 0)
{
    auto* value = yyjson_obj_get(object, key);
    return value ? unsigned(yyjson_get_uint(value)) : fallback;
}
int integer(yyjson_val* object, const char* key, int fallback = 0)
{
    auto* value = yyjson_obj_get(object, key);
    return value ? int(yyjson_get_sint(value)) : fallback;
}
const char* string(yyjson_val* object, const char* key)
{
    const char* value = yyjson_get_str(yyjson_obj_get(object, key));
    return value ? value : "";
}
bool initial_seat(yyjson_val* value, unsigned port, NativeWorkshopPracticeSeat& seat, Reply& reply)
{
    seat.present = true;
    seat.spawn = integer(value, "spawn", port);
    seat.facing = integer(value, "facing", port < 2 ? 1 : -1);
    if (!seat.facing) {
        reply.fail("invalid_facing", "Set facing to -1 for left or 1 for right.");
        return false;
    }
    seat.damage = number(value, "damage");
    if (!port) return true;
    seat.cpu_level = number(value, "cpu_level");
    auto* fighter_id = yyjson_obj_get(value, "fighter_id");
    const char* package = string(value, "package_id");
    if (bool(fighter_id) == bool(*package)) {
        reply.fail("invalid_participant", "Choose one stock fighter ID or one cached package ID for each opponent.");
        return false;
    }
    if (*package) {
        const NativeContentId empty{};
        if (nativeContentStore_IdFromHex(package, &seat.package) &&
            std::memcmp(&seat.package, &empty, sizeof(empty))) return true;
    } else {
        seat.character = nativeWorkshopPractice_StockCharacter(unsigned(yyjson_get_uint(fighter_id)));
        if (seat.character >= 0) return true;
    }
    reply.fail("invalid_participant", "Choose a playable fighter or a nonzero package ID.");
    return false;
}

const char* status_name(NativeWorkshopPracticeStatus status)
{
    switch (status) {
    case NativeWorkshopPractice_Opening: return "opening";
    case NativeWorkshopPractice_Ready: return "ready";
    case NativeWorkshopPractice_Running: return "running";
    case NativeWorkshopPractice_CheckpointPending: return "checkpoint_pending";
    case NativeWorkshopPractice_Failed: return "failed";
    case NativeWorkshopPractice_Closed: return "closed";
    }
    return "closed";
}

void describe(const NativeWorkshopPracticeState& state, Reply& reply)
{
    add_string(reply, reply.result, "status", status_name(state.status));
    add_string(reply, reply.result, "fighter_revision", state.fighter_revision);
    char id[65];
    nativeContentStore_IdHex(&state.package, id);
    add_string(reply, reply.result, "package_id", id);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "test_id", state.id);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "test_revision", state.revision);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "frame", state.frame);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "target_frame", state.target_frame);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "oldest_frame", state.oldest_frame);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "seed", state.seed);
    if (state.has_checkpoint)
        yyjson_mut_obj_add_uint(reply.document, reply.result, "checkpoint_frame", state.checkpoint_frame);
    else yyjson_mut_obj_add_null(reply.document, reply.result, "checkpoint_frame");
    if (state.error[0]) add_string(reply, reply.result, "message", state.error);
}

void add_real(Reply& reply, yyjson_mut_val* object, const char* field, float value)
{
    if (std::isfinite(value)) yyjson_mut_obj_add_real(reply.document, object, field, value);
    else yyjson_mut_obj_add_null(reply.document, object, field);
}

void add_trace(const NativeWorkshopPracticeTrace& trace, Reply& reply, yyjson_mut_val* rows)
{
    auto* frame = yyjson_mut_obj(reply.document);
    yyjson_mut_arr_append(rows, frame);
    yyjson_mut_obj_add_uint(reply.document, frame, "frame", trace.frame);
    yyjson_mut_obj_add_uint(reply.document, frame, "rng", trace.rng);
    yyjson_mut_obj_add_uint(reply.document, frame, "items", trace.items);
    yyjson_mut_obj_add_uint(reply.document, frame, "owned_items", trace.projectiles);
    yyjson_mut_obj_add_uint(reply.document, frame, "item_hit_count", trace.hit_count);
    yyjson_mut_obj_add_uint(reply.document, frame, "script_fault_count", trace.fault_count);
    auto* items = yyjson_mut_arr(reply.document);
    auto* hits = yyjson_mut_arr(reply.document);
    auto* faults = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, frame, "live_items", items);
    yyjson_mut_obj_add_val(reply.document, frame, "item_hits", hits);
    yyjson_mut_obj_add_val(reply.document, frame, "script_faults", faults);
    for (unsigned i = 0; i < std::min(trace.items, unsigned(NativeWorkshopPractice_TraceItems)); ++i) {
        const auto& item = trace.item_rows[i];
        auto* row = yyjson_mut_obj(reply.document);
        yyjson_mut_arr_append(items, row);
        yyjson_mut_obj_add_int(reply.document, row, "kind", item.kind);
        yyjson_mut_obj_add_int(reply.document, row, "state_id", item.action);
        yyjson_mut_obj_add_int(reply.document, row, "owner_actor", item.owner);
        add_real(reply, row, "x", item.x);
        add_real(reply, row, "y", item.y);
        add_real(reply, row, "vx", item.vx);
        add_real(reply, row, "vy", item.vy);
    }
    for (unsigned i = 0; i < std::min(trace.hit_count, unsigned(NativeWorkshopPractice_TraceHits)); ++i) {
        const auto& hit = trace.hits[i];
        auto* row = yyjson_mut_obj(reply.document);
        yyjson_mut_arr_append(hits, row);
        yyjson_mut_obj_add_int(reply.document, row, "item_kind", hit.item_kind);
        yyjson_mut_obj_add_int(reply.document, row, "attacker_actor", hit.attacker);
        yyjson_mut_obj_add_int(reply.document, row, "victim_actor", hit.victim);
        add_real(reply, row, "damage", hit.damage);
    }
    for (unsigned i = 0; i < std::min(trace.fault_count, unsigned(NativeWorkshopPractice_TraceFaults)); ++i) {
        const auto& fault = trace.faults[i];
        auto* row = yyjson_mut_obj(reply.document);
        yyjson_mut_arr_append(faults, row);
        add_string(reply, row, "source_kind", fault.source_kind);
        add_string(reply, row, "message", fault.message);
        yyjson_mut_obj_add_uint(reply.document, row, "source_generation", fault.generation);
        yyjson_mut_obj_add_uint(reply.document, row, "line", fault.line);
        yyjson_mut_obj_add_uint(reply.document, row, "column", fault.column);
    }
    auto* fighters = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, frame, "fighters", fighters);
    for (unsigned i = 0; i < NativeWorkshopPractice_Fighters; ++i) {
        const auto& source = trace.fighters[i];
        if (!source.present) continue;
        auto* row = yyjson_mut_obj(reply.document);
        yyjson_mut_arr_append(fighters, row);
        yyjson_mut_obj_add_uint(reply.document, row, "port", i / 2);
        yyjson_mut_obj_add_uint(reply.document, row, "actor", i % 2);
        yyjson_mut_obj_add_int(reply.document, row, "runtime_kind", source.kind);
        yyjson_mut_obj_add_int(reply.document, row, "action_id", source.action);
        yyjson_mut_obj_add_int(reply.document, row, "animation_id", source.animation);
        if (source.script_state) {
            char hex[17];
            std::snprintf(hex, sizeof(hex), "%016llx", static_cast<unsigned long long>(source.script_state));
            add_string(reply, row, "script_state", hex);
        }
        yyjson_mut_obj_add_bool(reply.document, row, "airborne", source.airborne);
        auto* hitboxes = yyjson_mut_arr(reply.document);
        yyjson_mut_obj_add_val(reply.document, row, "hitboxes", hitboxes);
        for (unsigned slot = 0; slot < 4; ++slot)
            if (source.hitboxes & (1 << slot)) yyjson_mut_arr_append(hitboxes, yyjson_mut_uint(reply.document, slot));
        add_real(reply, row, "animation_frame", source.animation_frame);
        add_real(reply, row, "damage", source.damage);
        add_real(reply, row, "x", source.x);
        add_real(reply, row, "y", source.y);
        add_real(reply, row, "vx", source.vx);
        add_real(reply, row, "vy", source.vy);
        add_real(reply, row, "facing", source.facing);
        add_real(reply, row, "hitlag", source.hitlag);
    }
}

void assert_trace(const NativeWorkshopPracticeState& state, yyjson_val* criteria, Reply& reply)
{
    describe(state, reply);
    auto* results = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "checks", results);
    bool all_passed = true;
    size_t index, count;
    yyjson_val* check;
    yyjson_arr_foreach(criteria, index, count, check) {
        const auto from = number(check, "from", state.oldest_frame);
        const auto through = number(check, "through", state.frame);
        const char* predicate = string(check, "predicate");
        const bool inactive = std::strcmp(predicate, "hitbox_inactive") == 0;
        const bool facing = std::strcmp(predicate, "facing") == 0;
        const bool item_hits = std::strcmp(predicate, "item_hit_count") == 0;
        const bool faults = std::strcmp(predicate, "no_script_faults") == 0;
        const auto port = number(check, "port"), member = number(check, "actor");
        const unsigned actor = 2 * port + member;
        const auto expected = integer(check, "expected");
        const auto slot = integer(check, "slot", -1);
        const auto item_kind = integer(check, "item_kind", -1);
        if (from > through || !nativeWorkshopPractice_Trace(from) || !nativeWorkshopPractice_Trace(through)) {
            reply.fail("practice_trace_unavailable", "Choose an inclusive range within the retained practice trace.");
            return;
        }
        if ((!faults && !yyjson_obj_get(check, "port")) ||
            (!inactive && !faults && !yyjson_obj_get(check, "expected")) ||
            (facing && expected != 1 && expected != -1) || (item_hits && expected < 0)) {
            reply.fail("invalid_assertion", "Provide a port and a valid expected value for this predicate.");
            return;
        }
        int observed = 0;
        std::optional<unsigned> failed_at;
        for (unsigned frame = from;; ++frame) {
            const auto& row = *nativeWorkshopPractice_Trace(frame);
            const auto& fighter = row.fighters[actor];
            bool passed = true;
            if (item_hits) {
                if (row.hit_count > NativeWorkshopPractice_TraceHits) {
                    reply.fail("practice_trace_incomplete", "An item-hit burst exceeded the per-frame evidence limit.");
                    return;
                }
                for (unsigned n = 0; n < row.hit_count; ++n) {
                    const auto& hit = row.hits[n];
                    if (hit.victim == int(actor) && (item_kind < 0 || hit.item_kind == item_kind)) ++observed;
                }
            } else if (faults) {
                observed += row.fault_count;
                passed = row.fault_count == 0;
            } else if (inactive) {
                observed = fighter.hitboxes;
                const int mask = slot < 0 ? 15 : 1 << slot;
                passed = fighter.present && !(fighter.hitboxes & mask);
            } else if (facing) {
                observed = fighter.facing < 0 ? -1 : 1;
                passed = fighter.present && observed == expected;
            } else {
                observed = fighter.action;
                passed = fighter.present && observed == expected;
            }
            if (!passed) {
                if (!failed_at) failed_at = frame;
                if (!faults) break;
            }
            if (frame == through) break;
        }
        const bool passed = !failed_at && (!item_hits || expected == observed);
        all_passed &= passed;
        auto* result = yyjson_mut_obj(reply.document);
        yyjson_mut_arr_append(results, result);
        add_string(reply, result, "predicate", predicate);
        yyjson_mut_obj_add_bool(reply.document, result, "passed", passed);
        yyjson_mut_obj_add_uint(reply.document, result, "from", from);
        yyjson_mut_obj_add_uint(reply.document, result, "through", through);
        yyjson_mut_obj_add_int(reply.document, result, "observed", observed);
        if (failed_at) yyjson_mut_obj_add_uint(reply.document, result, "first_failure_frame", *failed_at);
    }
    yyjson_mut_obj_add_bool(reply.document, reply.result, "passed", all_passed);
}

bool run(yyjson_val* arguments, Reply& reply)
{
    PADStatus pads[NativeWorkshopPractice_MaxRun][4]{};
    unsigned total = 0;
    size_t i, count;
    yyjson_val* segment;
    yyjson_arr_foreach(yyjson_obj_get(arguments, "inputs"), i, count, segment) {
        const unsigned frames = number(segment, "frames");
        if (frames > NativeWorkshopPractice_MaxRun - total) {
            reply.fail("practice_run_limit", "Schedule at most 600 frames per run.");
            return false;
        }
        unsigned used = 0;
        size_t j, controllers;
        yyjson_val* controller;
        yyjson_arr_foreach(yyjson_obj_get(segment, "pads"), j, controllers, controller) {
            const unsigned port = number(controller, "port");
            if (used & (1 << port)) {
                reply.fail("duplicate_port", "Specify each controller port once per input segment.");
                return false;
            }
            used |= 1 << port;
            PADStatus pad{};
            pad.stickX = integer(controller, "stick_x");
            pad.stickY = integer(controller, "stick_y");
            pad.substickX = integer(controller, "c_x");
            pad.substickY = integer(controller, "c_y");
            pad.triggerLeft = number(controller, "trigger_l");
            pad.triggerRight = number(controller, "trigger_r");
            static const struct { const char* name; unsigned bits; } buttons[] = {
                {"A", PAD_BUTTON_A}, {"B", PAD_BUTTON_B}, {"X", PAD_BUTTON_X}, {"Y", PAD_BUTTON_Y},
                {"L", PAD_TRIGGER_L}, {"R", PAD_TRIGGER_R}, {"Z", PAD_TRIGGER_Z},
                {"up", PAD_BUTTON_UP}, {"down", PAD_BUTTON_DOWN},
                {"left", PAD_BUTTON_LEFT}, {"right", PAD_BUTTON_RIGHT},
            };
            size_t k, keys;
            yyjson_val* key;
            yyjson_arr_foreach(yyjson_obj_get(controller, "buttons"), k, keys, key) {
                for (const auto& button : buttons)
                    if (!std::strcmp(yyjson_get_str(key), button.name)) pad.button |= button.bits;
            }
            for (unsigned frame = total; frame < total + frames; ++frame) pads[frame][port] = pad;
        }
        total += frames;
    }
    const auto& state = *nativeWorkshopPractice_State();
    if (auto* capture = yyjson_obj_get(arguments, "capture")) {
        if (state.status != NativeWorkshopPractice_Ready || !nativeWorkshopPractice_Controlled() ||
            !total || total > UINT32_MAX - state.frame) {
            reply.fail("practice_unavailable", "Wait for a ready test before scheduling a frame capture.");
            return false;
        }
        if (!nativeWorkshopPreview_ReservePracticeCapture(capture, string(arguments, "instance"),
                                                          state.frame + total, reply)) return false;
    }
    if (nativeWorkshopPractice_Run(pads, total)) return true;
    reply.fail("practice_unavailable", "Start a controlled practice and wait for it to become ready.");
    return false;
}
}

bool nativeWorkshopPractice_Start(yyjson_val* arguments, yyjson_mut_val* inspected, Reply& reply)
{
    auto* options = yyjson_obj_get(arguments, "controlled");
    if (!options) return nativeShellWorkshop_RequestPreview(true, &reply);
    if (nativeOnlineSession_State() != NativeLobbySessionState_Disconnected || nativeMatchLaunch_ReplayPath()) {
        reply.fail("practice_unavailable", "Start controlled practice offline with no replay recording or playback.");
        return false;
    }
    const char* revision = yyjson_mut_get_str(yyjson_mut_obj_get(inspected, "fighter_revision"));
    if (!revision || std::strlen(revision) != 64) {
        reply.fail("revision_unavailable", "Could not identify the fighter revision for the test.");
        return false;
    }
    NativeWorkshopPracticeConfig config{};
    config.seed = number(options, "seed");
    std::memcpy(config.fighter_revision, revision, sizeof(config.fighter_revision));
    config.sandbag = !yyjson_obj_get(options, "sandbag") || yyjson_get_bool(yyjson_obj_get(options, "sandbag"));
    config.sandbag_damage = number(options, "sandbag_damage");
    if (!initial_seat(yyjson_obj_get(options, "player"), 0, config.seats[0], reply)) return false;
    size_t i, count;
    yyjson_val* opponent;
    yyjson_arr_foreach(yyjson_obj_get(options, "opponents"), i, count, opponent) {
        const unsigned port = number(opponent, "port");
        if (config.seats[port].present) {
            reply.fail("duplicate_port", "Choose a different port for each opponent.");
            return false;
        }
        if (!initial_seat(opponent, port, config.seats[port], reply)) return false;
    }
    if (!nativeWorkshopPractice_Arm(&config)) {
        reply.fail("practice_busy", "Finish the current controlled test first.");
        return false;
    }
    if (!nativeShellWorkshop_RequestPreview(true, &reply)) {
        nativeWorkshopPractice_CancelStart("Could not start the controlled practice scene.");
        return false;
    }
    yyjson_mut_obj_add_uint(reply.document, reply.result, "test_id", nativeWorkshopPractice_State()->id);
    return true;
}

void nativeWorkshopPractice_Agent(const char* operation, yyjson_val* arguments, Reply& reply)
{
    const auto& state = *nativeWorkshopPractice_State();
    if (!std::strcmp(operation, "state")) {
        describe(state, reply);
        return;
    }
    if (number(arguments, "test_id") != state.id || number(arguments, "test_revision") != state.revision) {
        reply.fail("practice_conflict", "Read the current test and revision before operating on it.");
        return;
    }
    if (!std::strcmp(operation, "trace")) {
        const unsigned from = number(arguments, "from", state.frame);
        const unsigned limit = number(arguments, "limit", 60);
        if (!nativeWorkshopPractice_Trace(from)) {
            reply.fail("practice_trace_unavailable", "Choose a recorded frame within the retained trace.");
            return;
        }
        describe(state, reply);
        auto* rows = yyjson_mut_arr(reply.document);
        yyjson_mut_obj_add_val(reply.document, reply.result, "frames", rows);
        for (unsigned i = 0; i < std::min(limit, state.frame - from + 1); ++i)
            add_trace(*nativeWorkshopPractice_Trace(from + i), reply, rows);
        return;
    }
    if (!std::strcmp(operation, "assert")) {
        assert_trace(state, yyjson_obj_get(arguments, "checks"), reply);
        return;
    }
    if (nativeFeedback_IsOpen()) {
        reply.fail("practice_busy", "Close the F2 feedback editor before running a practice command.");
        return;
    }
    if (!std::strcmp(operation, "run")) {
        if (!run(arguments, reply)) return;
    } else {
        const auto request = !std::strcmp(operation, "reset") ? NativeWorkshopPractice_Reset :
            !std::strcmp(string(arguments, "action"), "save") ? NativeWorkshopPractice_Save :
            NativeWorkshopPractice_Restore;
        if (!nativeWorkshopPractice_Checkpoint(request)) {
            reply.fail("practice_checkpoint_unavailable", "Wait for the test to be ready and save a checkpoint before restoring it.");
            return;
        }
    }
    describe(state, reply);
}
