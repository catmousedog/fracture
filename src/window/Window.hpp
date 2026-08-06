#pragma once

#include "core/fractals/Fractal.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace vk::raii
{

class SurfaceKHR;
class Instance;

} // namespace vk::raii

class Window
{
    /* ======================== SETUP ======================= */
  public:
    Window(Fractal::Settings* settings);

    virtual ~Window();

    /* ==================== CAPABILITIES ==================== */
    bool shouldClose();

    void pollEvents();

    std::vector<const char*> getRequiredExtensions();

    vk::raii::SurfaceKHR createSurface(vk::raii::Instance& instance);

    /* ================= GETTERS AND SETTERS ================ */

    std::pair<uint32_t, uint32_t> getSize();

    class GLFWwindow* getGLFWWindow()
    {
        return _glfwWindow;
    }

    Fractal::Settings* getSettings()
    {
        return _settings;
    }

    /* ====================== VARIABLES ===================== */
  private:
    class GLFWwindow*  _glfwWindow = nullptr;
    Fractal::Settings* _settings   = nullptr;

  public:
    bool   dirty    = false;         // recompute flag
    bool   dragging = false;         // mouse dragging flag
    bool   ctrl     = false;         // ctrl key held
    double lastX = 0.0, lastY = 0.0; // previous mouse position
};