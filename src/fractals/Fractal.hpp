#pragma once

#include "util/Basics.hpp"

#include <cstdint>

enum class FractalType : uint32_t
{
    Mandelbrot,
    BurningShip
};

string toString(FractalType type);

struct Fractal
{
    FractalType type    = FractalType::Mandelbrot;
    float       offsetX = 0.0f, offsetY = 0.0f;
    float       zoom    = 1.0f;
    float       maxIter = 100;

    bool operator==(const Fractal& other) const = default;
};