#pragma once

#ifdef AURORA_ENABLE_RMLUI

#include <RmlUi/Core/Context.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>

union SDL_Event;

namespace aurora::rmlui {

enum class InputType {
  Text,
  Number,
};

inline constexpr const char* TouchStartEvent = "touchstart";
inline constexpr const char* TouchMoveEvent = "touchmove";
inline constexpr const char* TouchEndEvent = "touchend";
inline constexpr const char* TouchCancelEvent = "touchcancel";

Rml::Context* get_context() noexcept;
bool is_initialized() noexcept;
// Dispatch input to the current RmlUi document. An application event filter can
// use this for a modal overlay, then consume the event before game dispatch.
void handle_event(SDL_Event& event) noexcept;
void set_input_type(InputType type) noexcept;
void set_ui_scale(float scale) noexcept;
float get_ui_scale() noexcept;
// Render RmlUi across the native presentation surface instead of restricting
// it to the aspect-fitted game image. This supports desktop shells around a
// fixed-aspect game viewport. Surface overlays use a transparent base layer;
// scene backdrop filters are only supported in the content-sized mode.
void set_surface_overlay(bool enabled) noexcept;
bool get_surface_overlay() noexcept;
void set_glass_light_dir(float x, float y) noexcept;

struct RuntimeTexture {
  uint32_t width = 0;
  uint32_t height = 0;
  std::span<const std::byte> rgba8;
  bool premultipliedAlpha = false;
  bool generateMipmaps = false;
};

using TextureProvider = std::function<std::optional<RuntimeTexture>(std::string_view)>;

void register_texture_provider(std::string scheme, TextureProvider provider);
void unregister_texture_provider(std::string_view scheme) noexcept;

} // namespace aurora::rmlui

#endif
