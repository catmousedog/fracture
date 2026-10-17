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
    glfwWindowHintString(GLFW_WAYLAND_APP_ID, "float"); // custom Hyprland rule

    return glfwCreateWindow(1280, 720, "fracture", nullptr, nullptr);
}

} // namespace

////////////////////////////////////////////////////////////

void Window::scrollCallback(GLFWwindow* glfwWindow, double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    window->_input.scroll += yoffset;
}

////////////////////////////////////////////////////////////

void Window::mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            window->_dragging = true;
            glfwGetCursorPos(glfwWindow, &window->_lastX, &window->_lastY);
        }
        else if (action == GLFW_RELEASE)
        {
            window->_dragging = false;
        }
    }
}

////////////////////////////////////////////////////////////

void Window::cursorPosCallback(GLFWwindow* glfwWindow, double xpos, double ypos)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    if (window->_dragging)
    {
        double dx      = xpos - window->_lastX;
        double dy      = ypos - window->_lastY;
        window->_lastX = xpos;
        window->_lastY = ypos;

        int w, h;
        glfwGetWindowSize(glfwWindow, &w, &h);
        double height = h;

        window->_input.dragX += 2 * dx / height;
        window->_input.dragY += -2 * dy / height;
    }
}

////////////////////////////////////////////////////////////

void Window::keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods)
{
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_LEFT_CONTROL)
            window->_input.ctrl = true;
        if (action == GLFW_PRESS && key == GLFW_KEY_W && (mods & GLFW_MOD_CONTROL))
            glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
    }
    else if (action == GLFW_RELEASE)
    {
        if (key == GLFW_KEY_LEFT_CONTROL)
            window->_input.ctrl = false;
    }
}

////////////////////////////////////////////////////////////

Window::Window()
    : _glfwWindow(createWindow())
{
    glfwSetWindowUserPointer(_glfwWindow, this);
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

Window::Input Window::consumeInput()
{
    Input input   = _input;
    _input.dragX  = 0.0;
    _input.dragY  = 0.0;
    _input.scroll = 0.0;
    return input;
}

////////////////////////////////////////////////////////////

vk::Extent2D Window::getSize()
{
    int width, height;
    glfwGetFramebufferSize(_glfwWindow, &width, &height);
    return vk::Extent2D(width, height);
}

////////////////////////////////////////////////////////////