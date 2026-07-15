#include "VulkanContext.hpp"
#include <GLFW/glfw3.h>

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "fracture", nullptr, nullptr);

    VulkanContext vulkan(window, 1280, 720);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        vulkan.drawFrame();
    }

    vulkan.waitIdle();
    glfwDestroyWindow(window);
    glfwTerminate();
}