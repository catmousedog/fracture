#include "Window.hpp"
#include "util/Log.hpp"

#include <vulkan/vulkan_raii.hpp>

#include "imgui.h"

#include <GLFW/glfw3.h>

////////////////////////////////////////////////////////////

namespace
{

GLFWwindow* createWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    return glfwCreateWindow(1280, 720, "fracture", nullptr, nullptr);
}

////////////////////////////////////////////////////////////

void framebufferResizeCallback(GLFWwindow* glfwWindow, int width, int height)
{
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    window->dirty  = true;
}

void scrollCallback(GLFWwindow* glfwWindow, double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

    if (window->ctrl)
    {
        float maxIter    = window->getSettings()->maxIter;
        float iterFactor = (yoffset < 0) ? 1 / 1.1f : 1.1f;
        maxIter *= iterFactor;
        window->getSettings()->maxIter = std::max(maxIter, 1.f);
        window->dirty                  = true;
    }
    else
    {
        float zoomFactor = (yoffset < 0) ? 1.1f : 1.f / 1.1f;
        window->getSettings()->zoom *= zoomFactor;
        window->dirty = true;
    }
}

void mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            window->dragging = true;
            glfwGetCursorPos(glfwWindow, &window->lastX, &window->lastY);
        }
        else if (action == GLFW_RELEASE)
        {
            window->dragging = false;
        }
    }
}

void cursorPosCallback(GLFWwindow* glfwWindow, double xpos, double ypos)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    if (window->dragging)
    {
        double dx     = xpos - window->lastX;
        double dy     = ypos - window->lastY;
        window->lastX = xpos;
        window->lastY = ypos;

        int w, h;
        glfwGetWindowSize(glfwWindow, &w, &h);
        double height = h;

        float ux   = 2 * dx / height;
        float uy   = -2 * dy / height;
        float zoom = window->getSettings()->zoom;

        window->getSettings()->offsetX -= ux * zoom;
        window->getSettings()->offsetY -= uy * zoom;
        window->dirty = true;
    }
}

void keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods)
{
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_LEFT_CONTROL)
            window->ctrl = true;
    }
    else if (action == GLFW_RELEASE)
    {
        if (key == GLFW_KEY_LEFT_CONTROL)
            window->ctrl = false;
    }
}

} // namespace

////////////////////////////////////////////////////////////

Window::Window(Fractal::Settings* settings)
    : _glfwWindow(createWindow()),
      _settings(settings)
{
    glfwSetWindowUserPointer(_glfwWindow, this);
    glfwSetFramebufferSizeCallback(_glfwWindow, framebufferResizeCallback);

    glfwSetScrollCallback(_glfwWindow, scrollCallback);
    glfwSetMouseButtonCallback(_glfwWindow, mouseButtonCallback);
    glfwSetCursorPosCallback(_glfwWindow, cursorPosCallback);
    glfwSetKeyCallback(_glfwWindow, keyCallback);
}

////////////////////////////////////////////////////////////

Window::~Window()
{
    glfwDestroyWindow(_glfwWindow);
    glfwTerminate();
}

////////////////////////////////////////////////////////////

bool Window::shouldClose()
{
    return glfwWindowShouldClose(_glfwWindow);
}

////////////////////////////////////////////////////////////

void Window::pollEvents()
{
    glfwPollEvents();
}

////////////////////////////////////////////////////////////

std::vector<const char*> Window::getRequiredExtensions()
{
    uint32_t     glfwExtensionsCount = 0;
    const char** glfwExtensions      = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
    return std::vector(glfwExtensions, glfwExtensions + glfwExtensionsCount);
}

////////////////////////////////////////////////////////////

[[nodiscard]] vk::raii::SurfaceKHR Window::createSurface(vk::raii::Instance& instance)
{
    // GLFW cannot use raii
    VkSurfaceKHR surface;
    VkResult     res = glfwCreateWindowSurface(*instance, _glfwWindow, nullptr, &surface);
    if (res != VK_SUCCESS)
        FATAL("glfwCreateWindowSurface failed");

    // guaranteed copy elision
    return vk::raii::SurfaceKHR(instance, surface);
}

////////////////////////////////////////////////////////////

std::pair<uint32_t, uint32_t> Window::getSize()
{
    int width, height;
    glfwGetFramebufferSize(_glfwWindow, &width, &height);
    return {width, height};
}

////////////////////////////////////////////////////////////