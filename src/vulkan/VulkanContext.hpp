#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <vector>

class GLFWwindow;

class VulkanContext
{
  public:
    VulkanContext(GLFWwindow* window, uint32_t width, uint32_t height);

    void drawFrame();
    void waitIdle();

  private:
    void createInstance();
    void createDebugCallback();
    void createSurface(GLFWwindow* window);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain(uint32_t width, uint32_t height);
    void createCommandpool();
    void createCommandBuffers();
    void recordCommandBuffers();
    void createPipeline();

    PFN_vkVoidFunction getFunctionEXT(const char* funcName);

    // Core
    vk::raii::Context        _context;
    vk::raii::Instance       _instance       = nullptr;
    vk::raii::SurfaceKHR     _surface        = nullptr;
    vk::raii::PhysicalDevice _physicalDevice = nullptr;
    vk::raii::Device         _device         = nullptr;

    // Debug
    vk::raii::DebugUtilsMessengerEXT _debugMessenger = nullptr;

    // Queues
    uint32_t        _family = 0;
    vk::raii::Queue _queue  = nullptr;

    // Swapchain
    vk::raii::SwapchainKHR           _swapchain      = nullptr;
    uint32_t                         _imageCount     = 0;
    std::vector<vk::Image>           _swapImages     = {};
    std::vector<vk::raii::ImageView> _swapImageViews = {};

    // Framebuffers & commands
    vk::raii::CommandPool                _commandPool    = nullptr;
    std::vector<vk::raii::CommandBuffer> _commandBuffers = {};

    // pipeline
    vk::raii::ShaderModule   _shader         = nullptr;
    vk::raii::PipelineLayout _pipelineLayout = nullptr;
    vk::raii::Pipeline       _pipeline       = nullptr;
};
