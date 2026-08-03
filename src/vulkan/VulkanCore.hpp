#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "util/Basics.hpp"
#include "window/Window.hpp"

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

    /* ==================== CAPABILITIES ==================== */
  public:
    void logInfo();

    std::optional<uint32_t> acquireNextImage(class CommandQueue* commandQueue);

    void prepare(class CommandQueue* commandQueue, uint32_t imageIndex);

    bool present(uint32_t imageIndex);

    void waitIdle();

    void recreateSwapchain();

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    vk::raii::ShaderModule readShader(string shaderName) const;

    /* ================= GETTERS AND SETTERS ================ */

    Window* getWindow() const
    {
        return _window;
    }
    vk::raii::Instance& getInstance()
    {
        return _instance;
    }
    vk::raii::Device& getDevice()
    {
        return _device;
    }
    vk::raii::PhysicalDevice& getPhysicalDevice()
    {
        return _physicalDevice;
    }
    uint32_t getFamilyIndex() const
    {
        return _familyIndex;
    }
    vk::raii::Queue& getQueue()
    {
        return _queue;
    }
    vk::raii::SwapchainKHR& getSwapchain()
    {
        return _swapchain;
    }
    vk::Extent2D getSwapChainExtent() const
    {
        return _swapChainExtent;
    }
    vk::SurfaceFormatKHR& getSurfaceFormat()
    {
        return _surfaceFormat;
    }
    vk::Image getSwapImage(uint32_t index)
    {
        return _swapImages[index];
    }
    uint32_t getSwapImageCount() const
    {
        return _swapImageCount;
    }
    vk::raii::Semaphore& getRenderFinishedSemaphore(uint32_t index)
    {
        return _renderFinishedSemaphores[index];
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
    uint32_t                    _swapImageCount  = 0;

    // Sync objects
    // each image has its own semaphore to indicate it is ready for presentation
    vector<vk::raii::Semaphore> _renderFinishedSemaphores = {};
};
