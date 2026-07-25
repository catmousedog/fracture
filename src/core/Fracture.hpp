#pragma once

#include "core/fractals/Fractal.hpp"
#include "render/Renderer.hpp"
#include "ui/UI.hpp"
#include "window/Window.hpp"

class Fracture
{

  public:
    Fracture();

    void mainLoop();

  private:
    std::unique_ptr<Fractal> _fractal;
    Window                   _window;
    UI                       _ui;
    Renderer                 _renderer;
};