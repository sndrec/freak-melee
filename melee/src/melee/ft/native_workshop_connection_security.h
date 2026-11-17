#pragma once
#include "native_workshop_connection.h"
namespace NativeWorkshopConnection {
bool instance_valid(const std::string&);
std::string pipe_name(const std::string&);
std::string random_id();
bool process_is_current_user(DWORD process);
PSECURITY_DESCRIPTOR owner_descriptor();
}
