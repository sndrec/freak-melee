#pragma once
#include <cstdint>

namespace aurora::frame_profile {
enum class Phase : unsigned {
  Fifo, Textures, Finish, Overlay, Submit,
  ImGui, UiSetup, UiUpdate, UiBegin, UiRender, UiEnd, Count
};
void initialize() noexcept;
bool begin() noexcept;
void mark(Phase phase) noexcept;
// Nested overlay sections leave the outer phase boundary unchanged.
bool enabled() noexcept;
uint64_t section(Phase phase, uint64_t started) noexcept;
// Caller only, after the FIFO decoder has drained. No worker shares records.
void textures(uint64_t uploads, uint64_t hashes, uint64_t misses,
              uint64_t objectHits, uint64_t contentHits) noexcept;
void end() noexcept;
void report() noexcept;
} // namespace aurora::frame_profile
