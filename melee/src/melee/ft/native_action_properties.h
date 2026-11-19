#pragma once
#include <string>
#include <vector>

#include "native_authoring_reply.h"
#include "native_fighter_action_editor.h"

namespace NativeActionProperties {
    struct Type {
        std::string name, label, description;
        unsigned opcode, selector;
    };
    const std::vector<Type>& types();
    const Type* type(unsigned opcode, unsigned selector = 0);
    const Type* type(const char* name);
    const Type* type_for_event(unsigned event_kind);
    const char* element_label(unsigned);
    // Both creation defaults and live values use the same native field
    // definitions.
    void inspect(const NativeFighterProject&, unsigned action,
                 const NativeFighterActionCommand&, NativeAuthoring::Reply&,
                 yyjson_mut_val* output);
    void defaults(const NativeFighterProject&, const Type&,
                  NativeAuthoring::Reply&, yyjson_mut_val* output);
    bool update(NativeFighterProject&, unsigned action, uint32_t& source,
                yyjson_val* properties, std::string& error);
    bool create(NativeFighterProject&, unsigned action, unsigned frame,
                const Type&, yyjson_val* properties, uint32_t& source,
                std::string& error);
    // Superset of named field schemas; the selected event rejects irrelevant
    // fields.
    std::string schema();
}
