#pragma once

#include <vulkan/vulkan.hpp>

namespace vk::raii
{

class SurfaceKHR;
class Instance;

} // namespace vk::raii

class Window
{
    /* ======================== SETUP ======================= */
  public:
    Window();

    virtual ~Window();

    /* ==================== CAPABILITIES ==================== */
    bool shouldClose();

    void pollEvents();

    std::vector<const char*> getRequiredExtensions();

    vk::raii::SurfaceKHR createSurface(vk::raii::Instance& instance);

    /* ================= GETTERS AND SETTERS ================ */

    vk::Extent2D getSize();

    class GLFWwindow* getGLFWWindow()
    {
        return _glfwWindow;
    }

    /* ====================== VARIABLES ===================== */
  private:
    class GLFWwindow* _glfwWindow = nullptr;

  public:
    bool   dragging = false;         // mouse dragging flag
    bool   ctrl     = false;         // ctrl key held
    double lastX = 0.0, lastY = 0.0; // previous mouse position

    double deltaX = 0.0, deltaY = 0.0; // drag delta
};