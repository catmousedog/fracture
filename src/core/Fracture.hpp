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

    /* ================== HELPER FUNCTIONS ================== */
  private:
    void updateFractal(const Window::Input& input);

    void updateBackend();

    void switchBackend();

    /* ====================== VARIABLES ===================== */
  private:
    // window that will use GLFW implementation
    Window _window;

    // data relevant to the fractal like position, zoom, iterations
    Fractal _fractal;

    VulkanCore _vulkanCore;

    // renderer that will use Vulkan implementation
    Renderer _renderer;

    // backend to calculate fractal outputs
    Backend::Type            _backendType, _prevBackendType;
    std::unique_ptr<Backend> _backend;

    // UI is agnostic to renderer or fractal
    UI _ui;
};