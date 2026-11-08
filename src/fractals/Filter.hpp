#pragma once

#include <array>
#include <cstdint>
#include <string>

struct Filter
{
    // plain enum uses enclosing scope Filter::
    enum Value : uint32_t
    {
        GrayScale
    };

    // make type non-aggregate
    constexpr Filter(Value v)
        : value(v)
    {
    }

    // allow implicit conversion
    constexpr operator Value() const { return value; }

    constexpr std::string toString() const
    {
        switch (value)
        {
        case GrayScale:
            return "GrayScale";
        }
        return "";
    }

    Value value;

    static constexpr std::array<Value, 1> all{GrayScale};
};
static_assert(sizeof(Filter) == sizeof(uint32_t));
