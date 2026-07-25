#pragma once

#include "Fractal.hpp"

class Mandelbrot : public Fractal
{

  public:
    Mandelbrot();

    virtual ~Mandelbrot();

    constexpr string shaderFileName() const override;
};