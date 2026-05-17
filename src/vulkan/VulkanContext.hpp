#pragma once

#include <vector>
#include <vulkan/vulkan.h>

class GLFWwindow;

class VulkanContext
{
  public:
    VulkanContext(GLFWwindow* window, uint32_t width, uint32_t height);

    ~VulkanContext();

    void drawFrame();
    void waitIdle()
    {
        vkDeviceWaitIdle(_device);
    }

  private:
    void createInstance();
    void createDebugCallback();
    void createSurface(GLFWwindow* window);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain(uint32_t width, uint32_t height);
    void createRenderPass();
    void createPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    VkShaderModule createShaderModule(const char* path);

    // Core
    VkInstance       _instance       = VK_NULL_HANDLE;
    VkSurfaceKHR     _surface        = VK_NULL_HANDLE;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice         _device         = VK_NULL_HANDLE;

    // Debug
    VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;

    // Queues
    uint32_t _family = 0;
    VkQueue  queue   = VK_NULL_HANDLE;

    // Swapchain
    VkSwapchainKHR           _swapchain       = VK_NULL_HANDLE;
    VkFormat                 _swapchainFormat = {};
    VkExtent2D               _swapchainExtent = {};
    std::vector<VkImage>     _swapImages;
    std::vector<VkImageView> _swapImageViews;

    // Render pass & pipeline
    VkRenderPass     _renderPass     = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkPipeline       _pipeline       = VK_NULL_HANDLE;

    // Framebuffers & commands
    std::vector<VkFramebuffer>   _framebuffers;
    VkCommandPool                _commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> _commandBuffers;

    // Sync (double-buffered)
    static constexpr int     MAX_FRAMES_IN_FLIGHT = 2;
    int                      _currentFrame        = 0;
    std::vector<VkSemaphore> _imageAvailable;
    std::vector<VkSemaphore> _renderFinished;
    std::vector<VkFence>     _inFlightFences;
};