#pragma once

#include "Fractal.hpp"

class Mandelbrot : public Fractal
{
    /* ======================== SETUP ======================= */
  public:
    Mandelbrot();

    virtual ~Mandelbrot();

    /* ==================== CAPABILITIES ==================== */

    const PushConstants& getPushConstants() const override;

    constexpr string shaderFileName() const override;

    /* ====================== VARIABLES ===================== */
  private:
    PushConstants _pushConstants{
        .offsetX = 0.0f, //
        .offsetY = 0.0f, //
        .zoom    = 1.0f, //
        .maxIter = 100   //
    };
};