#pragma once
#include <atomic>
#include <format>
#include <print>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Time.hpp"

namespace Log
{

// Messages below `level` are hidden. Errors are always shown.
enum class Level
{
    Debug = 0,
    Info  = 1,
    Warn  = 2,
    Error = 3
};

inline std::atomic<Level> level = Level::Debug;

namespace Color
{

constexpr std::string_view Reset = "\033[0m";
constexpr std::string_view Bold  = "\033[1m";
constexpr std::string_view Dim   = "\033[2m";

// Foreground
constexpr std::string_view Red     = "\033[31m";
constexpr std::string_view Green   = "\033[32m";
constexpr std::string_view Yellow  = "\033[33m";
constexpr std::string_view Blue    = "\033[34m";
constexpr std::string_view Magenta = "\033[35m";
constexpr std::string_view Cyan    = "\033[36m";
constexpr std::string_view White   = "\033[37m";

// Bright foreground
constexpr std::string_view BrightRed     = "\033[91m";
constexpr std::string_view BrightGreen   = "\033[92m";
constexpr std::string_view BrightYellow  = "\033[93m";
constexpr std::string_view BrightBlue    = "\033[94m";
constexpr std::string_view BrightMagenta = "\033[95m";
constexpr std::string_view BrightCyan    = "\033[96m";
constexpr std::string_view BrightWhite   = "\033[97m";

} // namespace Color

namespace detail
{

struct Style
{
    std::string_view tag; // padded to 8 characters
    std::string_view tagColor;
    std::string_view textColor;
};

constexpr Style InfoStyle  = {"[INFO]  ", Color::BrightWhite, Color::White};
constexpr Style WarnStyle  = {"[WARN]  ", Color::BrightYellow, Color::Yellow};
constexpr Style DebugStyle = {"[DEBUG] ", Color::BrightBlue, Color::Blue};
constexpr Style ErrorStyle = {"[ERROR] ", Color::BrightRed, Color::Red};
constexpr Style FatalStyle = {"[FATAL] ", Color::BrightRed, Color::Red};

inline bool enabled(Level msgLevel)
{
    return msgLevel >= level;
}

inline void write(const Style& style, std::string_view msg)
{
    std::println(
        "{}[{}] {}{}{}{}{}",
        style.tagColor,
        Time::timestamp(),
        Color::Bold,
        style.tag,
        // Color::Reset,
        style.textColor,
        msg,
        Color::Reset
    );
}

inline void write_t(const Style& style, int indent, std::string_view msg)
{
    std::println(
        "{}[{}] {}{}{}{}",
        style.textColor,
        Time::timestamp(),
        Color::Bold,
        std::string(style.tag.size() + indent * 2, ' '),
        msg,
        Color::Reset
    );
}

} // namespace detail

// ---- Log levels ----

template <typename... Args>
void info(std::format_string<Args...> fmt, Args&&... args)
{
    if (detail::enabled(Level::Info))
        detail::write(detail::InfoStyle, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void info_t(int indent, std::format_string<Args...> fmt, Args&&... args)
{
    if (detail::enabled(Level::Info))
        detail::write_t(detail::InfoStyle, indent, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void warn(std::format_string<Args...> fmt, Args&&... args)
{
    if (detail::enabled(Level::Warn))
        detail::write(detail::WarnStyle, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void warn_t(int indent, std::format_string<Args...> fmt, Args&&... args)
{
    if (detail::enabled(Level::Warn))
        detail::write_t(detail::WarnStyle, indent, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void debug(std::format_string<Args...> fmt, Args&&... args)
{
    if (detail::enabled(Level::Debug))
        detail::write(detail::DebugStyle, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void debug_t(int indent, std::format_string<Args...> fmt, Args&&... args)
{
    if (detail::enabled(Level::Debug))
        detail::write_t(detail::DebugStyle, indent, std::format(fmt, std::forward<Args>(args)...));
}

// Errors are always shown, whatever `level` is.
template <typename... Args>
void error(std::format_string<Args...> fmt, Args&&... args)
{
    detail::write(detail::ErrorStyle, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
void error_t(int indent, std::format_string<Args...> fmt, Args&&... args)
{
    detail::write_t(detail::ErrorStyle, indent, std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
[[noreturn]] void fatal(std::source_location location, std::format_string<Args...> fmt, Args&&... args)
{
    std::string msg = std::format(fmt, std::forward<Args>(args)...);
    detail::write(
        detail::FatalStyle,
        std::format("{}\n    {}at {}:{}", msg, Color::BrightRed, location.file_name(), location.line())
    );
    throw std::runtime_error(msg);
}

#define FATAL(...) Log::fatal(std::source_location::current(), __VA_ARGS__)

} // namespace Log