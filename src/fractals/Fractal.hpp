#pragma once

#include <concepts>
#include <cstring>
#include <glm/glm.hpp>
#include <span>
#include <string_view>
#include <variant>

template <typename F>
concept FractalType = requires(const F& fractal, const F& other, std::span<std::byte> bytes) {
    { F::name } -> std::convertible_to<std::string_view>;
    { fractal.operator==(other) } -> std::same_as<bool>;
    { fractal.size() } -> std::same_as<size_t>;
    { fractal.pack(bytes) } -> std::same_as<void>;
};

struct Mandelbrot
{
    static constexpr std::string_view name                                      = "Mandelbrot";
    bool                              operator==(const Mandelbrot& other) const = default;
    size_t                            size() const { return 0; }
    void                              pack(std::span<std::byte> dst) const { }
};

struct Burningship
{
    static constexpr std::string_view name                                       = "Burningship";
    bool                              operator==(const Burningship& other) const = default;
    size_t                            size() const { return 0; }
    void                              pack(std::span<std::byte> dst) const { }
};

struct Julia
{
    struct UBO
    {
        glm::dvec2 c{};

        bool operator==(const UBO& other) const = default;
    } ubo;

    static constexpr std::string_view name                                 = "Julia";
    bool                              operator==(const Julia& other) const = default;
    size_t                            size() const { return sizeof(UBO); }
    void                              pack(std::span<std::byte> dst) const { std::memcpy(dst.data(), &ubo, size()); }
};

class Fractal
{
  private:
    // check all Variant types are FractalType
    template <typename V>
    static constexpr bool allFractalType = false;

    template <typename... Fs>
    static constexpr bool allFractalType<std::variant<Fs...>> = (FractalType<Fs> && ...);

    using Variant = std::variant<Mandelbrot, Burningship, Julia>;
    static_assert(allFractalType<Variant>);

    Variant variant;

  public:
    template <FractalType F>
    Fractal(const F& f)
        : variant(f)
    {
    }

    bool operator==(const Fractal& other) const = default;

    size_t index() const { return variant.index(); }

    std::string_view name() const
    {
        return std::visit([](const auto& v) { return v.name; }, variant);
    }

    size_t size() const
    {
        return std::visit([](const auto& v) { return v.size(); }, variant);
    }

    void pack(std::span<std::byte> dst) const
    {
        std::visit([&dst](const auto& v) { v.pack(dst); }, variant);
    }

    template <size_t N = std::variant_size_v<Variant>>
    static constexpr std::array<std::string_view, N> variantNameList()
    {
        constexpr auto indices = std::make_index_sequence<N>();

        return []<std::size_t... I>(std::index_sequence<I...>)
        {
            return std::array<std::string_view, N>{std::variant_alternative_t<I, Variant>().name...};
        }(indices);
    }
};
