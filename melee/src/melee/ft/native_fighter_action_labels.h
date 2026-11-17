#pragma once
#include <string>
namespace NativeFighterActionLabels {
    const char* event(unsigned kind);
const char* field(unsigned opcode, unsigned index);
    std::string command(unsigned opcode);
    const char* command_description(unsigned opcode);
}
