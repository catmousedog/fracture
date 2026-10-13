#pragma once

struct Fractal
{
    float offsetX = 0.0f, offsetY = 0.0f;
    float zoom    = 1.0f;
    float maxIter = 100;

    bool operator==(const Fractal& other) const = default;
};