#include "Fractal.hpp"

#include <array>
#include <utility>

namespace
{

std::array<string, 2> fractalNames = {"Mandelbrot", "BurningShip"};

}

string toString(FractalType type)
{
    return fractalNames[std::to_underlying(type)];
}