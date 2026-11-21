#pragma once

#include <aurora/aurora.h>

#include <fmt/base.h>
#include <fmt/format.h>

#include <cstdlib>
#include <string_view>
#include <type_traits>

namespace aurora {
void log_internal(AuroraLogLevel level, const char* module, const char* message, unsigned int len) noexcept;

extern AuroraConfig g_config;

// Reportable messages must be authored literals. Dynamic text belongs in a
// formatting argument, so a diagnostic never records it as the message template.
template <typename... T>
struct LogFormat {
  fmt::format_string<T...> text;
  template <size_t N>
  consteval LogFormat(const char (&literal)[N]) : text(literal) {}
};

struct Module {
  const char* name;
  explicit consteval Module(const char* name) noexcept : name(name) {}

  template <typename... T>
  void report(const AuroraLogLevel level, LogFormat<std::type_identity_t<T>...> format, T&&... args) const noexcept {
    if (g_config.logLevel > level) {
      return;
    }
    auto message = fmt::format(format.text, std::forward<T>(args)...);
    log_internal(level, name, message.c_str(), static_cast<unsigned int>(message.size()));
    if (level >= LOG_WARNING && g_config.diagnosticWarningCallback) {
      const auto text = format.text.get();
      g_config.diagnosticWarningCallback(level, name, text.data(), static_cast<unsigned int>(text.size()));
    }
  }

  template <typename... T>
  void debug(LogFormat<std::type_identity_t<T>...> fmt, T&&... args) const noexcept {
    report(LOG_DEBUG, fmt, std::forward<T>(args)...);
  }

  template <typename... T>
  void info(LogFormat<std::type_identity_t<T>...> fmt, T&&... args) const noexcept {
    report(LOG_INFO, fmt, std::forward<T>(args)...);
  }

  template <typename... T>
  void warn(LogFormat<std::type_identity_t<T>...> fmt, T&&... args) const noexcept {
    report(LOG_WARNING, fmt, std::forward<T>(args)...);
  }

  template <typename... T>
  void error(LogFormat<std::type_identity_t<T>...> fmt, T&&... args) const noexcept {
    report(LOG_ERROR, fmt, std::forward<T>(args)...);
  }

  template <typename... T>
  [[noreturn]] void fatal(LogFormat<std::type_identity_t<T>...> fmt, T&&... args) const noexcept {
    report(LOG_FATAL, fmt, std::forward<T>(args)...);
    std::abort();
  }
};
} // namespace aurora

template <>
struct fmt::formatter<AuroraLogLevel> : formatter<std::string_view> {
  auto format(AuroraLogLevel level, format_context& ctx) const -> format_context::iterator;
};
