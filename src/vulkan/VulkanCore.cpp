#include "VulkanCore.hpp"
#include "util/Log.hpp"
#include "util/Vulkan.hpp"
#include "vulkan/CommandQueue.hpp"

#include <algorithm>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>

#include <fstream>
#include <tuple>

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

} // namespace

////////////////////////////////////////////////////////////

VulkanCore::VulkanCore(Window* window)
    : _window(window)
{
    // Core
    createInstance();
    createDebugCallback();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();

    // Swapchain
    createSurfaceFormat();
    createSwapchain();

    // Sync objects
    createSyncObjects();
}

////////////////////////////////////////////////////////////

void VulkanCore::createInstance()
{
    vk::ApplicationInfo appInfo{
        .pApplicationName   = "Fracture",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion         = VK_API_VERSION_1_4,
    };

    /* ===================== EXTENSIONS ===================== */

    // get required extensions for GLFW window
    vector<const char*> extensions = _window->getRequiredExtensions();
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

void VulkanCore::createDebugCallback()
{
    // create debug callback for any severity and type
    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerInfo{
        .messageSeverity =
            // vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType     = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = &debugCallback,
    };

    _debugMessenger = _instance.createDebugUtilsMessengerEXT(debugMessengerInfo);
}

////////////////////////////////////////////////////////////

void VulkanCore::createSurface()
{
    _surface = _window->createSurface(_instance);
}

////////////////////////////////////////////////////////////

void VulkanCore::pickPhysicalDevice()
{
    // --- Get All Physical Devices --- //
    vector<vk::raii::PhysicalDevice> devices = _instance.enumeratePhysicalDevices();

    if (devices.empty())
        FATAL("No Vulkan GPU found");

    // --- Pick First Discrete GPU --- //
    for (auto& device : devices)
    {
        if (device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        {
            _physicalDevice = device;
            return;
        }
    }

    _physicalDevice = devices[0]; // fallback to first device
}

////////////////////////////////////////////////////////////

void VulkanCore::createLogicalDevice()
{
    // obtain family properties
    vector<vk::QueueFamilyProperties> queueFamilies = _physicalDevice.getQueueFamilyProperties();

    /* ================== FIND QUEUE FAMILY ================= */
    _familyIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilies.size(); i++)
    {
        auto& family = queueFamilies[i];
        auto& flags  = family.queueFlags;

        bool presentSupport = _physicalDevice.getSurfaceSupportKHR(i, _surface);
        if ((flags & vk::QueueFlagBits::eGraphics) && (flags & vk::QueueFlagBits::eCompute) && presentSupport)
        {
            _familyIndex = i;
            break;
        }
    }
    if (_familyIndex == UINT32_MAX)
        FATAL("No graphics+present+compute queue family found");

    // --- Create graphics queue --- //
    float                     priority = 1.0f;
    vk::DeviceQueueCreateInfo queueInfo{
        .queueFamilyIndex = _familyIndex, .queueCount = 1, .pQueuePriorities = &priority
    };

    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    /* ================ CREATE LOGICAL DEVICE =============== */
    vk::DeviceCreateInfo deviceInfo{
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &queueInfo,
        .enabledExtensionCount   = 1,
        .ppEnabledExtensionNames = deviceExtensions
    };
    vk::PhysicalDeviceFeatures2        deviceFeatures2{.features = {.samplerAnisotropy = true}};
    vk::PhysicalDeviceVulkan13Features deviceFeatures13{.synchronization2 = true, .dynamicRendering = true};
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT deviceFeaturesDynamicState{.extendedDynamicState = true};

    vk::StructureChain<
        vk::DeviceCreateInfo,
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        chain{
            deviceInfo,                //
            deviceFeatures2,           // anisotropic filtering
            deviceFeatures13,          // new API features + dynamic rendering
            deviceFeaturesDynamicState // dynamic rendering
        };

    _device = _physicalDevice.createDevice(chain.get<vk::DeviceCreateInfo>());

    // --- Obtain queue --- //
    _queue = _device.getQueue(_familyIndex, 0);
}

////////////////////////////////////////////////////////////

void VulkanCore::createSurfaceFormat()
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
    _surfaceCaps = _physicalDevice.getSurfaceCapabilitiesKHR(_surface);

    // image count
    _swapImageCount = std::max(_surfaceCaps.minImageCount + 1, _surfaceCaps.maxImageCount);
    if (_swapImageCount == 0)
        FATAL("Vulkan surface does not support any images");
}

////////////////////////////////////////////////////////////

void VulkanCore::createSwapchain()
{
    // --- Set swapchain extent --- //
    std::tie(_swapChainExtent.width, _swapChainExtent.height) = _window->getSize();

    /* ================== CREATE SWAPCHAIN ================== */
    vk::SwapchainCreateInfoKHR swapChainInfo{
        .surface               = *_surface,
        .minImageCount         = _swapImageCount,
        .imageFormat           = _surfaceFormat.format,
        .imageColorSpace       = _surfaceFormat.colorSpace,
        .imageExtent           = _swapChainExtent,
        .imageArrayLayers      = 1,
        .imageUsage            = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode      = vk::SharingMode::eExclusive, // single queue family
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices   = &_familyIndex,
        .preTransform          = _surfaceCaps.currentTransform,
        .compositeAlpha        = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode           = vk::PresentModeKHR::eFifo, // vsync, always supported
        .clipped               = vk::True
    };
    _swapchain = _device.createSwapchainKHR(swapChainInfo);

    // --- Obtain images --- //
    _swapImages = _swapchain.getImages();

    /* ================= CREATE IMAGE VIEWS ================= */
    _swapImageViews.reserve(_swapImages.size());
    for (const auto& image : _swapImages)
    {
        vk::ImageViewCreateInfo imageViewInfo{
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

void VulkanCore::createSyncObjects()
{
    if (!_renderFinishedSemaphores.empty())
        Log::error("Sync objects already exist!");

    // Each image requires its own semaphore to indicate when it is ready for presentation
    for (uint32_t i = 0; i < _swapImages.size(); i++)
    {
        _renderFinishedSemaphores.emplace_back(_device, vk::SemaphoreCreateInfo());
    }
}

////////////////////////////////////////////////////////////

void VulkanCore::logInfo()
{
    // get highest available version
    uint32_t availVersion;
    vkEnumerateInstanceVersion(&availVersion);
    Log::info(
        "Available Vulkan instance version: {}.{}.{}",
        VK_API_VERSION_MAJOR(availVersion),
        VK_API_VERSION_MINOR(availVersion),
        VK_API_VERSION_PATCH(availVersion)
    );

    // --- Physical Devices ---
    auto physicalDevices = _instance.enumeratePhysicalDevices();

    Log::info("Vulkan compatible devices:");
    for (const auto& physicalDevice : physicalDevices)
    {
        // get device properties
        const auto& deviceProperties = physicalDevice.getProperties();
        string      name             = deviceProperties.deviceName;
        Log::info_t(1, "{}{}", Log::Color::Bold, name);

        // --- Limits ---
        const auto& limits = deviceProperties.limits;
        Log::info_t(2, "LIMITS:");
        Log::info_t(3, "maxSamplerAnisotropy: {}", limits.maxSamplerAnisotropy);
        Log::info_t(3, "maxPushConstantsSize: {}", limits.maxPushConstantsSize);
        Log::info_t(3, "maxComputeSharedMemorySize: {} KiB", limits.maxComputeSharedMemorySize / 1024);
        Log::info_t(3, "maxComputeWorkGroupSize: {}", limits.maxComputeWorkGroupSize);
        Log::info_t(3, "maxComputeWorkGroupInvocations: {}", limits.maxComputeWorkGroupInvocations);
        Log::info_t(3, "maxComputeWorkGroupCount: {}", limits.maxComputeWorkGroupCount);

        // --- Queue Families ---
        auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        Log::info_t(2, "QUEUE FAMILIES:");
        for (uint32_t f = 0; f < queueFamilies.size(); f++)
        {
            const auto& queueFamily = queueFamilies[f];

            // get queue count
            uint32_t queueCount = queueFamily.queueCount;

            // get queue flags
            auto flags = queueFamily.queueFlags;
            Log::info_t(3, "family {}: {} queue(s) {}", f, queueCount, vk::to_string(flags));
        }

        // --- Memory Properties ---
        auto memProps = physicalDevice.getMemoryProperties();

        Log::info_t(2, "MEMORY PROPERTIES:");
        for (uint32_t i = 0; i < memProps.memoryHeapCount; i++)
        {
            auto& heap = memProps.memoryHeaps[i];
            Log::info_t(3, "heap {}: {} MB {}", i, heap.size / 1024 / 1024, vk::to_string(heap.flags));

            for (uint32_t j = 0; j < memProps.memoryTypeCount; j++)
            {
                auto& type = memProps.memoryTypes[j];
                if (type.heapIndex != i)
                    continue;

                Log::info_t(4, "type {}: {}", j, vk::to_string(type.propertyFlags));
            }
        }

        Log::info_t(0, "");
    }
}

////////////////////////////////////////////////////////////

std::optional<uint32_t> VulkanCore::acquireNextImage(CommandQueue* commandQueue)
{
    auto [acquireResult, imageIndex] =
        _swapchain.acquireNextImage(UINT64_MAX, *commandQueue->getPresentCompleteSemaphore(), nullptr);

    // if out-of-date, recreate swapchain
    if (acquireResult == vk::Result::eErrorOutOfDateKHR)
        return std::nullopt;
    else if (acquireResult == vk::Result::eSuboptimalKHR)
        Log::warn("vk::SwapchainKHR::acquireNextImage returned vk::Result::eSuboptimalKHR");
    else if (acquireResult != vk::Result::eSuccess)
        Log::error("vk::SwapchainKHR::acquireNextImage returned {}", vk::to_string(acquireResult));

    return imageIndex;
}

////////////////////////////////////////////////////////////

void VulkanCore::prepare(CommandQueue* commandQueue, uint32_t imageIndex)
{
    auto& commandBuffer = commandQueue->getCommandBuffer();

    // --- Transition to ColorAttachmentOptimal --- //
    util::transitionImageLayout(
        commandQueue->getCommandBuffer(),
        {
            .srcStageMask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .srcAccessMask = {},
            .dstStageMask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
            .oldLayout     = vk::ImageLayout::eUndefined,
            .newLayout     = vk::ImageLayout::eColorAttachmentOptimal,
            .image         = _swapImages[imageIndex],
        }
    );

    // --- Viewport and Scissor --- //
    commandBuffer.setViewport(
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
    commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), _swapChainExtent));

    // --- Clear Color Attachment --- //
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

    // --- Start Rendering --- //
    commandBuffer.beginRendering(renderingInfo);
}

////////////////////////////////////////////////////////////

bool VulkanCore::present(uint32_t imageIndex)
{
    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*_renderFinishedSemaphores[imageIndex], // wait for submitted command buffer
        .swapchainCount     = 1,
        .pSwapchains        = &*_swapchain,
        .pImageIndices      = &imageIndex
    };
    auto presentResult = _queue.presentKHR(presentInfoKHR);

    // recreate swapchain if out-of-date or if window was resized
    if (presentResult == vk::Result::eErrorOutOfDateKHR || _window->wasResized())
    {
        _window->setResized(false);
        return false;
    }
    else if (presentResult == vk::Result::eSuboptimalKHR)
        Log::warn("vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR");
    else if (presentResult != vk::Result::eSuccess)
        Log::error("vk::Queue::presentKHR returned {}", vk::to_string(presentResult));

    return true;
}

////////////////////////////////////////////////////////////

void VulkanCore::waitIdle()
{
    _queue.waitIdle();
}

////////////////////////////////////////////////////////////

void VulkanCore::recreateSwapchain()
{
    // explicitly clear to avoid vk::NativeWindowInUseKHRError
    _swapchain.clear();
    _swapImageViews.clear();

    createSwapchain();
}

////////////////////////////////////////////////////////////

uint32_t VulkanCore::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties = _physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    FATAL("failed to find suitable memory type {}:{}", vk::to_string(properties), typeFilter);
}

////////////////////////////////////////////////////////////

vk::raii::ShaderModule VulkanCore::readShader(string shaderName) const
{
    string shaderPath = SHADER_DIR + shaderName + ".spv";

    // seek end of file
    std::ifstream file(shaderPath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        FATAL("failed to open shader at {}!", shaderPath);

    vector<char> shaderCode(file.tellg());

    file.seekg(0, std::ios::beg);
    file.read(shaderCode.data(), static_cast<std::streamsize>(shaderCode.size()));

    file.close();

    vk::ShaderModuleCreateInfo shaderModuleInfo{
        .codeSize = shaderCode.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(shaderCode.data())
    };

    // guaranteed copy elision
    return vk::raii::ShaderModule(_device, shaderModuleInfo);
}

////////////////////////////////////////////////////////////