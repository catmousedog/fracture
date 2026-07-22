#pragma once
#include <atomic>
#include <format>
#include <print>
#include <source_location>
#include <string_view>

namespace Log
{

enum class Level
{
    Info  = 0,
    Warn  = 1,
    Debug = 2,
    Error = 3 // always show errors
};

inline std::atomic<Level> level = Level::Info;

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

// ---- Log levels ----

template <typename... Args>
void info(std::format_string<Args...> fmt, Args&&... args)
{
    if (level > Level::Info)
        return;

    std::print("{}[INFO]  {}", Color::BrightCyan, Color::Reset);
    std::println("{}{}{}{}", Color::White, Color::Bold, std::format(fmt, std::forward<Args>(args)...), Color::Reset);
}

template <typename... Args>
void info_t(int indent, std::format_string<Args...> fmt, Args&&... args)
{
    if (level > Level::Info)
        return;

    std::println(
        "        {}{}{}{}",
        Color::White,
        std::string(indent * 2, ' '),
        std::format(fmt, std::forward<Args>(args)...),
        Color::Reset
    );
}

template <typename... Args>
void warn(std::format_string<Args...> fmt, Args&&... args)
{
    if (level > Level::Warn)
        return;

    std::print("{}[WARN]  {}", Color::BrightYellow, Color::Reset);
    std::println("{}{}{}{}", Color::Yellow, Color::Bold, std::format(fmt, std::forward<Args>(args)...), Color::Reset);
}

template <typename... Args>
void warn_t(int indent, std::format_string<Args...> fmt, Args&&... args)
{
    if (level > Level::Warn)
        return;

    std::println(
        "        {}{}{}{}",
        Color::Yellow,
        std::string(indent * 2, ' '),
        std::format(fmt, std::forward<Args>(args)...),
        Color::Reset
    );
}

template <typename... Args>
void debug(std::format_string<Args...> fmt, Args&&... args)
{
    if (level > Level::Debug)
        return;

    std::print("{}[DEBUG] {}", Color::Blue, Color::Reset);
    std::println("{}{}{}{}", Color::Dim, Color::Bold, std::format(fmt, std::forward<Args>(args)...), Color::Reset);
}

template <typename... Args>
void debug_t(int indent, std::format_string<Args...> fmt, Args&&... args)
{
    if (level > Level::Debug)
        return;

    std::println(
        "        {}{}{}",
        Color::Dim,
        std::string(indent * 2, ' '),
        std::format(fmt, std::forward<Args>(args)...),
        Color::Reset
    );
}

template <typename... Args>
void error(std::format_string<Args...> fmt, Args&&... args)
{
    if (level > Level::Error)
        return;

    std::print("{}[ERROR] {}", Color::BrightRed, Color::Reset);
    std::println("{}{}{}{}", Color::Red, Color::Bold, std::format(fmt, std::forward<Args>(args)...), Color::Reset);
}

template <typename... Args>
void error_t(int indent, std::format_string<Args...> fmt, Args&&... args)
{
    if (level > Level::Error)
        return;

    std::println(
        "        {}{}{}{}",
        Color::Red,
        std::string(indent * 2, ' '),
        std::format(fmt, std::forward<Args>(args)...),
        Color::Reset
    );
}

template <typename... Args>
[[noreturn]] void fatal(std::source_location location, std::format_string<Args...> fmt, Args&&... args)
{
    std::string msg = std::format(fmt, std::forward<Args>(args)...);
    std::print("{}[FATAL] {}", Color::BrightRed, Color::Reset);
    std::println("{}{}{}{}", Color::Red, Color::Bold, msg, Color::Reset);
    std::println("    {}at {}:{}{}", Color::BrightRed, location.file_name(), location.line(), Color::Reset);
    throw std::runtime_error(msg);
}

#define FATAL(...) Log::fatal(std::source_location::current(), __VA_ARGS__)

} // namespace Log