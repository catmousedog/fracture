#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "Vertices.hpp"
#include "util/Basics.hpp"
#include "window/Window.hpp"

class VulkanContext
{
  public:
    VulkanContext(Window* window);

    void logInfo();
    void drawFrame();
    void recreateSwapchain();

  private:
    // Core
    void createInstance();
    void createDebugCallback();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();

    // Swapchain
    void createSurfaceFormat();
    void createSwapchain();

    // Descriptor Layout
    void createGraphicsDescriptorLayout();
    void createComputeDescriptorLayout();

    // Pipeline
    void createGraphicsPipeline();
    void createComputePipeline();

    // Command buffers
    void createCommandPool();
    void createCommandBuffer();
    void recordCommandBuffer(uint32_t imageIndex);

    // Compute Image
    void createComputeImage();
    void createSampler();

    // Buffers
    void createVertexBuffer();
    void createIndexBuffer();

    // Descriptors
    void createDescriptorPool();
    void createUiDescriptorPool();
    void createGraphicsDescriptorSets();
    void createComputeDescriptorSets();

    // Sync objects
    void createSyncObjects();

    // UI
    void initImGUI();

    /* ================== Helper Functions ================== */
    void transition_image_layout(
        const vk::Image&        image,
        vk::ImageLayout         old_layout,
        vk::ImageLayout         new_layout,
        vk::AccessFlags2        src_access_mask,
        vk::AccessFlags2        dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dst_stage_mask
    );
    void transitionImageLayout(
        vk::raii::CommandBuffer& commandBuffer,
        const vk::raii::Image&   image,
        vk::ImageLayout          oldLayout,
        vk::ImageLayout          newLayout
    );
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
         createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    void singleCommand(std::function<void(vk::raii::CommandBuffer&)> command);
    vk::raii::ShaderModule readShader(string fileName) const;
    PFN_vkVoidFunction     getFunctionEXT(const char* funcName);

    // Window
    Window* _window = nullptr;

    // Core
    vk::raii::Context        _context;
    vk::raii::Instance       _instance       = nullptr;
    vk::raii::SurfaceKHR     _surface        = nullptr;
    vk::raii::PhysicalDevice _physicalDevice = nullptr;
    vk::raii::Device         _device         = nullptr;

    // Debug
    vk::raii::DebugUtilsMessengerEXT _debugMessenger = nullptr;

    // Queue
    uint32_t        _familyIndex = 0;
    vk::raii::Queue _queue       = nullptr;

    // Swapchain
    vk::raii::SwapchainKHR      _swapchain       = nullptr;
    vk::Extent2D                _swapChainExtent = {};
    vk::SurfaceFormatKHR        _surfaceFormat   = {};
    vector<vk::Image>           _swapImages      = {};
    vector<vk::raii::ImageView> _swapImageViews  = {};
    vk::SurfaceCapabilitiesKHR  _surfaceCaps     = {};
    uint32_t                    _imageCount      = 0;

    // Pipeline
    vk::raii::PipelineLayout _graphicsPipelineLayout = nullptr;
    vk::raii::PipelineLayout _computePipelineLayout  = nullptr;
    vk::raii::Pipeline       _graphicsPipeline       = nullptr;
    vk::raii::Pipeline       _computePipeline        = nullptr;

    // Command buffers
    uint32_t                        _framesInFlight = 2;
    uint32_t                        _frameIndex     = 0;
    vk::raii::CommandPool           _commandPool    = nullptr;
    vector<vk::raii::CommandBuffer> _commandBuffers = {};

    // Compute image
    vk::raii::Image        _computeImage       = nullptr;
    vk::raii::DeviceMemory _computeImageMemory = nullptr;
    vk::raii::ImageView    _computeImageView   = nullptr;
    vk::raii::Sampler      _sampler            = nullptr;
    bool                   _dirty              = true;

    // Vertex buffer
    std::vector<Vertex> _vertices = {
        {{-1.f, -1.f}, {1.0f, 0.0f}},
        {{1.f, -1.f}, {0.0f, 0.0f}},
        {{1.f, 1.f}, {0.0f, 1.0f}},
        {{-1.f, 1.f}, {1.0f, 1.0f}}
    };
    vk::raii::Buffer       _vertexBuffer       = nullptr;
    vk::raii::DeviceMemory _vertexBufferMemory = nullptr;

    // Index buffer
    vector<uint16_t>       _indices           = {0, 1, 2, 2, 3, 0};
    vk::raii::Buffer       _indexBuffer       = nullptr;
    vk::raii::DeviceMemory _indexBufferMemory = nullptr;

    // Descriptors
    vk::raii::DescriptorPool      _descriptorPool              = nullptr;
    vk::raii::DescriptorPool      _uiDescriptorPool            = nullptr;
    vk::raii::DescriptorSetLayout _graphicsDescriptorSetLayout = nullptr;
    vk::raii::DescriptorSetLayout _computeDescriptorSetLayout  = nullptr;
    vk::raii::DescriptorSet       _graphicsDescriptorSet       = nullptr;
    vk::raii::DescriptorSet       _computeDescriptorSet        = nullptr;

    // Sync objects
    vector<vk::raii::Fence>     _drawFences                = {};
    vector<vk::raii::Semaphore> _renderFinishedSemaphores  = {};
    vector<vk::raii::Semaphore> _presentCompleteSemaphores = {};
};
