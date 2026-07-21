#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <vector>

#include "Vertices.hpp"

class GLFWwindow;

struct VulkanContextInfo
{
    uint32_t framesInFlight = 2;
};

class VulkanContext
{
  public:
    VulkanContext(GLFWwindow* window, const VulkanContextInfo& info);

    void drawFrame();
    void waitIdle();
    void recreateSwapchain();
    void resizeFramebuffer(uint32_t width, uint32_t height);

  private:
    void createInstance();
    void createDebugCallback();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void createPipeline();
    void createCommandPool();
    void createVertexBuffer();
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
    uint32_t           findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    PFN_vkVoidFunction getFunctionEXT(const char* funcName);

    // GLFW
    GLFWwindow* _window = nullptr;

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
    std::vector<vk::Image>           _swapImages      = {};
    std::vector<vk::raii::ImageView> _swapImageViews  = {};

    // Framebuffers & commands
    uint32_t                             _framesInFlight     = 0;
    uint32_t                             _frameIndex         = 0;
    vk::raii::CommandPool                _commandPool        = nullptr;
    std::vector<vk::raii::CommandBuffer> _commandBuffers     = {};
    bool                                 _frameBufferResized = false;

    // pipeline
    vk::raii::ShaderModule   _shader         = nullptr;
    vk::raii::PipelineLayout _pipelineLayout = nullptr;
    vk::raii::Pipeline       _pipeline       = nullptr;

    // Sync objects
    std::vector<vk::raii::Fence>     _drawFences                = {};
    std::vector<vk::raii::Semaphore> _renderFinishedSemaphores  = {};
    std::vector<vk::raii::Semaphore> _presentCompleteSemaphores = {};

    // Vertex buffer
    std::vector<Vertex> _vertices = {
        {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}}, //
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},  //
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}  //
    };
    vk::raii::Buffer       _vertexBuffer       = nullptr;
    vk::raii::DeviceMemory _vertexBufferMemory = nullptr;
};
