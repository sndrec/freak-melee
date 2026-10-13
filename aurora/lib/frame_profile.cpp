#include "frame_profile.hpp"
#include <SDL3/SDL_timer.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace aurora::frame_profile {
namespace {
constexpr unsigned Capacity = 1024;
struct Record {
  std::array<uint64_t, static_cast<unsigned>(Phase::Count)> phase{};
  uint64_t elapsed{}, uploads{}, hashes{}, misses{}, objectHits{}, contentHits{};
};
struct {
  std::array<Record, Capacity> records{};
  Record current{};
  uint64_t started{}, last{};
  unsigned count{}, overflow{};
  bool enabled{};
} profile;
}

void initialize() noexcept {
  profile = {};
  const char* enabled = std::getenv("AURORA_PROFILE_FRAME_END");
  profile.enabled = enabled && std::strcmp(enabled, "1") == 0;
}
bool begin() noexcept {
  if (!profile.enabled) return false;
  profile.current = {};
  profile.started = profile.last = SDL_GetTicksNS();
  return true;
}
void mark(Phase phase) noexcept {
  const uint64_t now = SDL_GetTicksNS();
  profile.current.phase[static_cast<unsigned>(phase)] = now - profile.last;
  profile.last = now;
}
bool enabled() noexcept { return profile.enabled; }
uint64_t section(Phase phase, uint64_t started) noexcept {
  const uint64_t now = SDL_GetTicksNS();
  profile.current.phase[static_cast<unsigned>(phase)] += now - started;
  return now;
}
void textures(uint64_t uploads, uint64_t hashes, uint64_t misses,
              uint64_t objectHits, uint64_t contentHits) noexcept {
  profile.current.uploads = uploads;
  profile.current.hashes = hashes;
  profile.current.misses = misses;
  profile.current.objectHits = objectHits;
  profile.current.contentHits = contentHits;
}
void end() noexcept {
  profile.current.elapsed = SDL_GetTicksNS() - profile.started;
  if (profile.count < Capacity) profile.records[profile.count++] = profile.current;
  else ++profile.overflow;
}
void report() noexcept {
  if (!profile.enabled) return;
  std::fprintf(stderr, "[frame-end-profile] records=%u overflow=%u scope=caller-frame-end\n",
               profile.count, profile.overflow);
  for (unsigned i = 0; i < profile.count; ++i) {
    const auto& row = profile.records[i];
    std::fprintf(stderr, "[frame-end-profile] index=%u total_us=%.3f fifo_us=%.3f texture_us=%.3f "
        "finish_us=%.3f overlay_us=%.3f submit_us=%.3f uploads=%llu hashes=%llu misses=%llu "
        "object_hits=%llu content_hits=%llu imgui_us=%.3f ui_setup_us=%.3f ui_update_us=%.3f "
        "ui_begin_us=%.3f ui_render_us=%.3f ui_end_us=%.3f\n", i, row.elapsed / 1000., row.phase[0] / 1000.,
        row.phase[1] / 1000., row.phase[2] / 1000., row.phase[3] / 1000., row.phase[4] / 1000.,
        static_cast<unsigned long long>(row.uploads), static_cast<unsigned long long>(row.hashes),
        static_cast<unsigned long long>(row.misses), static_cast<unsigned long long>(row.objectHits),
        static_cast<unsigned long long>(row.contentHits), row.phase[5] / 1000., row.phase[6] / 1000.,
        row.phase[7] / 1000., row.phase[8] / 1000., row.phase[9] / 1000., row.phase[10] / 1000.);
  }
  profile.enabled = false;
}
} // namespace aurora::frame_profile
