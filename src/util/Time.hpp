#pragma once

#include <chrono>
#include <concepts>
#include <format>
#include <ratio>
#include <string>

namespace Time
{

inline const auto startTime = std::chrono::steady_clock::now();

template <typename T>
concept Duration = std::same_as<T, std::chrono::duration<typename T::rep, typename T::period>>;

template <Duration D = std::chrono::milliseconds>
D elapsed()
{
    return std::chrono::duration_cast<D>(std::chrono::steady_clock::now() - startTime);
}

// Formats a duration as "  12s 345ms 678us"
// Precision sets how far down to print; defaults to the resolution of the duration itself
template <typename Precision = void, Duration D>
    requires std::is_void_v<Precision> || Duration<Precision>
std::string formatTime(D d)
{
    using namespace std::chrono;
    using Res = std::conditional_t<std::is_void_v<Precision>, D, Precision>; // use D if Precision is void
    using P   = typename Res::period;

    const auto ns = duration_cast<nanoseconds>(duration_cast<Res>(d)).count();

    std::string out = std::format("{:>4}s", ns / 1'000'000'000);
    if constexpr (std::ratio_less_equal_v<P, std::milli>)
        out += std::format(" {:03}ms", ns / 1'000'000 % 1'000);
    if constexpr (std::ratio_less_equal_v<P, std::micro>)
        out += std::format(" {:03}us", ns / 1'000 % 1'000);
    if constexpr (std::ratio_less_equal_v<P, std::nano>)
        out += std::format(" {:03}ns", ns % 1'000);
    return out;
}

inline std::string timestamp()
{
    auto elap = elapsed<std::chrono::microseconds>();
    return formatTime(elap);
}

} // namespace Time