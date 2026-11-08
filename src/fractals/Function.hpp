#pragma once

#include <array>
#include <cstdint>
#include <string>

struct Function
{
    // plain enum uses enclosing scope Function::
    enum Value : uint32_t
    {
        Iterative,
        Normalized
    };

    // make type non-aggregate
    constexpr Function(Value v)
        : value(v)
    {
    }

    // allow implicit conversion
    constexpr operator Value() const { return value; }

    constexpr std::string toString() const
    {
        switch (value)
        {
        case Iterative:
            return "Iterative";
        case Normalized:
            return "Normalized";
        }
        return "";
    }

    Value value;

    static constexpr std::array<Value, 2> all{Iterative, Normalized};
};
static_assert(sizeof(Function) == sizeof(uint32_t));
