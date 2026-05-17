#include "VulkanContext.hpp"
#include <algorithm>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <vulkan/vk_enum_string_helper.h>

#include "Basics.hpp"
#include "Log.hpp"

#include <cstdint>
#include <fstream>
#include <vector>

namespace
{

vector<char> readFile(const char* path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        FATAL("Failed to open file: {}", path);
    size_t       size = file.tellg();
    vector<char> buf(size);
    file.seekg(0);
    file.read(buf.data(), size);
    return buf;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*                                       pUserData
)
{
    const char* msg        = pCallbackData->pMessage;
    uint32_t    objectType = messageTypes;

    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        Log::info("Validation layer VERBOSE {}: {}", objectType, msg);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        Log::info("Validation layer INFO {}: {}", objectType, msg);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        Log::warn("Validation layer WARN {}: {}", objectType, msg);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        Log::error("Validation layer ERROR {}: {}", objectType, msg);
        break;
    default:
        Log::error("Validation layer DEFAULT {}: {}", objectType, msg);
        break;
    }

    return VK_FALSE;
}

} // namespace

VulkanContext::VulkanContext(GLFWwindow* window, uint32_t width, uint32_t height)
{
    createInstance();
    createDebugCallback();
    createSurface(window);
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain(width, height);
    createRenderPass();
    createPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

void VulkanContext::createInstance()
{
    VkApplicationInfo appInfo{
        .sType      = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_4,
    };

    // --- GLFW instance extensions --- //
    uint32_t            glfwExtensionsCount = 0;
    const char**        glfwExtensions      = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
    vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionsCount);
    extensions.push_back("VK_EXT_debug_utils");
    uint32_t extensionsCount = static_cast<uint32_t>(extensions.size());

    const char* validationLayer = "VK_LAYER_KHRONOS_validation";

    // --- Create instance --- //
    VkInstanceCreateInfo info{
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = 1,
        .ppEnabledLayerNames     = &validationLayer,
        .enabledExtensionCount   = extensionsCount,
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkResult res = vkCreateInstance(&info, nullptr, &_instance);

    if (res != VK_SUCCESS)
        FATAL("vkCreateInstance failed: {}", string_VkResult(res));
}

void VulkanContext::createDebugCallback()
{
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT,
        .pfnUserCallback = &debugCallback,
    };

    auto vkCreateDebugUtilsMessengerEXT =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
    if (!vkCreateDebugUtilsMessengerEXT)
        FATAL("vkCreateDebugCallback not found");
    VkResult res = vkCreateDebugUtilsMessengerEXT(_instance, &debugMessengerCreateInfo, nullptr, &_debugMessenger);

    if (res != VK_SUCCESS)
        FATAL("vkCreateDebugCallback failed: {}", string_VkResult(res));
}

void VulkanContext::createSurface(GLFWwindow* window)
{
    // --- Create Vulkan surface --- //
    VkResult res = glfwCreateWindowSurface(_instance, window, nullptr, &_surface);

    if (res != VK_SUCCESS)
        FATAL("glfwCreateWindowSurface failed");
}

void VulkanContext::pickPhysicalDevice()
{
    // --- Pick physical device --- //
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(_instance, &count, nullptr);
    vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(_instance, &count, devices.data());

    if (count == 0)
        FATAL("No Vulkan GPU found");

    _physicalDevice = devices[0];
}

void VulkanContext::createLogicalDevice()
{
    // --- Obtain queue family properties --- //
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, nullptr);
    vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, queueFamilies.data());

    // --- Find queue family with graphics and present support --- //
    _family = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice, i, _surface, &presentSupport);
        if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport)
        {
            _family = i;
            break;
        }
    }
    if (_family == UINT32_MAX)
        FATAL("No graphics+present queue family found");

    // --- Create logical device --- //
    float                   priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = _family;
    queueInfo.queueCount       = 1;
    queueInfo.pQueuePriorities = &priority;

    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkDeviceCreateInfo info{};
    info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.queueCreateInfoCount    = 1;
    info.pQueueCreateInfos       = &queueInfo;
    info.enabledExtensionCount   = 1;
    info.ppEnabledExtensionNames = deviceExtensions;

    VkResult res = vkCreateDevice(_physicalDevice, &info, nullptr, &_device);

    if (res != VK_SUCCESS)
        FATAL("vkCreateDevice failed");

    // --- Obtain queue --- //
    vkGetDeviceQueue(_device, _family, 0, &queue);
}

void VulkanContext::createSwapchain(uint32_t width, uint32_t height)
{
    // --- Obtain surface formats --- //
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &formatCount, nullptr);
    vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &formatCount, formats.data());

    // --- Pick format (prefer B8G8R8A8_SRGB / SRGB_NONLINEAR) --- //
    VkSurfaceFormatKHR format = formats[0]; // default
    for (auto& f : formats)
    {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            format = f;
            break;
        }
    }
    _swapchainFormat = format.format;
    _swapchainExtent = {width, height};

    // --- Obtain surface capabilities --- //
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &caps);
    uint32_t imageCount = std::max(caps.minImageCount + 1, caps.maxImageCount);
    if (imageCount == 0)
        FATAL("Vulkan surface does not support any images");

    // --- Create swapchain --- //
    VkSwapchainCreateInfoKHR info{};
    info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface               = _surface;
    info.minImageCount         = imageCount;
    info.imageFormat           = format.format;
    info.imageColorSpace       = format.colorSpace;
    info.imageExtent           = _swapchainExtent;
    info.imageArrayLayers      = 1;
    info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE; // single queue family
    info.queueFamilyIndexCount = 1;
    info.pQueueFamilyIndices   = &_family;
    info.preTransform          = caps.currentTransform;
    info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode           = VK_PRESENT_MODE_FIFO_KHR; // vsync, always supported
    info.clipped               = VK_TRUE;

    VkResult res = vkCreateSwapchainKHR(_device, &info, nullptr, &_swapchain);

    if (res != VK_SUCCESS)
        FATAL("vkCreateSwapchainKHR failed");

    // --- Obtain images --- //
    uint32_t swapchainImageCount = 0;
    vkGetSwapchainImagesKHR(_device, _swapchain, &swapchainImageCount, nullptr);
    if (swapchainImageCount != imageCount)
        FATAL("Swapchain image count mismatch {} != {}", swapchainImageCount, imageCount);
    _swapImages.resize(imageCount);
    vkGetSwapchainImagesKHR(_device, _swapchain, &imageCount, _swapImages.data());

    // --- Create image views --- //
    _swapImageViews.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo ivInfo{};
        ivInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivInfo.image                           = _swapImages[i];
        ivInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        ivInfo.format                          = _swapchainFormat;
        ivInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ivInfo.subresourceRange.baseMipLevel   = 0;
        ivInfo.subresourceRange.levelCount     = 1;
        ivInfo.subresourceRange.baseArrayLayer = 0;
        ivInfo.subresourceRange.layerCount     = 1;

        VkResult res = vkCreateImageView(_device, &ivInfo, nullptr, &_swapImageViews[i]);

        if (res != VK_SUCCESS)
            FATAL("vkCreateImageView failed");
    }
}

void VulkanContext::createRenderPass()
{
    // --- Create render pass --- //
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = _swapchainFormat;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorRef;

    // Dependency ensures the swapchain image is ready before we write to it
    VkSubpassDependency dep{};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info{};
    info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments    = &colorAttachment;
    info.subpassCount    = 1;
    info.pSubpasses      = &subpass;
    info.dependencyCount = 1;
    info.pDependencies   = &dep;

    VkResult res = vkCreateRenderPass(_device, &info, nullptr, &_renderPass);

    if (res != VK_SUCCESS)
        FATAL("vkCreateRenderPass failed");
}

VkShaderModule VulkanContext::createShaderModule(const char* path)
{
    auto code = readFile(path);

    VkShaderModuleCreateInfo info{};
    info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    info.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule mod;
    VkResult       res = vkCreateShaderModule(_device, &info, nullptr, &mod);

    if (res != VK_SUCCESS)
        FATAL("vkCreateShaderModule failed: {}", path);
    return mod;
}

void VulkanContext::createPipeline()
{
    VkShaderModule vert = createShaderModule("/home/arno/Documents/code/fracture/shader/fullscreen.vert.spv");
    VkShaderModule frag = createShaderModule("/home/arno/Documents/code/fracture/shader/fractal.frag.spv");

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName  = "main";

    // No vertex input — positions are generated in the vertex shader
    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{0, 0, (float)_swapchainExtent.width, (float)_swapchainExtent.height, 0, 1};
    VkRect2D   scissor{{0, 0}, _swapchainExtent};

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode    = VK_CULL_MODE_NONE;
    rasterizer.frontFace   = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments    = &blendAttachment;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    vkCreatePipelineLayout(_device, &layoutInfo, nullptr, &_pipelineLayout);

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = 2;
    pipelineInfo.pStages             = stages;
    pipelineInfo.pVertexInputState   = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pColorBlendState    = &colorBlend;
    pipelineInfo.layout              = _pipelineLayout;
    pipelineInfo.renderPass          = _renderPass;
    pipelineInfo.subpass             = 0;

    VkResult res = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline);

    if (res != VK_SUCCESS)
        FATAL("vkCreateGraphicsPipelines failed");

    vkDestroyShaderModule(_device, vert, nullptr);
    vkDestroyShaderModule(_device, frag, nullptr);
}

void VulkanContext::createFramebuffers()
{
    _framebuffers.resize(_swapImageViews.size());
    for (size_t i = 0; i < _swapImageViews.size(); i++)
    {
        VkFramebufferCreateInfo info{};
        info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass      = _renderPass;
        info.attachmentCount = 1;
        info.pAttachments    = &_swapImageViews[i];
        info.width           = _swapchainExtent.width;
        info.height          = _swapchainExtent.height;
        info.layers          = 1;

        vkCreateFramebuffer(_device, &info, nullptr, &_framebuffers[i]);
    }
}

void VulkanContext::createCommandPool()
{
    // --- Create command buffer pool --- //
    VkCommandPoolCreateInfo info{};
    info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.queueFamilyIndex = _family;
    info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vkCreateCommandPool(_device, &info, nullptr, &_commandPool);
}

void VulkanContext::createCommandBuffers()
{
    _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo info{};
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool        = _commandPool;
    info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    vkAllocateCommandBuffers(_device, &info, _commandBuffers.data());
}

void VulkanContext::createSyncObjects()
{
    _imageAvailable.resize(MAX_FRAMES_IN_FLIGHT);
    _renderFinished.resize(MAX_FRAMES_IN_FLIGHT);
    _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo     fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start signaled so first frame doesn't wait forever

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkCreateSemaphore(_device, &semInfo, nullptr, &_imageAvailable[i]);
        vkCreateSemaphore(_device, &semInfo, nullptr, &_renderFinished[i]);
        vkCreateFence(_device, &fenceInfo, nullptr, &_inFlightFences[i]);
    }
}

void VulkanContext::drawFrame()
{
    // Wait for the previous use of this frame slot to finish
    vkWaitForFences(_device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(_device, 1, &_inFlightFences[_currentFrame]);

    // Acquire the next swapchain image
    uint32_t imageIndex;
    vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _imageAvailable[_currentFrame], VK_NULL_HANDLE, &imageIndex);

    // Record commands
    VkCommandBuffer cmd = _commandBuffers[_currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkClearValue          clearColor{{{0.0f, 0.0f, 0.0f, 1.0f}}};
    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass        = _renderPass;
    rpInfo.framebuffer       = _framebuffers[imageIndex];
    rpInfo.renderArea.extent = _swapchainExtent;
    rpInfo.clearValueCount   = 1;
    rpInfo.pClearValues      = &clearColor;

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);
    vkCmdDraw(cmd, 3, 1, 0, 0); // 3 verts — fullscreen triangle, no buffer needed
    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    // Submit
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo         submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &_imageAvailable[_currentFrame];
    submitInfo.pWaitDstStageMask    = &waitStage;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &_renderFinished[_currentFrame];

    vkQueueSubmit(queue, 1, &submitInfo, _inFlightFences[_currentFrame]);

    // Present
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &_renderFinished[_currentFrame];
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &_swapchain;
    presentInfo.pImageIndices      = &imageIndex;

    vkQueuePresentKHR(queue, &presentInfo);

    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

VulkanContext::~VulkanContext()
{
    // ====== Device ====== //
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(_device, _imageAvailable[i], nullptr);
        vkDestroySemaphore(_device, _renderFinished[i], nullptr);
        vkDestroyFence(_device, _inFlightFences[i], nullptr);
    }
    // --- Command buffer memory pool --- //
    vkDestroyCommandPool(_device, _commandPool, nullptr);

    for (auto fb : _framebuffers)
        vkDestroyFramebuffer(_device, fb, nullptr);
    vkDestroyPipeline(_device, _pipeline, nullptr);
    vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
    vkDestroyRenderPass(_device, _renderPass, nullptr);

    // --- Swapchain --- //
    for (auto iv : _swapImageViews)
        vkDestroyImageView(_device, iv, nullptr);
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    vkDestroyDevice(_device, nullptr);

    // ====== Instance ====== //

    // --- Debug --- //
    auto vkDestroyDebugUtilsMessengerEXT =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (!vkDestroyDebugUtilsMessengerEXT)
        FATAL("vkDestroyDebugUtilsMessengerEXT not found");
    vkDestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyInstance(_instance, nullptr);
}