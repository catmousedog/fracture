#pragma once

#include <array>
#include <cstdint>
#include <string>

// generalize into smart enum template?

struct Fractal
{
    // plain enum uses enclosing scope Fractal::
    enum Value : uint32_t
    {
        Mandelbrot,
        BurningShip
    };

    // make type non-aggregate
    constexpr Fractal(Value v)
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
        }
        return "";
    }

    Value value;

    static constexpr std::array<Value, 2> all{Mandelbrot, BurningShip};
};
static_assert(sizeof(Fractal) == sizeof(uint32_t));
