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
    void createSwapchain();
    void createPipeline();
    void createCommandPool();
    void createCommandBuffer();
    void recordCommandBuffer(uint32_t imageIndex);
    void createSyncObjects();

    void transition_image_layout(
        uint32_t                imageIndex,
        vk::ImageLayout         old_layout,
        vk::ImageLayout         new_layout,
        vk::AccessFlags2        src_access_mask,
        vk::AccessFlags2        dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dst_stage_mask
    );
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
    uint32_t        _familyIndex = 0;
    vk::raii::Queue _queue       = nullptr;

    // Swapchain
    vk::raii::SwapchainKHR           _swapchain       = nullptr;
    vk::Extent2D                     _swapChainExtent = {};
    vk::SurfaceFormatKHR             _surfaceFormat   = {};
    uint32_t                         _imageCount      = 0;
    std::vector<vk::Image>           _swapImages      = {};
    std::vector<vk::raii::ImageView> _swapImageViews  = {};

    // Framebuffers & commands
    vk::raii::CommandPool   _commandPool   = nullptr;
    vk::raii::CommandBuffer _commandBuffer = nullptr;

    // pipeline
    vk::raii::ShaderModule   _shader         = nullptr;
    vk::raii::PipelineLayout _pipelineLayout = nullptr;
    vk::raii::Pipeline       _pipeline       = nullptr;

    // Sync objects
    vk::raii::Semaphore _presentCompleteSemaphore = nullptr;
    vk::raii::Semaphore _renderFinishedSemaphore  = nullptr;
    vk::raii::Fence     _drawFence                = nullptr;
};
