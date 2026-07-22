#include "VulkanContext.hpp"
#include <GLFW/glfw3.h>

constexpr uint32_t WIDTH  = 1280;
constexpr uint32_t HEIGHT = 720;

namespace
{

void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    VulkanContext* vulkan = static_cast<VulkanContext*>(glfwGetWindowUserPointer(window));
    vulkan->resizeFramebuffer(width, height);
}

} // namespace

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "fracture", nullptr, nullptr);

    VulkanContextInfo vulkanInfo{.framesInFlight = 2};
    VulkanContext     vulkan(window, vulkanInfo);
    vulkan.logInfo();

    glfwSetWindowUserPointer(window, &vulkan);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        vulkan.drawFrame();
    }

    vulkan.waitIdle();

    glfwDestroyWindow(window);
    glfwTerminate();
}