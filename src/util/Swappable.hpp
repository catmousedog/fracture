#pragma once

#include <array>
#include <concepts>
#include <cstdint>

template <typename T>
class Swappable
{
  public:
    template <typename... Args>
        requires std::constructible_from<T, Args&...>
    explicit Swappable(Args&&... args)
        : _data{T(args...), T(args...)}
    {
    }

    Swappable(const Swappable&)            = default;
    Swappable(Swappable&&)                 = default;
    Swappable& operator=(const Swappable&) = default;
    Swappable& operator=(Swappable&&)      = default;

    void swap() { _index ^= 1; }

    T&       front() { return _data[_index]; }
    const T& front() const { return _data[_index]; }

    T&       back() { return _data[_index ^ 1]; }
    const T& back() const { return _data[_index ^ 1]; }

    T&       operator[](uint32_t index) { return _data[index]; }
    const T& operator[](uint32_t index) const { return _data[index]; }

    auto begin() { return _data.begin(); }
    auto begin() const { return _data.begin(); }

    auto end() { return _data.end(); }
    auto end() const { return _data.end(); }

    uint32_t frontIndex() const { return _index; }
    uint32_t backIndex() const { return _index ^ 1; }
    uint32_t size() const { return _data.size(); }

  private:
    uint32_t         _index = 0;
    std::array<T, 2> _data;
};