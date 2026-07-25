#include "Window.hpp"
#include "util/Log.hpp"

#include <vulkan/vulkan_raii.hpp>

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
    window->setResized(true);
}

} // namespace

////////////////////////////////////////////////////////////

Window::Window()
    : _glfwWindow(createWindow())
{
    glfwSetWindowUserPointer(_glfwWindow, this);
    glfwSetFramebufferSizeCallback(_glfwWindow, framebufferResizeCallback);
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

void Window::setResized(bool resized)
{
    _resized = resized;
}

////////////////////////////////////////////////////////////

bool Window::wasResized() const
{
    return _resized;
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

    // copy elision
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

GLFWwindow* Window::getWindow()
{
    return _glfwWindow;
}

////////////////////////////////////////////////////////////