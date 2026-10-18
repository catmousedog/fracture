#pragma once

#include <vulkan/vulkan.hpp>

namespace vk::raii
{

class SurfaceKHR;
class Instance;

} // namespace vk::raii

class Window
{
    /* ==================== NESTED CLASS ==================== */
  public:
    struct Input
    {
        float dragX = 0.0, dragY = 0.0;
        float mouseX = 0.0, mouseY = 0.0;
        float scroll = 0.0;
        bool  ctrl   = false; // ctrl while scrolling
    };

    /* ======================== SETUP ======================= */

    Window();

    virtual ~Window();

    /* ================== PUBLIC FUNCTIONS ================== */
    bool shouldClose();

    void pollEvents();

    std::vector<const char*> getRequiredExtensions();

    vk::raii::SurfaceKHR createSurface(vk::raii::Instance& instance);

    /* ================= GETTERS AND SETTERS ================ */

    Input consumeInput();

    vk::Extent2D getSize();

    class GLFWwindow* getGLFWWindow() { return _glfwWindow; }

    /* ================== HELPER FUNCTIONS ================== */
  private:
    static void scrollCallback(GLFWwindow* glfwWindow, double xoffset, double yoffset);
    static void mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* glfwWindow, double xpos, double ypos);
    static void keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods);

    /* ====================== VARIABLES ===================== */

    class GLFWwindow* _glfwWindow = nullptr;

    bool   _dragging = false;          // mouse dragging flag
    double _lastX = 0.0, _lastY = 0.0; // previous mouse position

    Input _input;
};