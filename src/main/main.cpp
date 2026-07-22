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

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    VulkanContext* vulkan     = static_cast<VulkanContext*>(glfwGetWindowUserPointer(window));
    float          zoomFactor = (yoffset > 0) ? 1.1f : 0.9f;
    vulkan->addZoom(zoomFactor);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    VulkanContext* vulkan = static_cast<VulkanContext*>(glfwGetWindowUserPointer(window));
    float          step   = 0.05f;
    if (key == GLFW_KEY_LEFT && action != GLFW_RELEASE)
        vulkan->addOffset(-step, 0);
    if (key == GLFW_KEY_RIGHT && action != GLFW_RELEASE)
        vulkan->addOffset(step, 0);
    if (key == GLFW_KEY_UP && action != GLFW_RELEASE)
        vulkan->addOffset(0, -step);
    if (key == GLFW_KEY_DOWN && action != GLFW_RELEASE)
        vulkan->addOffset(0, step);
    if (key == GLFW_KEY_KP_ADD && action != GLFW_RELEASE)
        vulkan->addIterations(1.1f);
    if (key == GLFW_KEY_KP_SUBTRACT && action != GLFW_RELEASE)
        vulkan->addIterations(0.9f);
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
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        vulkan.drawFrame();
    }

    vulkan.waitIdle();

    glfwDestroyWindow(window);
    glfwTerminate();
}