#pragma once

#include "core/fractals/Fractal.hpp"
#include "render/Renderer.hpp"
#include "window/Window.hpp"

class Fracture
{
    /* ======================== SETUP ======================= */
  public:
    Fracture();

    /* ==================== CAPABILITIES ==================== */

    void mainLoop();

    /* ====================== VARIABLES ===================== */
  private:
    std::unique_ptr<Fractal> _fractal;
    Window                   _window;
    Renderer                 _renderer;
};