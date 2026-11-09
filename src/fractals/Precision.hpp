#pragma once

#include <array>
#include <cstdint>
#include <string>

// generalize into smart enum template?

struct Precision
{
    // plain enum uses enclosing scope Fractal::
    enum Value : uint32_t
    {
        Float,
        Double
    };

    // make type non-aggregate
    constexpr Precision(Value v)
        : value(v)
    {
    }

    // allow implicit conversion
    constexpr operator Value() const { return value; }

    constexpr std::string toString() const
    {
        switch (value)
        {
        case Float:
            return "float";
        case Double:
            return "double";
        }
        return "";
    }

    Value value;

    static constexpr std::array<Value, 2> all{Float, Double};
};
static_assert(sizeof(Precision) == sizeof(uint32_t));
