#include "Window.hpp"
#include "util/Log.hpp"

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>
#include <string_view>

////////////////////////////////////////////////////////////

namespace
{

constexpr uint32_t MIN_WIDTH  = 320;
constexpr uint32_t MIN_HEIGHT = 240;
constexpr uint32_t WIDTH      = 1280;
constexpr uint32_t HEIGHT     = 720;

GLFWwindow* createWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHintString(GLFW_WAYLAND_APP_ID, "float"); // custom Hyprland rule

    return glfwCreateWindow(WIDTH, HEIGHT, "Fracture", nullptr, nullptr);
}

} // namespace

////////////////////////////////////////////////////////////

void Window::scrollCallback(GLFWwindow* glfwWindow, double xoffset, double yoffset)
{
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
    window->_input.scroll += yoffset;
    window->_input.ctrl = glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                          glfwGetKey(glfwWindow, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
}

////////////////////////////////////////////////////////////

void Window::mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods)
{
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
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

    int w, h;
    glfwGetWindowSize(glfwWindow, &w, &h);

    window->_input.mouseX = (2.0 * xpos - w) / static_cast<float>(h);  // [-aspect, aspect]
    window->_input.mouseY = -(2.0 * ypos - h) / static_cast<float>(h); // [-1, 1], up is +

    if (window->_dragging)
    {
        double dx      = xpos - window->_lastX;
        double dy      = ypos - window->_lastY;
        window->_lastX = xpos;
        window->_lastY = ypos;

        window->_input.dragX += 2 * dx / static_cast<float>(h);
        window->_input.dragY += -2 * dy / static_cast<float>(h);
    }
}

////////////////////////////////////////////////////////////

void Window::keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods)
{
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

    if (action == GLFW_PRESS)
    {
        const char* name = glfwGetKeyName(key, scancode);
        if (name && std::string_view(name) == "w" && (mods & GLFW_MOD_CONTROL))
            glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
    }
}

////////////////////////////////////////////////////////////

Window::Window()
    : _glfwWindow(createWindow())
{
    glfwSetWindowSizeLimits(_glfwWindow, MIN_WIDTH, MIN_HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);
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