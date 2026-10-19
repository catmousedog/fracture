#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "util/Basics.hpp"
#include "window/Window.hpp"

#include <optional>

class VulkanCore
{
    /* ======================== SETUP ======================= */
  public:
    VulkanCore(Window* window);

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

    // Sync objects
    void createSyncObjects();

    /* ================== PUBLIC FUNCTIONS ================== */
  public:
    void logInfo();

    std::optional<uint32_t> acquireNextImage(class GraphicsQueue* graphicsQueue);

    void prepareImage(class GraphicsQueue* graphicsQueue, uint32_t swapChainIndex);

    bool presentImage(uint32_t swapChainIndex);

    void waitIdle();

    void recreateSwapchain();

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    vk::raii::ShaderModule readShader(string shaderName) const;

    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
    createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);

    /* ================= GETTERS AND SETTERS ================ */
  public:
    Window*                   getWindow() const { return _window; }
    vk::raii::Instance&       getInstance() { return _instance; }
    vk::raii::Device&         getDevice() { return _device; }
    vk::raii::PhysicalDevice& getPhysicalDevice() { return _physicalDevice; }
    uint32_t                  getGraphiscsFamilyIndex() const { return _graphicsIndex; }
    uint32_t                  getComputeFamilyIndex() const { return _computeIndex; }
    vk::raii::Queue&          getGraphicsQueue() { return _graphicsQueue; }
    vk::raii::Queue&          getComputeQueue() { return _computeQueue; }
    vk::raii::SwapchainKHR&   getSwapchain() { return _swapchain; }
    vk::Extent2D              getSwapChainExtent() const { return _swapChainExtent; }
    vk::SurfaceFormatKHR&     getSurfaceFormat() { return _surfaceFormat; }
    vk::Image                 getSwapImage(uint32_t swapChainIndex) { return _swapImages[swapChainIndex]; }
    uint32_t                  getSwapImageCount() const { return _swapImageCount; }
    vk::raii::Semaphore&      getRenderFinishedSemaphore(uint32_t swapChainIndex)
    {
        return _renderFinishedSemaphores[swapChainIndex];
    }

    /* ====================== VARIABLES ===================== */
  private:
    // Window
    Window* _window = nullptr;

    // Core
    vk::raii::Context        _context;
    vk::raii::Instance       _instance       = nullptr;
    vk::raii::SurfaceKHR     _surface        = nullptr;
    vk::raii::PhysicalDevice _physicalDevice = nullptr;
    vk::raii::Device         _device         = nullptr;

#ifndef NDEBUG
    // Debug
    vk::raii::DebugUtilsMessengerEXT _debugMessenger = nullptr;
#endif

    // Queue
    uint32_t        _graphicsIndex = 0;
    vk::raii::Queue _graphicsQueue = nullptr;
    uint32_t        _computeIndex  = 0;
    vk::raii::Queue _computeQueue  = nullptr;

    // Swapchain
    vk::raii::SwapchainKHR      _swapchain       = nullptr;
    vk::Extent2D                _swapChainExtent = {};
    vk::SurfaceFormatKHR        _surfaceFormat   = {};
    vector<vk::Image>           _swapImages      = {};
    vector<vk::raii::ImageView> _swapImageViews  = {};
    vk::SurfaceCapabilitiesKHR  _surfaceCaps     = {};
    uint32_t                    _swapImageCount  = 0;

    // Sync objects
    // each image has its own semaphore to indicate it is ready for presentation
    vector<vk::raii::Semaphore> _renderFinishedSemaphores = {};
};
