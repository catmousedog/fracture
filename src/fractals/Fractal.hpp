#pragma once

#include "fractals/Precision.hpp"

#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <span>
#include <string>
#include <variant>

struct FractalType
{
    // plain enum uses enclosing scope Fractal::
    enum Value : uint32_t
    {
        Mandelbrot,
        BurningShip,
        Julia
    };

    // make type non-aggregate
    constexpr FractalType(Value v)
        : value(v)
    {
    }

    // allow implicit conversion
    constexpr operator Value() const { return value; }

    constexpr std::string toString() const
    {
        switch (value)
        {
        case Mandelbrot:
            return "Mandelbrot";
        case BurningShip:
            return "BurningShip";
        case Julia:
            return "Julia";
        }
        return "";
    }

    Value value;

    static constexpr std::array all{Mandelbrot, BurningShip, Julia};
};
static_assert(sizeof(FractalType) == sizeof(uint32_t));

class Fractal
{
  public:
    template <size_t size>
    struct UniformBufferObject
    {
    };

  protected:
    Fractal(FractalType type)
        : _type(type)
    {
    }

  public:
    FractalType type() { return _type; }

    virtual size_t size() const { return baseSize(); };
    virtual void   pack(std::span<std::byte> dst) const { basePack(dst); }

  protected:
    size_t baseSize() const { return sizeof(Fractal); }
    void   basePack(std::span<std::byte> dst) const { }

    FractalType _type;
    real        _offsetX;
    real        _offsetY;
    real        _zoom;
    int         _maxIter;
};

class Mandelbrot : public Fractal
{
  public:
    Mandelbrot()
        : Fractal(FractalType::Mandelbrot)
    {
    }
};

struct Burningship
{
    static constexpr FractalType type() { return FractalType::BurningShip; }
    size_t                       size() const { return 0; }
    void                         pack(std::span<std::byte> dst) const { }
};

struct Julia
{
    glm::dvec2 c{};

    static constexpr FractalType type() { return FractalType::Julia; }
    size_t                       size() const { return 0; }
    void                         pack(std::span<std::byte> dst) const { }
};

using FractalData = std::variant<Mandelbrot, Burningship, Julia>;
