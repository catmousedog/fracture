#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "Basics.hpp"
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

    void logInfo();

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
    void createIndexBuffer();
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
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
    createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    vk::raii::CommandBuffer beginSingleTimeCommands();
    void                    endSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer);
    PFN_vkVoidFunction      getFunctionEXT(const char* funcName);

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
    vk::raii::SwapchainKHR      _swapchain       = nullptr;
    vk::Extent2D                _swapChainExtent = {};
    vk::SurfaceFormatKHR        _surfaceFormat   = {};
    vector<vk::Image>           _swapImages      = {};
    vector<vk::raii::ImageView> _swapImageViews  = {};

    // Framebuffers & commands
    uint32_t                        _framesInFlight     = 0;
    uint32_t                        _frameIndex         = 0;
    vk::raii::CommandPool           _commandPool        = nullptr;
    vector<vk::raii::CommandBuffer> _commandBuffers     = {};
    bool                            _frameBufferResized = false;

    // pipeline
    vk::raii::ShaderModule   _shader         = nullptr;
    vk::raii::PipelineLayout _pipelineLayout = nullptr;
    vk::raii::Pipeline       _pipeline       = nullptr;

    // Sync objects
    vector<vk::raii::Fence>     _drawFences                = {};
    vector<vk::raii::Semaphore> _renderFinishedSemaphores  = {};
    vector<vk::raii::Semaphore> _presentCompleteSemaphores = {};

    // Vertex buffer
    std::vector<Vertex> _vertices = {
        {{-1.f, -1.f}}, //
        {{1.f, -1.f}},  //
        {{1.f, 1.f}},   //
        {{-1.f, 1.f}}   //
    };
    vk::raii::Buffer       _vertexBuffer       = nullptr;
    vk::raii::DeviceMemory _vertexBufferMemory = nullptr;
    vector<uint16_t>       _indices            = {0, 1, 2, 2, 3, 0};
    vk::raii::Buffer       _indexBuffer        = nullptr;
    vk::raii::DeviceMemory _indexBufferMemory  = nullptr;
};
