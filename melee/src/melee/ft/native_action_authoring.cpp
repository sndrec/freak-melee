#include "native_action_authoring.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "native_fighter_action_labels.h"
#include "native_fighter_authoring.h"
#include "native_hitbox_lifetime.h"

namespace NativeActionAuthoring {
    namespace {
        using NativeAuthoring::add_string;
        using NativeAuthoring::Reply;
        constexpr unsigned MaxCommands = 512, MaxOccurrences = 4096,
                           MaxFrames = 1000;
        const char* text(yyjson_val* obj, const char* key)
        {
            const char* s = yyjson_get_str(yyjson_obj_get(obj, key));
            return s ? s : "";
        }
        bool contains(const char* haystack, const char* needle)
        {
            std::string h = haystack ? haystack : "", n = needle ? needle : "";
            return std::search(h.begin(), h.end(), n.begin(), n.end(),
                               [](unsigned char a, unsigned char b) {
                                   return std::tolower(a) == std::tolower(b);
                               }) != h.end();
        }
        void page(Reply& r, size_t count, unsigned offset, unsigned limit)
        {
            yyjson_mut_obj_add_uint(r.document, r.result, "total", count);
            if (offset + limit < count) {
                yyjson_mut_obj_add_uint(r.document, r.result, "next_offset",
                                        offset + limit);
            } else {
                yyjson_mut_obj_add_null(r.document, r.result, "next_offset");
            }
        }
        unsigned limit(yyjson_val* a)
        {
            auto* v = yyjson_obj_get(a, "limit");
            return v ? (unsigned) yyjson_get_uint(v) : 50;
        }
        std::string handle(uint32_t source)
        {
            char out[24];
            std::snprintf(out, sizeof(out), "event-%08x", source);
            return out;
        }
        struct Action {
            unsigned id = 0;
            NativeFighterActionInfo info{};
            std::vector<NativeFighterActionCommand> commands;
            std::vector<NativeFighterActionOccurrence> trace;
            bool complete = true;
            bool load(const NativeFighterProject& p, yyjson_val* args,
                      Reply& r, bool timeline = true)
            {
                auto* field = yyjson_obj_get(args, "action_id");
                if (!field || !yyjson_is_uint(field) ||
                    yyjson_get_uint(field) >= p.action_count)
                {
                    r.fail("unknown_action",
                           "Choose an action_id returned by actions.list or "
                           "the live action list.");
                    return false;
                }
                id = (unsigned) yyjson_get_uint(field);
                size_t count = 0;
                std::vector<NativeFighterActionInfo> all(p.action_count);
                if (!nativeFighterAction_List(&p, all.data(), all.size(),
                                              &count))
                {
                    r.fail("invalid_actions",
                           "Could not read this fighter's action table.");
                    return false;
                }
                info = all[id];
                if (!nativeFighterAction_Commands(&p, id, nullptr, 0,
                                                  &count) ||
                    count > MaxCommands)
                {
                    r.fail("command_limit",
                           "This action's command graph exceeds the supported "
                           "512-command limit or is malformed.");
                    return false;
                }
                commands.resize(count);
                if (count && !nativeFighterAction_Commands(
                                 &p, id, commands.data(), count, &count))
                {
                    r.fail("invalid_events",
                           "Could not read the action's commands.");
                    return false;
                }
                if (timeline && info.has_script) {
                    trace.resize(MaxOccurrences);
                    if (!nativeFighterAction_Timeline(&p, id, trace.data(),
                                                      trace.size(), &count,
                                                      &complete))
                    {
                        r.fail(
                            "invalid_timeline",
                            "Could not trace the action's timers and loops.");
                        return false;
                    }
                    trace.resize(count);
                }
                return true;
            }
            const NativeFighterActionCommand* event(const char* id) const
            {
                for (const auto& c : commands) {
                    if (handle(c.source_offset) == id) {
                        return &c;
                    }
                }
                return nullptr;
            }
        };
        uint32_t word(const NativeFighterProject& p, uint32_t source)
        {
            const auto* b = p.fighter_dat + 32 + source;
            return uint32_t(b[0]) << 24 | uint32_t(b[1]) << 16 |
                   uint32_t(b[2]) << 8 | b[3];
        }
        void metadata(const NativeFighterProject& p, const Action& a, Reply& r)
        {
            NativeFighterActionMetadata m{};
            nativeFighterAction_GetMetadata(&p, a.id, &m);
            yyjson_mut_obj_add_uint(r.document, r.result, "action_id", a.id);
            add_string(r, r.result, "name", a.info.name);
            const char* label =
                nativeFighterAuthoring_Name(&p, NativeAuthoring_Action, a.id);
            if (label) {
                add_string(r, r.result, "label", label);
            }
            yyjson_mut_obj_add_bool(r.document, r.result, "hidden",
                                    nativeFighterAuthoring_Hidden(&p, a.id));
            yyjson_mut_obj_add_uint(r.document, r.result, "animation_flags",
                                    m.animation_flags);
            yyjson_mut_obj_add_bool(r.document, r.result, "loop",
                                    (m.animation_flags & 0x40000000u) != 0);
            yyjson_mut_obj_add_bool(r.document, r.result, "root_motion",
                                    (m.animation_flags & 0x80000000u) != 0);
            yyjson_mut_obj_add_bool(r.document, r.result, "has_animation",
                                    m.animation_size > 0);
            yyjson_mut_obj_add_uint(r.document, r.result, "command_count",
                                    a.commands.size());
            yyjson_mut_obj_add_uint(r.document, r.result, "occurrence_count",
                                    a.trace.size());
            yyjson_mut_obj_add_bool(r.document, r.result, "timeline_complete",
                                    a.complete);
            yyjson_mut_obj_add_uint(r.document, r.result, "maximum_commands",
                                    MaxCommands);
            yyjson_mut_obj_add_uint(r.document, r.result,
                                    "maximum_occurrences", MaxOccurrences);
            yyjson_mut_obj_add_uint(r.document, r.result, "maximum_edit_frame",
                                    MaxFrames);
        }
        void event(const NativeFighterProject& p, const Action& a,
                   const NativeFighterActionCommand& c, Reply& r,
                   yyjson_mut_val* out, bool properties)
        {
            add_string(r, out, "event_id", handle(c.source_offset));
            yyjson_mut_obj_add_uint(r.document, out, "opcode", c.opcode);
            const auto* type = NativeActionProperties::type(
                c.opcode, word(p, c.source_offset));
            add_string(r, out, "kind", type ? type->name : "control");
            add_string(r, out, "label",
                       NativeFighterActionLabels::command(c.opcode));
            add_string(
                r, out, "description",
                NativeFighterActionLabels::command_description(c.opcode));
            yyjson_mut_obj_add_bool(r.document, out, "timer",
                                    c.opcode == 1 || c.opcode == 2);
            yyjson_mut_obj_add_bool(r.document, out, "editable",
                                    type != nullptr);
            unsigned hits = 0;
            int first = -1;
            for (const auto& t : a.trace) {
                if (t.source_offset == c.source_offset) {
                    if (first < 0) {
                        first = t.frame;
                    }
                    ++hits;
                }
            }
            if (first >= 0) {
                yyjson_mut_obj_add_int(r.document, out, "first_frame", first);
            } else {
                yyjson_mut_obj_add_null(r.document, out, "first_frame");
            }
            yyjson_mut_obj_add_uint(r.document, out, "occurrences", hits);
            if (properties) {
                NativeActionProperties::inspect(p, a.id, c, r, out);
            }
        }
        std::string schema(bool editing, bool live)
        {
            std::string fields =
                live
                    ? R"("instance":{"type":"string","minLength":34,"maxLength":43})"
                    : R"("project":{"type":"string","minLength":1,"maxLength":500},"member":{"type":"string","maxLength":31})";
            fields +=
                R"(,"action_id":{"type":"integer","minimum":0,"maximum":1023},"event_id":{"type":"string","minLength":14,"maxLength":14})";
            std::string required = live ? "\"instance\"" : "\"project\"";
            if (editing) {
                fields +=
                    R"(,"revision":{"type":"string","minLength":64,"maxLength":64},"summary":{"type":"string","minLength":1,"maxLength":300},"operation":{"type":"string","enum":["metadata","playback","own_hitboxes","create","update","timing","delete"]},"label":{"type":"string","maxLength":63},"hidden":{"type":"boolean"},"kind":{"type":"string","minLength":1,"maxLength":64},"frame":{"type":"integer","minimum":0,"maximum":1000},"delta":{"type":"integer","minimum":-1000,"maximum":1000},"duration":{"type":"number","minimum":0.00390625,"maximum":1000},"loop":{"type":"boolean"},"root_motion":{"type":"boolean"},"animation_flags":{"type":"integer","minimum":0,"maximum":4294967295},"properties":)" +
                    NativeActionProperties::schema();
                fields +=
                    R"(,"updates":{"type":"array","minItems":1,"maxItems":256,"items":{"type":"object","properties":{"event_id":{"type":"string","minLength":14,"maxLength":14},"properties":)" +
                    NativeActionProperties::schema() +
                    R"(},"required":["event_id","properties"],"additionalProperties":false}})";
                required += ",\"action_id\",\"revision\",\"operation\"";
                if (live) {
                    required += ",\"summary\"";
                }
            } else {
                fields +=
                    R"(,"view":{"type":"string","enum":["metadata","events","event","timeline","types"]},"query":{"type":"string","maxLength":256},"offset":{"type":"integer","minimum":0,"maximum":1000000},"limit":{"type":"integer","minimum":1,"maximum":200},"include_timers":{"type":"boolean"})";
                required += ",\"view\"";
            }
            return "{\"type\":\"object\",\"properties\":{" + fields +
                   "},\"required\":[" + required +
                   "],\"additionalProperties\":false}";
        }
    }

    const char* read_schema(bool live)
    {
        static const std::string saved = schema(false, false),
                                 running = schema(false, true);
        return (live ? running : saved).c_str();
    }
    const char* edit_schema(bool live)
    {
        static const std::string saved = schema(true, false),
                                 running = schema(true, true);
        return (live ? running : saved).c_str();
    }
    void list(const NativeFighterProject& p, yyjson_val* args, Reply& r)
    {
        std::vector<NativeFighterActionInfo> actions(p.action_count);
        size_t count = 0;
        if (!nativeFighterAction_List(&p, actions.data(), actions.size(),
                                      &count))
        {
            r.fail("invalid_actions",
                   "Could not read this fighter's action table.");
            return;
        }
        auto* rows = yyjson_mut_arr(r.document);
        yyjson_mut_obj_add_val(r.document, r.result, "actions", rows);
        unsigned offset = (unsigned) yyjson_get_uint(
                     yyjson_obj_get(args, "offset")),
                 take = limit(args);
        size_t total = 0;
        for (const auto& a : actions) {
            const char* label = nativeFighterAuthoring_Name(
                &p, NativeAuthoring_Action, a.index);
            if (!contains(a.name, text(args, "query")) &&
                !(label && contains(label, text(args, "query"))))
            {
                continue;
            }
            if (total++ < offset || yyjson_mut_arr_size(rows) >= take) {
                continue;
            }
            auto* row = yyjson_mut_obj(r.document);
            yyjson_mut_arr_append(rows, row);
            yyjson_mut_obj_add_uint(r.document, row, "id", a.index);
            add_string(r, row, "name", a.name);
            if (label) {
                add_string(r, row, "label", label);
            }
            yyjson_mut_obj_add_bool(
                r.document, row, "hidden",
                nativeFighterAuthoring_Hidden(&p, a.index));
            yyjson_mut_obj_add_bool(r.document, row, "has_events",
                                    a.has_script);
        }
        page(r, total, offset, take);
    }

    void read(const NativeFighterProject& p, yyjson_val* args, Reply& r)
    {
        const char* view = text(args, "view");
        unsigned offset = (unsigned) yyjson_get_uint(
                     yyjson_obj_get(args, "offset")),
                 take = limit(args);
        if (!std::strcmp(view, "types")) {
            auto* rows = yyjson_mut_arr(r.document);
            yyjson_mut_obj_add_val(r.document, r.result, "types", rows);
            size_t total = 0;
            for (const auto& t : NativeActionProperties::types()) {
                if (!contains(t.name.c_str(), text(args, "query")) &&
                    !contains(t.label.c_str(), text(args, "query")))
                {
                    continue;
                }
                if (total++ < offset || yyjson_mut_arr_size(rows) >= take) {
                    continue;
                }
                auto* row = yyjson_mut_obj(r.document);
                yyjson_mut_arr_append(rows, row);
                add_string(r, row, "kind", t.name);
                add_string(r, row, "label", t.label);
                add_string(r, row, "description", t.description);
                NativeActionProperties::defaults(p, t, r, row);
            }
            page(r, total, offset, take);
            return;
        }
        Action a;
        if (!a.load(p, args, r)) {
            return;
        }
        metadata(p, a, r);
        if (!std::strcmp(view, "metadata")) {
            return;
        }
        if (!std::strcmp(view, "event")) {
            const auto* c = a.event(text(args, "event_id"));
            if (!c) {
                r.fail(
                    "unknown_event",
                    "Use an event_id returned for this action and revision.");
                return;
            }
            event(p, a, *c, r, r.result, true);
            auto* rows = yyjson_mut_arr(r.document);
            yyjson_mut_obj_add_val(r.document, r.result, "frames", rows);
            for (const auto& o : a.trace) {
                if (o.source_offset == c->source_offset) {
                    yyjson_mut_arr_append(
                        rows, yyjson_mut_uint(r.document, o.frame));
                }
            }
            return;
        }
        const bool trace = !std::strcmp(view, "timeline");
        auto* rows = yyjson_mut_arr(r.document);
        yyjson_mut_obj_add_val(r.document, r.result,
                               trace ? "occurrences" : "events", rows);
        size_t total = 0;
        const bool timers =
            yyjson_get_bool(yyjson_obj_get(args, "include_timers"));
        if (trace) {
            for (const auto& o : a.trace) {
                if (o.opcode < 9 &&
                    !(timers && (o.opcode == 1 || o.opcode == 2)))
                {
                    continue;
                }
                if (total++ < offset || yyjson_mut_arr_size(rows) >= take) {
                    continue;
                }
                auto* row = yyjson_mut_obj(r.document);
                yyjson_mut_arr_append(rows, row);
                add_string(r, row, "event_id", handle(o.source_offset));
                yyjson_mut_obj_add_uint(r.document, row, "frame", o.frame);
                yyjson_mut_obj_add_uint(r.document, row, "opcode", o.opcode);
                NativeFighterHitbox hit;
                if (nativeHitbox_IsCreate(o.opcode) &&
                    nativeFighterAction_GetHitbox(&p, a.id, o.source_offset,
                                                  &hit) &&
                    hit.timed)
                {
                    yyjson_mut_obj_add_real(
                        r.document, row, "duration_end_frame",
                        o.frame + double(hit.duration256) / 256.);
                }
            }
        } else {
            for (const auto& c : a.commands) {
                if (c.opcode < 9 &&
                    !(timers && (c.opcode == 1 || c.opcode == 2)))
                {
                    continue;
                }
                const auto* t = NativeActionProperties::type(
                    c.opcode, word(p, c.source_offset));
                if (!contains(
                        NativeFighterActionLabels::command(c.opcode).c_str(),
                        text(args, "query")) &&
                    !(t && contains(t->name.c_str(), text(args, "query"))))
                {
                    continue;
                }
                if (total++ < offset || yyjson_mut_arr_size(rows) >= take) {
                    continue;
                }
                auto* row = yyjson_mut_obj(r.document);
                yyjson_mut_arr_append(rows, row);
                event(p, a, c, r, row, false);
            }
        }
        page(r, total, offset, take);
    }

    bool edit(NativeFighterProject& p, yyjson_val* args, Reply& r)
    {
        const char* op = text(args, "operation");
        const bool editing_metadata = !std::strcmp(op, "metadata");
        Action before;
        if (!before.load(p, args, r, !editing_metadata)) {
            return false;
        }
        struct Candidate {
            NativeFighterProject p;
            bool owns_authoring = false;
            ~Candidate()
            {
                std::free(p.fighter_dat);
                if (owns_authoring) {
                    std::free(p.authoring);
                }
            }
        } candidate{ p };
        candidate.p.fighter_dat = (uint8_t*) std::malloc(p.fighter_size);
        if (!candidate.p.fighter_dat) {
            r.fail("allocation_failed", "Could not stage the action edit.");
            return false;
        }
        std::memcpy(candidate.p.fighter_dat, p.fighter_dat, p.fighter_size);
        auto& next = candidate.p;
        std::string error;
        bool ok = false;
        bool metadata_changed = false;
        uint32_t source = 0;
        if (editing_metadata) {
            const char* old = nativeFighterAuthoring_Name(
                &p, NativeAuthoring_Action, before.id);
            const char* label = yyjson_obj_get(args, "label")
                                    ? text(args, "label")
                                    : (old ? old : "");
            const bool hidden =
                yyjson_obj_get(args, "hidden")
                    ? yyjson_get_bool(yyjson_obj_get(args, "hidden"))
                    : nativeFighterAuthoring_Hidden(&p, before.id);
            if (!yyjson_obj_get(args, "label") &&
                !yyjson_obj_get(args, "hidden"))
            {
                r.fail("missing_metadata", "Supply a label, hidden, or both.");
                return false;
            }
            metadata_changed =
                std::strcmp(old ? old : "", label) ||
                hidden != nativeFighterAuthoring_Hidden(&p, before.id);
            ok = true;
            if (metadata_changed) {
                next.authoring = (NativeFighterAuthoring*) std::calloc(
                    1, sizeof(NativeFighterAuthoring));
                candidate.owns_authoring = true;
                if (!next.authoring) {
                    r.fail("allocation_failed",
                           "Could not stage the action label.");
                    return false;
                }
                if (p.authoring) {
                    *next.authoring = *p.authoring;
                }
                ok =
                    nativeFighterAuthoring_SetName(
                        &next, NativeAuthoring_Action, before.id, label) &&
                    nativeFighterAuthoring_SetHidden(&next, before.id, hidden);
            }
        } else if (!std::strcmp(op, "playback")) {
            NativeFighterActionMetadata m{};
            ok = nativeFighterAction_GetMetadata(&next, before.id, &m);
            uint32_t flags = yyjson_obj_get(args, "animation_flags")
                                 ? (uint32_t) yyjson_get_uint(
                                       yyjson_obj_get(args, "animation_flags"))
                                 : m.animation_flags;
            if (yyjson_obj_get(args, "loop")) {
                flags = yyjson_get_bool(yyjson_obj_get(args, "loop"))
                            ? flags | 0x40000000u
                            : flags & ~0x40000000u;
            }
            if (yyjson_obj_get(args, "root_motion")) {
                flags = yyjson_get_bool(yyjson_obj_get(args, "root_motion"))
                            ? flags | 0x80000000u
                            : flags & ~0x80000000u;
            }
            ok = ok && nativeFighterAction_SetAnimationFlags(&next, before.id,
                                                             flags);
        } else if (!std::strcmp(op, "own_hitboxes")) {
            unsigned count = 0;
            ok = !before.info.has_script ||
                 nativeFighterAction_ConvertHitboxIntervals(&next, before.id,
                                                            &count);
            yyjson_mut_obj_add_uint(r.document, r.result, "converted_hitboxes",
                                    count);
        } else if (!std::strcmp(op, "create")) {
            const auto* kind =
                NativeActionProperties::type(text(args, "kind"));
            if (!kind) {
                r.fail("unknown_event_kind",
                       "Choose a kind returned by the types view.");
                return false;
            }
            if (!yyjson_obj_get(args, "frame")) {
                r.fail("missing_frame", "Choose a frame for the new event.");
                return false;
            }
            ok = NativeActionProperties::create(
                next, before.id,
                (unsigned) yyjson_get_uint(yyjson_obj_get(args, "frame")),
                *kind, yyjson_obj_get(args, "properties"), source, error);
        } else if (!std::strcmp(op, "update")) {
            auto* updates = yyjson_obj_get(args, "updates");
            if (updates && (yyjson_obj_get(args, "event_id") ||
                            yyjson_obj_get(args, "properties")))
            {
                r.fail("ambiguous_updates",
                       "Use updates for a batch, or event_id and properties "
                       "for one event.");
                return false;
            }
            struct Update {
                size_t ordinal;
                yyjson_val* properties;
                std::string previous;
            };
            std::vector<Update> edits;
            const auto collect = [&](yyjson_val* entry) {
                const char* wanted = text(entry, "event_id");
                const auto* command = before.event(wanted);
                if (!command ||
                    !NativeActionProperties::type(
                        command->opcode, word(p, command->source_offset)))
                {
                    r.fail("unknown_event",
                           "Choose an editable event from this action's "
                           "inspected revision.");
                    return false;
                }
                auto* properties = yyjson_obj_get(entry, "properties");
                if (!yyjson_is_obj(properties)) {
                    r.fail("missing_properties",
                           "Supply the event properties to change.");
                    return false;
                }
                const size_t ordinal = command - before.commands.data();
                for (const auto& edit : edits) {
                    if (edit.ordinal == ordinal) {
                        r.fail("duplicate_event",
                               "Combine each event's property changes in one "
                               "updates entry.");
                        return false;
                    }
                }
                edits.push_back({ ordinal, properties, wanted });
                return true;
            };
            if (updates) {
                size_t i, count;
                yyjson_val* entry;
                yyjson_arr_foreach(updates, i, count,
                                   entry) if (!collect(entry)) return false;
            } else if (!collect(args)) {
                return false;
            }
            ok = !edits.empty();
            for (const auto& edit : edits) {
                Action current;
                if (!ok || !current.load(next, args, r, false)) {
                    return false;
                }
                // Property edits preserve command order and count.
                // Localization copies the action's command range in that same
                // source order.
                if (current.commands.size() != before.commands.size()) {
                    r.fail("command_graph_changed",
                           "The property edit changed this action's command "
                           "graph.");
                    return false;
                }
                source = current.commands[edit.ordinal].source_offset;
                unsigned users = 0;
                ok = nativeFighterAction_Users(&next, source, &users) && users;
                if (ok && users > 1) {
                    ok = nativeFighterAction_LocalizeCommand(&next, before.id,
                                                             source, &source);
                }
                if (ok) {
                    ok = NativeActionProperties::update(
                        next, before.id, source, edit.properties, error);
                }
                if (!ok) {
                    break;
                }
            }
            if (ok) {
                Action after;
                if (!after.load(next, args, r)) {
                    return false;
                }
                auto* changed = yyjson_mut_arr(r.document);
                yyjson_mut_obj_add_val(r.document, r.result, "updated_events",
                                       changed);
                for (const auto& edit : edits) {
                    const auto& command = after.commands[edit.ordinal];
                    auto* row = yyjson_mut_obj(r.document);
                    yyjson_mut_arr_append(changed, row);
                    add_string(r, row, "previous_event_id", edit.previous);
                    event(next, after, command, r, row, true);
                    yyjson_mut_obj_add_val(
                        r.document, row, "requested",
                        yyjson_val_mut_copy(r.document, edit.properties));
                }
                source = after.commands[edits[0].ordinal].source_offset;
            }
        } else {
            const auto* c = before.event(text(args, "event_id"));
            if (!c) {
                r.fail(
                    "unknown_event",
                    "Use an event_id from this action's inspected revision.");
                return false;
            }
            source = c->source_offset;
            if (!std::strcmp(op, "delete")) {
                ok = nativeFighterAction_DeleteCommand(&next, before.id,
                                                       source);
            } else if (!std::strcmp(op, "timing")) {
                int delta =
                    (int) yyjson_get_sint(yyjson_obj_get(args, "delta"));
                if (yyjson_obj_get(args, "frame")) {
                    int first = -1;
                    for (const auto& at : before.trace) {
                        if (at.source_offset == source) {
                            first = at.frame;
                            break;
                        }
                    }
                    if (first < 0) {
                        r.fail(
                            "unknown_timing",
                            "This event has no proven timeline occurrence.");
                        return false;
                    }
                    delta =
                        (int) yyjson_get_uint(yyjson_obj_get(args, "frame")) -
                        first;
                }
                double duration =
                    yyjson_get_num(yyjson_obj_get(args, "duration"));
                ok = nativeFighterAction_EditEventTiming(
                    &next, before.id, source, delta,
                    (uint32_t) std::lround(duration * 256), &source);
            }
        }
        if (!ok ||
            (!editing_metadata &&
             (before.info.has_script || !std::strcmp(op, "create")) &&
             !nativeFighterAction_HitboxIntervalsValid(&next, before.id)))
        {
            r.fail("action_edit_rejected",
                   error.empty()
                       ? "The edit violates this action's command graph, "
                         "supported operand ranges, hitbox intervals or loop "
                         "boundaries. The original draft is preserved."
                       : error);
            return false;
        }
        bool archive_changed =
            p.fighter_size != next.fighter_size ||
            std::memcmp(p.fighter_dat, next.fighter_dat, p.fighter_size) != 0;
        if (archive_changed) {
            std::free(p.fighter_dat);
            p.fighter_dat = next.fighter_dat;
            p.fighter_size = next.fighter_size;
            next.fighter_dat = nullptr;
        }
        if (metadata_changed) {
            std::free(p.authoring);
            p.authoring = next.authoring;
            candidate.owns_authoring = false;
        }
        bool changed = archive_changed || metadata_changed;
        if (editing_metadata) {
            const char* label = nativeFighterAuthoring_Name(
                &p, NativeAuthoring_Action, before.id);
            add_string(r, r.result, "label", label ? label : "");
            yyjson_mut_obj_add_bool(
                r.document, r.result, "hidden",
                nativeFighterAuthoring_Hidden(&p, before.id));
        }
        yyjson_mut_obj_add_bool(r.document, r.result, "changed", changed);
        add_string(r, r.result, "status",
                   changed ? "edited_draft" : "unchanged");
        if (!std::strcmp(op, "create") || !std::strcmp(op, "update") ||
            !std::strcmp(op, "timing"))
        {
            add_string(r, r.result, "event_id", handle(source));
        }
        yyjson_mut_obj_add_bool(r.document, r.result, "saved", false);
        yyjson_mut_obj_add_bool(r.document, r.result, "installed", false);
        add_string(r, r.result, "preview_status", "refresh_required");
        return true;
    }
}
