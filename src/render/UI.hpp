#pragma once

#include "fractals/Fractal.hpp"

class UI
{
  public:
    UI(Fractal& fractal);

    virtual ~UI() = default;

    /* ==================== CAPABILITIES ==================== */
  public:
    void draw();

    /* ====================== VARIABLES ===================== */
  private:
    Fractal& _fractal;

    float _uiWidth   = 200.0f;
    bool  _collapsed = false;
};