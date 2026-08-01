/* ====================== FRACTURE ====================== */
#include "VulkanContext.hpp"
#include "util/Log.hpp"

/* ======================= Vulkan ======================= */
#include <algorithm>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>

/* ======================== IMGUI ======================= */
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include "vulkan/vulkan.hpp"

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

VulkanContext::VulkanContext(Window* window)
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

    // Descriptor Layouts
    createGraphicsDescriptorLayout();
    createComputeDescriptorLayout();

    // Pipeline
    createGraphicsPipeline();
    createComputePipeline();

    // Command buffers
    createCommandPool();
    createCommandBuffer();

    // Compute Image
    createComputeImage();
    createSampler();

    // Buffers
    createVertexBuffer();
    createIndexBuffer();

    // Descriptors
    createDescriptorPool();
    createUiDescriptorPool();
    createGraphicsDescriptorSet();
    createComputeDescriptorSet();

    // Sync objects
    createSyncObjects();

    initImGUI();
}

////////////////////////////////////////////////////////////

void VulkanContext::logInfo()
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

void VulkanContext::drawFrame()
{
    // each frame has its own command buffer, fence and present semaphore
    auto& commandBuffer            = _commandBuffers[_frameIndex];            // command buffer per GPU 'worker'
    auto& drawFence                = _drawFences[_frameIndex];                // fence if GPU is still rendering
    auto& presentCompleteSemaphore = _presentCompleteSemaphores[_frameIndex]; // signaled when image is ready for use

    // wait for current command buffer to finish
    auto fenceResult = _device.waitForFences(*drawFence, vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess)
        Log::warn("vk::Device::waitForFences returned {} !", vk::to_string(fenceResult));

    // acquire next (possibly still presenting) image
    auto [acquireResult, imageIndex] = _swapchain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphore, nullptr);

    // if out-of-date, recreate swapchain
    if (acquireResult == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapchain();
        return;
    }
    else if (acquireResult == vk::Result::eSuboptimalKHR)
        Log::warn("vk::SwapchainKHR::acquireNextImage returned vk::Result::eSuboptimalKHR");
    else if (acquireResult != vk::Result::eSuccess)
        Log::error("vk::SwapchainKHR::acquireNextImage returned {}", vk::to_string(acquireResult));

    // each image has its own semaphore to indicate it is ready for presentation
    auto& renderFinishedSemaphore = _renderFinishedSemaphores[imageIndex];

    /* ======================= RENDER ======================= */
    commandBuffer.reset();
    recordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo   submitInfo{
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &*presentCompleteSemaphore, // wait for this frame to finish presenting
        .pWaitDstStageMask    = &waitDestinationStageMask,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &*commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &*renderFinishedSemaphore // signal this command buffer finished
    };

    // unsignal fence, before submitting to avoid deadlock (if unsignaled and eErrorOutOfDateKHR)
    _device.resetFences(*drawFence);
    _queue.submit(submitInfo, *drawFence);

    /* ======================= PRESENT ====================== */
    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*renderFinishedSemaphore, // wait for submitted command buffer
        .swapchainCount     = 1,
        .pSwapchains        = &*_swapchain,
        .pImageIndices      = &imageIndex
    };
    auto presentResult = _queue.presentKHR(presentInfoKHR);

    // recreate swapchain if out-of-date or if window was resized
    if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR ||
        _window->wasResized())
    {
        _dirty = true;
        _window->setResized(false);
        recreateSwapchain();
        return;
    }
    else if (presentResult != vk::Result::eSuccess)
        Log::error("vk::Queue::presentKHR returned {}", vk::to_string(presentResult));

    // CPU use next frame
    _frameIndex = (_frameIndex + 1) % _framesInFlight;
}

////////////////////////////////////////////////////////////

void VulkanContext::recreateSwapchain()
{
    _queue.waitIdle();

    // explicitly clear to avoid vk::NativeWindowInUseKHRError
    _swapchain.clear();
    _swapImageViews.clear();
    createSwapchain();

    // recreate compute image
    _computeImage.clear();
    _computeImageMemory.clear();
    _computeImageView.clear();
    createComputeImage();

    // update descriptor sets
    updateGraphicsDescriptorSet();
    updateComputeDescriptorSet();
}

////////////////////////////////////////////////////////////

void VulkanContext::createInstance()
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

void VulkanContext::createDebugCallback()
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

void VulkanContext::createSurface()
{
    _surface = _window->createSurface(_instance);
}

////////////////////////////////////////////////////////////

void VulkanContext::pickPhysicalDevice()
{
    // --- Get All Physical Devices --- //
    vector<vk::raii::PhysicalDevice> devices = _instance.enumeratePhysicalDevices();

    if (devices.empty())
        FATAL("No Vulkan GPU found");

    // --- Pick First Device --- //
    _physicalDevice = devices[0];
}

////////////////////////////////////////////////////////////

void VulkanContext::createLogicalDevice()
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

void VulkanContext::createSurfaceFormat()
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
    _imageCount = std::max(_surfaceCaps.minImageCount + 1, _surfaceCaps.maxImageCount);
    if (_imageCount == 0)
        FATAL("Vulkan surface does not support any images");

    // clamp frames-in-flight
    _framesInFlight = std::clamp(_framesInFlight, _framesInFlight, _imageCount);
}

////////////////////////////////////////////////////////////

void VulkanContext::createSwapchain()
{
    // --- Set swapchain extent --- //
    std::tie(_swapChainExtent.width, _swapChainExtent.height) = _window->getSize();

    /* ================== CREATE SWAPCHAIN ================== */
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

void VulkanContext::createGraphicsDescriptorLayout()
{
    // Graphics Descriptor: CombinedImageSampler
    vk::DescriptorSetLayoutBinding bindings{
        .binding         = 0,
        .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags      = vk::ShaderStageFlagBits::eFragment
    };
    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = 1,        //
        .pBindings    = &bindings //
    };
    _graphicsDescriptorSetLayout = vk::raii::DescriptorSetLayout(_device, layoutInfo);
}

////////////////////////////////////////////////////////////

void VulkanContext::createComputeDescriptorLayout()
{
    // Compute Descriptor: StorageImage
    vk::DescriptorSetLayoutBinding bindings{
        .binding         = 0,
        .descriptorType  = vk::DescriptorType::eStorageImage,
        .descriptorCount = 1,
        .stageFlags      = vk::ShaderStageFlagBits::eCompute
    };
    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = 1,        //
        .pBindings    = &bindings //
    };
    _computeDescriptorSetLayout = vk::raii::DescriptorSetLayout(_device, layoutInfo);
}

////////////////////////////////////////////////////////////

void VulkanContext::createComputeImage()
{
    constexpr vk::Format format = vk::Format::eR8G8B8A8Unorm;

    /* ==================== CREATE IMAGE ==================== */
    vk::ImageCreateInfo imageInfo{
        .imageType     = vk::ImageType::e2D,
        .format        = format,
        .extent        = {_swapChainExtent.width, _swapChainExtent.height, 1},
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = vk::SampleCountFlagBits::e1,
        .tiling        = vk::ImageTiling::eOptimal,
        .usage         = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled,
        .sharingMode   = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined
    };
    _computeImage = vk::raii::Image(_device, imageInfo);

    // --- Create/Bind Image Memory --- //
    vk::MemoryRequirements memRequirements = _computeImage.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize  = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
    };
    _computeImageMemory = vk::raii::DeviceMemory(_device, allocInfo);
    _computeImage.bindMemory(*_computeImageMemory, 0);

    /* ================== CREATE IMAGE VIEW ================= */
    vk::ImageViewCreateInfo viewInfo{
        .image            = _computeImage,
        .viewType         = vk::ImageViewType::e2D,
        .format           = format,
        .subresourceRange = {
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };
    _computeImageView = vk::raii::ImageView(_device, viewInfo);

    /* ================== TRANSITION IMAGE ================== */
    singleCommand(
        [this](vk::raii::CommandBuffer& commandBuffer)
        {
            transitionImageLayout(
                commandBuffer,
                _computeImage,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eShaderReadOnlyOptimal // will switch back to General in recordCommandBuffer
            );
        }
    );
}

////////////////////////////////////////////////////////////

void VulkanContext::createSampler()
{
    // Graphics Image Sampler
    vk::PhysicalDeviceProperties properties = _physicalDevice.getProperties();
    vk::SamplerCreateInfo        samplerInfo{
        .magFilter        = vk::Filter::eLinear,
        .minFilter        = vk::Filter::eLinear,
        .mipmapMode       = vk::SamplerMipmapMode::eLinear,
        .addressModeU     = vk::SamplerAddressMode::eRepeat,
        .addressModeV     = vk::SamplerAddressMode::eRepeat,
        .addressModeW     = vk::SamplerAddressMode::eRepeat,
        .mipLodBias       = 0.0f,
        .anisotropyEnable = vk::True,
        .maxAnisotropy    = properties.limits.maxSamplerAnisotropy,
        .compareEnable    = vk::False,
        .compareOp        = vk::CompareOp::eAlways
    };
    _sampler = _device.createSampler(samplerInfo);
}

////////////////////////////////////////////////////////////

void VulkanContext::createGraphicsPipeline()
{
    // --- Create Shader --- //
    vk::raii::ShaderModule            shader = readShader("graphics");
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex, .module = shader, .pName = "vertMain"
    };
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment, .module = shader, .pName = "fragMain"
    };
    vk::PipelineShaderStageCreateInfo shaderStagesInfo[] = {vertShaderStageInfo, fragShaderStageInfo};

    // --- Bind Vertex Buffer --- //
    auto                                   bindingDescription    = Vertex::getBindingDescription();
    auto                                   attributeDescriptions = Vertex::getAttributeDescriptions();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions    = attributeDescriptions.data()
    };

    // input assembly
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{.topology = vk::PrimitiveTopology::eTriangleList};

    // viewport and scissor counts
    vk::PipelineViewportStateCreateInfo viewportInfo{.viewportCount = 1, .scissorCount = 1};

    // rasterization
    vk::PipelineRasterizationStateCreateInfo rasterizationInfo{
        .depthClampEnable        = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode             = vk::PolygonMode::eFill,
        .cullMode                = vk::CullModeFlagBits::eBack,
        .frontFace               = vk::FrontFace::eClockwise,
        .depthBiasEnable         = vk::False,
        .lineWidth               = 1.0f
    };

    // multisampling
    vk::PipelineMultisampleStateCreateInfo multisampleInfo{
        .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False
    };

    // color blending
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

    // viewport and scissors
    vector<vk::DynamicState>           dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data()
    };

    /* =================== CREATE PIPELINE ================== */
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = 1,                             //
        .pSetLayouts    = &*_graphicsDescriptorSetLayout //
    };
    _graphicsPipelineLayout = _device.createPipelineLayout(pipelineLayoutInfo);

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
         .layout              = _graphicsPipelineLayout,
         .renderPass          = nullptr},
        {.colorAttachmentCount = 1, .pColorAttachmentFormats = &_surfaceFormat.format}
    };

    _graphicsPipeline =
        vk::raii::Pipeline(_device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}

////////////////////////////////////////////////////////////

void VulkanContext::createComputePipeline()
{
    // Create Shader
    vk::raii::ShaderModule            shader = readShader("compute");
    vk::PipelineShaderStageCreateInfo compShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eCompute, .module = shader, .pName = "compMain"
    };

    // --- Push Constants --- //
    vk::PushConstantRange pushConstantRange{
        .stageFlags = vk::ShaderStageFlagBits::eCompute, .offset = 0, .size = sizeof(FractalPushConstants)
    };

    /* =================== CREATE PIPELINE ================== */
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount         = 1,
        .pSetLayouts            = &*_computeDescriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges    = &pushConstantRange
    };
    _computePipelineLayout = _device.createPipelineLayout(pipelineLayoutInfo);

    vk::ComputePipelineCreateInfo pipelineInfo{
        .stage  = compShaderStageInfo,   //
        .layout = _computePipelineLayout //
    };

    _computePipeline = vk::raii::Pipeline(_device, nullptr, pipelineInfo);
}

////////////////////////////////////////////////////////////

void VulkanContext::createCommandPool()
{
    // --- Create Command Pool --- //
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = _familyIndex
    };
    _commandPool = vk::raii::CommandPool(_device, poolInfo);
}

////////////////////////////////////////////////////////////

void VulkanContext::createCommandBuffer()
{
    // --- Allocate Command Buffers --- //
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = _commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = _framesInFlight
    };
    _commandBuffers = _device.allocateCommandBuffers(allocInfo);
}

////////////////////////////////////////////////////////////

void VulkanContext::recordCommandBuffer(uint32_t imageIndex)
{
    auto& commandBuffer = _commandBuffers[_frameIndex];

    commandBuffer.begin({});

    // --- Recompute --- //
    if (_dirty)
    {
        // --- Transition to General --- //
        transition_image_layout(
            *_computeImage,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eGeneral,
            vk::AccessFlagBits2::eShaderRead,
            vk::AccessFlagBits2::eShaderWrite,
            vk::PipelineStageFlagBits2::eFragmentShader,
            vk::PipelineStageFlagBits2::eComputeShader
        );

        // --- Bind Descriptor Set --- //
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *_computePipeline);
        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute, _computePipelineLayout, 0, *_computeDescriptorSet, nullptr
        );

        // --- Push Constants --- //
        commandBuffer.pushConstants(
            _computePipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(FractalPushConstants), &_pushConstants
        );

        // --- Execute --- //
        uint32_t groupX = (_swapChainExtent.width + 15) / 16;
        uint32_t groupY = (_swapChainExtent.height + 15) / 16;
        commandBuffer.dispatch(groupX, groupY, 1);

        // --- Transition to ShaderReadOnlyOptimal --- //
        transition_image_layout(
            *_computeImage,
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::AccessFlagBits2::eShaderWrite,
            vk::AccessFlagBits2::eShaderRead,
            vk::PipelineStageFlagBits2::eComputeShader,
            vk::PipelineStageFlagBits2::eFragmentShader
        );
        _dirty = false;
    }

    // --- Transition to ColorAttachmentOptimal --- //
    transition_image_layout(
        _swapImages[imageIndex],
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},                                                 // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite,         // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput  // dstStage
    );

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

    // --- Render --- //
    commandBuffer.beginRendering(renderingInfo);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *_graphicsPipeline);

    // Set Viewport and Scissor
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

    // Bind
    commandBuffer.bindVertexBuffers(0, *_vertexBuffer, {0});
    commandBuffer.bindIndexBuffer(*_indexBuffer, 0, vk::IndexType::eUint16);
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, _graphicsPipelineLayout, 0, *_graphicsDescriptorSet, nullptr
    );

    // Draw
    commandBuffer.drawIndexed(static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);
    // UI
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *commandBuffer);

    // --- End Rendering --- //
    commandBuffer.endRendering();

    // --- Transition to PresentSrcKHR --- //
    transition_image_layout(
        _swapImages[imageIndex],
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,         // srcAccessMask
        {},                                                 // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe           // dstStage
    );
    commandBuffer.end();
}

////////////////////////////////////////////////////////////

void VulkanContext::createVertexBuffer()
{
    // --- Create Staging Buffer --- //
    vk::DeviceSize bufferSize                 = sizeof(_vertices[0]) * _vertices.size();
    auto [stagingBuffer, stagingBufferMemory] = createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    // --- Copy to Staging Buffer --- //
    void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(dataStaging, _vertices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();

    // --- Create Vertex Buffer --- //
    std::tie(_vertexBuffer, _vertexBufferMemory) = createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    // --- Transfer from Staging Buffer --- //
    singleCommand([&](vk::raii::CommandBuffer& commandBuffer)
                  { commandBuffer.copyBuffer(*stagingBuffer, *_vertexBuffer, vk::BufferCopy{.size = bufferSize}); });
}

////////////////////////////////////////////////////////////

void VulkanContext::createIndexBuffer()
{
    // --- Create Staging Buffer --- //
    vk::DeviceSize bufferSize                 = sizeof(_indices[0]) * _indices.size();
    auto [stagingBuffer, stagingBufferMemory] = createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    // --- Copy to Staging Buffer --- //
    void* data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, _indices.data(), (size_t)bufferSize);
    stagingBufferMemory.unmapMemory();

    // --- Create Index Buffer --- //
    std::tie(_indexBuffer, _indexBufferMemory) = createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    // --- Transfer from Staging Buffer --- //
    singleCommand([&](vk::raii::CommandBuffer& commandBuffer)
                  { commandBuffer.copyBuffer(*stagingBuffer, *_indexBuffer, vk::BufferCopy{.size = bufferSize}); });
}

////////////////////////////////////////////////////////////

void VulkanContext::createDescriptorPool()
{
    std::array<vk::DescriptorPoolSize, 2> poolSize{
        {{.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = _framesInFlight},
         {.type = vk::DescriptorType::eStorageImage, .descriptorCount = 1}}
    };
    vk::DescriptorPoolCreateInfo poolInfo{
        .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets       = _framesInFlight + 1,
        .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
        .pPoolSizes    = poolSize.data()
    };
    _descriptorPool = _device.createDescriptorPool(poolInfo);
}

////////////////////////////////////////////////////////////

void VulkanContext::createUiDescriptorPool()
{
    vk::DescriptorPoolSize poolSizes[] = {
        {vk::DescriptorType::eSampledImage, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE}, // 8
        {vk::DescriptorType::eSampler, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE}             // 2
    };
    vk::DescriptorPoolCreateInfo pool_info = {
        .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets       = 10, // 8 + 2
        .poolSizeCount = 2,
        .pPoolSizes    = poolSizes
    };
    _uiDescriptorPool = _device.createDescriptorPool(pool_info);
}

////////////////////////////////////////////////////////////

void VulkanContext::createGraphicsDescriptorSet()
{
    // --- Allocate Descriptor --- //
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool     = _descriptorPool,               //
        .descriptorSetCount = 1,                             //
        .pSetLayouts        = &*_graphicsDescriptorSetLayout //
    };
    _graphicsDescriptorSet = std::move(_device.allocateDescriptorSets(allocInfo).front());

    // --- Update Allocated Descriptor --- //
    updateGraphicsDescriptorSet();
}

////////////////////////////////////////////////////////////

void VulkanContext::updateGraphicsDescriptorSet()
{
    vk::DescriptorImageInfo imageInfo{
        .sampler     = _sampler,                               //
        .imageView   = _computeImageView,                      //
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal //
    };
    vk::WriteDescriptorSet descriptorWrites{
        .dstSet          = _graphicsDescriptorSet,
        .dstBinding      = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo      = &imageInfo
    };
    _device.updateDescriptorSets(descriptorWrites, {});
}

////////////////////////////////////////////////////////////

void VulkanContext::createComputeDescriptorSet()
{
    // --- Allocate Descriptor --- //
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool     = _descriptorPool,              //
        .descriptorSetCount = 1,                            //
        .pSetLayouts        = &*_computeDescriptorSetLayout //
    };
    _computeDescriptorSet = std::move(_device.allocateDescriptorSets(allocInfo).front());

    // --- Update Allocated Descriptor --- //
    updateComputeDescriptorSet();
}

////////////////////////////////////////////////////////////

void VulkanContext::updateComputeDescriptorSet()
{
    vk::DescriptorImageInfo imageInfo{
        .imageView   = _computeImageView,        //
        .imageLayout = vk::ImageLayout::eGeneral //
    };
    vk::WriteDescriptorSet descriptorWrites{
        .dstSet          = _computeDescriptorSet,
        .dstBinding      = 0,
        .descriptorCount = 1,
        .descriptorType  = vk::DescriptorType::eStorageImage,
        .pImageInfo      = &imageInfo
    };
    _device.updateDescriptorSets(descriptorWrites, {});
}

////////////////////////////////////////////////////////////

void VulkanContext::createSyncObjects()
{
    if (!_presentCompleteSemaphores.empty() || !_renderFinishedSemaphores.empty() || !_drawFences.empty())
        Log::error("Sync objects already exist!");

    // Each image requires its own semaphore to indicate when it is ready for presentation
    for (uint32_t i = 0; i < _swapImages.size(); i++)
    {
        _renderFinishedSemaphores.emplace_back(_device, vk::SemaphoreCreateInfo());
    }

    // Each frame requires a fence to halt the host, and a semaphore to indicate when ready for rendering
    for (uint32_t i = 0; i < _framesInFlight; i++)
    {
        _presentCompleteSemaphores.emplace_back(_device, vk::SemaphoreCreateInfo());
        _drawFences.emplace_back(_device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}

////////////////////////////////////////////////////////////

void VulkanContext::initImGUI()
{
    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo = {
        .colorAttachmentCount    = 1,                     //
        .pColorAttachmentFormats = &_surfaceFormat.format //
    };

    ImGui_ImplVulkan_InitInfo initInfo{
        .Instance            = *_instance,
        .PhysicalDevice      = *_physicalDevice,
        .Device              = *_device,
        .QueueFamily         = _familyIndex,
        .Queue               = *_queue,
        .DescriptorPool      = *_uiDescriptorPool,
        .MinImageCount       = _framesInFlight,
        .ImageCount          = _framesInFlight,
        .PipelineInfoMain    = {.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo},
        .UseDynamicRendering = true
    };

    ImGui_ImplVulkan_Init(&initInfo);
}

////////////////////////////////////////////////////////////

void VulkanContext::transition_image_layout(
    const vk::Image&        image,
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
        .image               = image,
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
    _commandBuffers[_frameIndex].pipelineBarrier2(dependency_info);
}

////////////////////////////////////////////////////////////

void VulkanContext::transitionImageLayout(
    vk::raii::CommandBuffer& commandBuffer,
    const vk::raii::Image&   image,
    vk::ImageLayout          oldLayout,
    vk::ImageLayout          newLayout
)
{
    vk::ImageMemoryBarrier barrier{
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image               = image,
        .subresourceRange    = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1}
    };

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    // Undefined -> TransferDstOptimal
    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage      = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }

    // Undefined -> General (compute)
    else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eGeneral)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;

        sourceStage      = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eComputeShader;
    }

    // Undefined -> ShaderReadOnlyOptimal (startup)
    else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage      = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eComputeShader;
    }

    // TransferDstOptimal -> ShaderReadOnlyOptimal
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage      = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }

    // Unsupported
    else
    {
        FATAL(
            "unsupported vk::ImageLayout::ImageLayout transition from {} to {}",
            vk::to_string(oldLayout),
            vk::to_string(newLayout)
        );
    }

    commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);
}

////////////////////////////////////////////////////////////

uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
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

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory>
VulkanContext::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    vk::BufferCreateInfo   bufferInfo{.size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive};
    vk::raii::Buffer       buffer          = vk::raii::Buffer(_device, bufferInfo);
    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize  = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
    };
    vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(_device, allocInfo);
    buffer.bindMemory(*bufferMemory, 0);
    return {std::move(buffer), std::move(bufferMemory)};
}

////////////////////////////////////////////////////////////

void VulkanContext::singleCommand(std::function<void(vk::raii::CommandBuffer&)> command)
{
    // --- Create Command Buffer --- //
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = _commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1
    };
    vk::raii::CommandBuffer commandBuffer = std::move(vk::raii::CommandBuffers(_device, allocInfo).front());

    vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

    // --- Execute --- //
    commandBuffer.begin(beginInfo);
    command(commandBuffer);
    commandBuffer.end();

    // --- Submit and wait --- //
    vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
    _queue.submit(submitInfo, nullptr);
    _queue.waitIdle();
}

////////////////////////////////////////////////////////////

vk::raii::ShaderModule VulkanContext::readShader(string shaderName) const
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

PFN_vkVoidFunction VulkanContext::getFunctionEXT(const char* funcName)
{
    auto func = _instance.getProcAddr(funcName);
    if (!func)
        FATAL("FunctionEXT {} not found", funcName);
    return func;
}

////////////////////////////////////////////////////////////
