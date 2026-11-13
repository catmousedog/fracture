#pragma once

#include <string>

// using real = float;
using real = double;

constexpr std::string real_name()
{
    if constexpr (std::is_same_v<real, float>)
        return "float";
    else if constexpr (std::is_same_v<real, double>)
        return "double";
    else
        return "";
}
