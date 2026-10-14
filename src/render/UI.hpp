#pragma once

#include "backend/Backend.hpp"
#include "fractals/Fractal.hpp"

class UI
{
  public:
    UI(Fractal& fractal, Backend::Type& backendType);

    virtual ~UI() = default;

    /* ================== PUBLIC FUNCTIONS ================== */
  public:
    void draw();

    /* ====================== VARIABLES ===================== */
  private:
    Fractal&       _fractal;
    Backend::Type& _backendType;

    float _uiWidth   = 200.0f;
    bool  _collapsed = false;
};