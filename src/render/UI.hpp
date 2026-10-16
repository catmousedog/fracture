#pragma once

#include "backend/Backend.hpp"
#include "fractals/Fractal.hpp"

class UI
{
  public:
    UI(Fractal& fractal);

    virtual ~UI() = default;

    /* ================== PUBLIC FUNCTIONS ================== */
  public:
    void draw(Backend::Type& type, const Backend::Stats& stats);

    /* ====================== VARIABLES ===================== */
  private:
    Fractal& _fractal;

    float _uiWidth   = 200.0f;
    bool  _collapsed = false;
};