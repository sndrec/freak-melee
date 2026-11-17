#pragma once
#include "native_agent_internal.h"
#include <melee/ft/native_fighter_family_project.h>
#include <melee/ft/native_fighter_sources.h>
#include <memory>

namespace NativeAgent {
struct Project {
    NativeFighterFamilyProject family{};
    unsigned member = 0;
    ~Project();
};
std::unique_ptr<Project> open_project(yyjson_val* arguments, Reply&);
bool load_sources(const Project&, NativeFighterSources::Session&, Reply&);
bool source_status(const NativeFighterSources::Session&, const NativeFighterProject&, Reply&);
}
