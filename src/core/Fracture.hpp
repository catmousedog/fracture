#pragma once

#include "backend/Backend.hpp"
#include "fractals/Fractal.hpp"
#include "render/Renderer.hpp"
#include "render/UI.hpp"
#include "window/Window.hpp"

class Fracture
{
    /* ======================== SETUP ======================= */
  public:
    Fracture();

    /* ================== PUBLIC FUNCTIONS ================== */

    void mainLoop();

    /* ====================== VARIABLES ===================== */
  private:
    // window that will use GLFW implementation
    Window _window;

    // UI is agnostic to renderer or fractal
    UI _ui;

    // data relevant to the fractal like position, zoom, iterations
    Fractal _fractal;

    VulkanCore _vulkanCore;

    // renderer that will use Vulkan implementation
    Renderer _renderer;

    // backend to calculate fractal outputs
    std::unique_ptr<Backend> _backend;
};