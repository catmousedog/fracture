#include "VulkanContext.hpp"

#include <string>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_to_string.hpp>

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <vector>

#include "Basics.hpp"
#include "Log.hpp"
#include "vulkan/vulkan.hpp"

////////////////////////////////////////////////////////////

namespace
{

////////////////////////////////////////////////////////////

VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT             messageTypes,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*                                         pUserData
)
{
    const char* msg        = pCallbackData->pMessage;
    string      objectType = vk::to_string(messageTypes);

    switch (messageSeverity)
    {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        Log::info("{} Vulkan VRB: {}", objectType, msg);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        Log::info("{} Vulkan INF: {}", objectType, msg);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        Log::warn("{} Vulkan WRN: {}", objectType, msg);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        Log::error("{} Vulkan ERR: {}", objectType, msg);
        break;
    default:
        Log::error("{} Vulkan DFL: {}", objectType, msg);
        break;
    }

    return vk::False;
}
////////////////////////////////////////////////////////////

vector<char> readFile(const string& filename)
{
    // seek end of file
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        FATAL("failed to open file {}!", filename);

    vector<char> buffer(file.tellg());

    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    file.close();

    return buffer;
}

////////////////////////////////////////////////////////////

} // namespace

////////////////////////////////////////////////////////////

VulkanContext::VulkanContext(GLFWwindow* window, uint32_t width, uint32_t height)
    : _swapChainExtent({width, height})
{
    createInstance();
    createDebugCallback();
    createSurface(window);
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createPipeline();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
}

////////////////////////////////////////////////////////////

void VulkanContext::drawFrame()
{
    auto fenceResult = _device.waitForFences(*_drawFence, vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess)
        FATAL("failed to wait for fence!");
    _device.resetFences(*_drawFence);

    auto [result, imageIndex] = _swapchain.acquireNextImage(UINT64_MAX, *_presentCompleteSemaphore, nullptr);

    recordCommandBuffer(imageIndex);

    _queue.waitIdle(); // NOTE: for simplicity, wait for the queue to be idle before starting the frame
                       // In the next chapter you see how to use multiple frames in flight and fences to sync

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo   submitInfo{
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &*_presentCompleteSemaphore,
        .pWaitDstStageMask    = &waitDestinationStageMask,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &*_commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &*_renderFinishedSemaphore
    };
    _queue.submit(submitInfo, *_drawFence);

    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*_renderFinishedSemaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &*_swapchain,
        .pImageIndices      = &imageIndex
    };
    result = _queue.presentKHR(presentInfoKHR);
    switch (result)
    {
    case vk::Result::eSuccess:
        break;
    case vk::Result::eSuboptimalKHR:
        Log::warn("vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !");
        break;
    default:
        Log::warn("vk::Queue::presentKHR returned unexpected result!");
        break;
    }
}

////////////////////////////////////////////////////////////

void VulkanContext::waitIdle()
{
    _queue.waitIdle();
}

////////////////////////////////////////////////////////////

void VulkanContext::createInstance()
{
    vk::ApplicationInfo appInfo{
        .pApplicationName   = "Fracture",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion         = VK_API_VERSION_1_4,
    };

    // ===================== EXTENSIONS ===================== //

    // get required extensions for GLFW window
    uint32_t            glfwExtensionsCount = 0;
    const char**        glfwExtensions      = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
    vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionsCount);
    extensions.push_back("VK_EXT_debug_utils"); // add debug utils extension
    uint32_t extensionsCount = static_cast<uint32_t>(extensions.size());

    // Check if the required extensions are supported by the Vulkan implementation.
    auto extensionProperties   = _context.enumerateInstanceExtensionProperties();
    auto unsupportedPropertyIt = std::ranges::find_if(
        extensions,
        [&extensionProperties](auto const& requiredExtension)
        {
            return std::ranges::none_of(
                extensionProperties,
                [requiredExtension](auto const& extensionProperty)
                { return !strcmp(extensionProperty.extensionName, requiredExtension); }
            );
        }
    );
    if (unsupportedPropertyIt != extensions.end())
        FATAL("Required extension not supported: {}", *unsupportedPropertyIt);

    // ======================= LAYERS ======================= //

    // get required layers (default add validation layers for now)
    vector<const char*> layers     = {"VK_LAYER_KHRONOS_validation"};
    uint32_t            layerCount = static_cast<uint32_t>(layers.size());

    // Check if the required layers are supported by the Vulkan implementation.
    auto layerProperties    = _context.enumerateInstanceLayerProperties();
    auto unsupportedLayerIt = std::ranges::find_if(
        layers,
        [&layerProperties](auto const& requiredLayer)
        {
            return std::ranges::none_of(
                layerProperties,
                [requiredLayer](auto const& layerProperty) { return !strcmp(layerProperty.layerName, requiredLayer); }
            );
        }
    );
    if (unsupportedLayerIt != layers.end())
        FATAL("Required layer not supported: {}", *unsupportedLayerIt);

    // create instance
    vk::InstanceCreateInfo instanceInfo{
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = layerCount,
        .ppEnabledLayerNames     = layers.data(),
        .enabledExtensionCount   = extensionsCount,
        .ppEnabledExtensionNames = extensions.data(),
    };

    _instance = vk::raii::Instance(_context, instanceInfo);
}

////////////////////////////////////////////////////////////

void VulkanContext::createDebugCallback()
{
    // create debug callback for any severity and type
    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerInfo{
        .messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType     = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = &debugCallback,
    };

    _debugMessenger = _instance.createDebugUtilsMessengerEXT(debugMessengerInfo);
}

////////////////////////////////////////////////////////////

void VulkanContext::createSurface(GLFWwindow* window)
{
    // GLFW cannot use raii
    VkSurfaceKHR surface;
    VkResult     res = glfwCreateWindowSurface(*_instance, window, nullptr, &surface);
    if (res != VK_SUCCESS)
        FATAL("glfwCreateWindowSurface failed");

    // convert to raii
    _surface = vk::raii::SurfaceKHR(_instance, surface);
}

////////////////////////////////////////////////////////////

void VulkanContext::pickPhysicalDevice()
{
    // --- Get all physical devices --- //
    vector<vk::raii::PhysicalDevice> devices = _instance.enumeratePhysicalDevices();

    if (devices.empty())
        FATAL("No Vulkan GPU found");

    // --- Pick first device --- //
    _physicalDevice = devices[0];
}

////////////////////////////////////////////////////////////

void VulkanContext::createLogicalDevice()
{
    // --- Obtain queue family properties --- //
    vector<vk::QueueFamilyProperties> queueFamilies = _physicalDevice.getQueueFamilyProperties();

    // --- Find queue family with graphics and present support --- //
    _familyIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilies.size(); i++)
    {
        vk::Bool32 presentSupport = _physicalDevice.getSurfaceSupportKHR(i, _surface);
        if ((queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) && presentSupport)
        {
            _familyIndex = i;
            break;
        }
    }
    if (_familyIndex == UINT32_MAX)
        FATAL("No graphics+present queue family found");

    // --- Create single queue --- //
    float                     priority = 1.0f;
    vk::DeviceQueueCreateInfo queueInfo{
        .queueFamilyIndex = _familyIndex, .queueCount = 1, .pQueuePriorities = &priority
    };

    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // --- Create logical device --- //
    vk::DeviceCreateInfo deviceInfo{
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &queueInfo,
        .enabledExtensionCount   = 1,
        .ppEnabledExtensionNames = deviceExtensions
    };
    vk::PhysicalDeviceVulkan11Features deviceFeatures11{.shaderDrawParameters = true};
    vk::PhysicalDeviceVulkan13Features deviceFeatures13{.synchronization2 = true, .dynamicRendering = true};
    vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceVulkan13Features>
        chain{deviceInfo, deviceFeatures11, deviceFeatures13};

    _device = _physicalDevice.createDevice(chain.get<vk::DeviceCreateInfo>());

    // --- Obtain queue --- //
    _queue = _device.getQueue(_familyIndex, 0);
}

////////////////////////////////////////////////////////////

void VulkanContext::createSwapchain()
{
    // --- Obtain surface formats --- //
    vector<vk::SurfaceFormatKHR> formats = _physicalDevice.getSurfaceFormatsKHR(_surface);

    if (formats.empty())
        FATAL("No surface formats available");

    // --- Pick format (prefer B8G8R8A8_SRGB / SRGB_NONLINEAR) --- //
    constexpr vk::SurfaceFormatKHR desiredFormat{vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear};
    const auto                     itFormat = std::ranges::find(formats, desiredFormat);
    _surfaceFormat                          = itFormat != formats.end() ? *itFormat : formats[0];

    // --- Obtain surface capabilities --- //
    vk::SurfaceCapabilitiesKHR caps = _physicalDevice.getSurfaceCapabilitiesKHR(_surface);
    _imageCount                     = std::max(caps.minImageCount + 1, caps.maxImageCount);
    if (_imageCount == 0)
        FATAL("Vulkan surface does not support any images");

    // --- Create swapchain --- //
    vk::SwapchainCreateInfoKHR swapChainInfo{
        .surface               = *_surface,
        .minImageCount         = _imageCount,
        .imageFormat           = _surfaceFormat.format,
        .imageColorSpace       = _surfaceFormat.colorSpace,
        .imageExtent           = _swapChainExtent,
        .imageArrayLayers      = 1,
        .imageUsage            = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode      = vk::SharingMode::eExclusive, // single queue family
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices   = &_familyIndex,
        .preTransform          = caps.currentTransform,
        .compositeAlpha        = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode           = vk::PresentModeKHR::eFifo, // vsync, always supported
        .clipped               = vk::True
    };

    _swapchain = _device.createSwapchainKHR(swapChainInfo);

    // --- Obtain images --- //
    _swapImages = _swapchain.getImages();

    // --- Create image views --- //
    _swapImageViews.reserve(_swapImages.size());
    for (const auto& image : _swapImages)
    {
        vk::ImageViewCreateInfo imageViewInfo{
            .sType            = vk::StructureType::eImageViewCreateInfo,
            .image            = image,
            .viewType         = vk::ImageViewType::e2D,
            .format           = _surfaceFormat.format,
            .subresourceRange = {
                .aspectMask     = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            }
        };

        _swapImageViews.emplace_back(_device, imageViewInfo);
    }
}

////////////////////////////////////////////////////////////

void VulkanContext::createPipeline()
{
    const vector<char> code = readFile("../shaders/slang.spv");

    vk::ShaderModuleCreateInfo shaderModuleInfo{
        .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };
    _shader = _device.createShaderModule(shaderModuleInfo);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex, .module = _shader, .pName = "vertMain"
    };
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment, .module = _shader, .pName = "fragMain"
    };
    vk::PipelineShaderStageCreateInfo shaderStagesInfo[] = {vertShaderStageInfo, fragShaderStageInfo};

    vk::PipelineVertexInputStateCreateInfo   vertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{.topology = vk::PrimitiveTopology::eTriangleList};
    vk::PipelineViewportStateCreateInfo      viewportInfo{.viewportCount = 1, .scissorCount = 1};

    vk::PipelineRasterizationStateCreateInfo rasterizationInfo{
        .depthClampEnable        = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode             = vk::PolygonMode::eFill,
        .cullMode                = vk::CullModeFlagBits::eBack,
        .frontFace               = vk::FrontFace::eClockwise,
        .depthBiasEnable         = vk::False,
        .lineWidth               = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisampleInfo{
        .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachmentInfo{
        .blendEnable    = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendInfo{
        .logicOpEnable   = vk::False,
        .logicOp         = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments    = &colorBlendAttachmentInfo
    };

    std::vector<vk::DynamicState>      dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data()
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{.setLayoutCount = 0, .pushConstantRangeCount = 0};
    _pipelineLayout = vk::raii::PipelineLayout(_device, pipelineLayoutInfo);

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
        {.stageCount          = 2,
         .pStages             = shaderStagesInfo,
         .pVertexInputState   = &vertexInputInfo,
         .pInputAssemblyState = &inputAssemblyInfo,
         .pViewportState      = &viewportInfo,
         .pRasterizationState = &rasterizationInfo,
         .pMultisampleState   = &multisampleInfo,
         .pColorBlendState    = &colorBlendInfo,
         .pDynamicState       = &dynamicStateInfo,
         .layout              = _pipelineLayout,
         .renderPass          = nullptr},
        {.colorAttachmentCount = 1, .pColorAttachmentFormats = &_surfaceFormat.format}
    };

    _pipeline = vk::raii::Pipeline(_device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}

////////////////////////////////////////////////////////////

void VulkanContext::createCommandPool()
{
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = _familyIndex
    };
    _commandPool = _device.createCommandPool(poolInfo);
}

////////////////////////////////////////////////////////////

void VulkanContext::createCommandBuffer()
{
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = _commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1
    };
    _commandBuffer = std::move(_device.allocateCommandBuffers(allocInfo).front());
}

////////////////////////////////////////////////////////////

void VulkanContext::recordCommandBuffer(uint32_t imageIndex)
{
    _commandBuffer.begin({});

    // Before starting rendering, transition the swapchain image to vk::ImageLayout::eColorAttachmentOptimal
    transition_image_layout(
        imageIndex,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},                                                 // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite,         // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput  // dstStage
    );
    vk::ClearValue              clearColor     = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView   = _swapImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eStore,
        .clearValue  = clearColor
    };
    vk::RenderingInfo renderingInfo = {
        .renderArea           = {.offset = {0, 0}, .extent = _swapChainExtent},
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &attachmentInfo
    };

    _commandBuffer.beginRendering(renderingInfo);
    _commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipeline);
    _commandBuffer.setViewport(
        0,
        vk::Viewport(
            0.0f,
            0.0f,
            static_cast<float>(_swapChainExtent.width),
            static_cast<float>(_swapChainExtent.height),
            0.0f,
            1.0f
        )
    );
    _commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), _swapChainExtent));
    _commandBuffer.draw(3, 1, 0, 0);
    _commandBuffer.endRendering();

    // After rendering, transition the swapchain image to vk::ImageLayout::ePresentSrcKHR
    transition_image_layout(
        imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,         // srcAccessMask
        {},                                                 // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe           // dstStage
    );
    _commandBuffer.end();
}

////////////////////////////////////////////////////////////

void VulkanContext::createSyncObjects()
{
    _presentCompleteSemaphore = vk::raii::Semaphore(_device, vk::SemaphoreCreateInfo());
    _renderFinishedSemaphore  = vk::raii::Semaphore(_device, vk::SemaphoreCreateInfo());
    _drawFence                = vk::raii::Fence(_device, {.flags = vk::FenceCreateFlagBits::eSignaled});
}

////////////////////////////////////////////////////////////

void VulkanContext::transition_image_layout(
    uint32_t                imageIndex,
    vk::ImageLayout         old_layout,
    vk::ImageLayout         new_layout,
    vk::AccessFlags2        src_access_mask,
    vk::AccessFlags2        dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask
)
{
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask        = src_stage_mask,
        .srcAccessMask       = src_access_mask,
        .dstStageMask        = dst_stage_mask,
        .dstAccessMask       = dst_access_mask,
        .oldLayout           = old_layout,
        .newLayout           = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = _swapImages[imageIndex],
        .subresourceRange    = {
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };
    vk::DependencyInfo dependency_info = {
        .dependencyFlags = {}, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier
    };
    _commandBuffer.pipelineBarrier2(dependency_info);
}

////////////////////////////////////////////////////////////

PFN_vkVoidFunction VulkanContext::getFunctionEXT(const char* funcName)
{
    auto func = _instance.getProcAddr(funcName);
    if (!func)
        FATAL("FunctionEXT {} not found", funcName);
    return func;
}

////////////////////////////////////////////////////////////
