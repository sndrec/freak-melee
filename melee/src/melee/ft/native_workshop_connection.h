#pragma once
#include "native_authoring_reply.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <vector>

namespace NativeWorkshopConnection {
constexpr uint32_t MaxMessage = 4 * 1024 * 1024;
constexpr const char* Prefix = "FreakMeleeWorkshop-";
using Handler = void (*)(const char* method, yyjson_val* arguments, NativeAuthoring::Reply&, void*);
struct Server {
    HANDLE pipe = INVALID_HANDLE_VALUE;
    OVERLAPPED io{};
    enum Phase { Off, Listening, Header, Body, Writing, Acknowledgment, Closing } phase = Off;
    bool pending = false;
    DWORD immediate = 0;
    uint32_t length = 0, offset = 0;
    unsigned char acknowledgment = 0;
    uint64_t deadline = 0;
    std::vector<char> buffer;
    std::string instance;
    std::string request_id;
    Handler handler = nullptr;
    void* context = nullptr;
    bool enabled = false;
};
// All server calls run on the editor thread. poll never waits for I/O.
bool start(Server&, Handler, void* context, std::string& error);
void poll(Server&);
void stop(Server&);
void shutdown(Server&);
std::vector<std::string> instances();
bool request(const std::string& instance, const char* method, yyjson_val* arguments,
             NativeAuthoring::Reply&, unsigned timeout_ms = 10000);
}
