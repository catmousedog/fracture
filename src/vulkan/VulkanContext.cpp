#include "VulkanContext.hpp"

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vulkan_to_string.hpp>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <string>
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

vector<char> readShader(const string& filename)
{
    string shaderPath = SHADER_DIR + filename;

    // seek end of file
    std::ifstream file(shaderPath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        FATAL("failed to open shader at {}!", shaderPath);

    vector<char> buffer(file.tellg());

    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    file.close();

    return buffer;
}

////////////////////////////////////////////////////////////

} // namespace

////////////////////////////////////////////////////////////

VulkanContext::VulkanContext(GLFWwindow* window, const VulkanContextInfo& info)
    : _window(window),
      _framesInFlight(info.framesInFlight)
{
    createInstance();
    createDebugCallback();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createDescriptorSetLayout();
    createPipeline();
    createCommandPool();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffer();
    createSyncObjects();
}

////////////////////////////////////////////////////////////

void VulkanContext::drawFrame()
{
    // each frame-in-flight worker has its own command buffer, fence and present semaphore
    auto& commandBuffer            = _commandBuffers[_frameIndex];
    auto& drawFence                = _drawFences[_frameIndex];
    auto& presentCompleteSemaphore = _presentCompleteSemaphores[_frameIndex];

    auto fenceResult = _device.waitForFences(*drawFence, vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess)
        Log::warn("vk::Device::waitForFences returned {} !", vk::to_string(fenceResult));

    auto [acquireResult, imageIndex] = _swapchain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphore, nullptr);

    if (acquireResult == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapchain();
        return;
    }
    else if (acquireResult == vk::Result::eSuboptimalKHR)
    {
        Log::warn("vk::SwapchainKHR::acquireNextImage returned vk::Result::eSuboptimalKHR");
    }
    else if (acquireResult != vk::Result::eSuccess)
    {
        Log::error("vk::SwapchainKHR::acquireNextImage returned {}", vk::to_string(acquireResult));
    }

    // each image has its own semaphore to indicate it is ready for presentation
    auto& renderFinishedSemaphore = _renderFinishedSemaphores[imageIndex];

    // update uniform buffer
    updateUniformBuffer(_frameIndex);

    commandBuffer.reset();
    recordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo   submitInfo{
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &*presentCompleteSemaphore,
        .pWaitDstStageMask    = &waitDestinationStageMask,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &*commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &*renderFinishedSemaphore
    };

    _device.resetFences(*drawFence);
    _queue.submit(submitInfo, *drawFence);

    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*renderFinishedSemaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &*_swapchain,
        .pImageIndices      = &imageIndex
    };
    auto presentResult = _queue.presentKHR(presentInfoKHR);
    if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR ||
        _frameBufferResized)
    {
        _frameBufferResized = false;
        recreateSwapchain();
        return;
    }
    else if (presentResult != vk::Result::eSuccess)
    {
        Log::error("vk::Queue::presentKHR returned {}", vk::to_string(presentResult));
    }

    _frameIndex = (_frameIndex + 1) % _framesInFlight;
}

////////////////////////////////////////////////////////////

void VulkanContext::waitIdle()
{
    _queue.waitIdle();
}

////////////////////////////////////////////////////////////

void VulkanContext::recreateSwapchain()
{
    waitIdle();

    // manually clear to avoid vk::NativeWindowInUseKHRError
    // vk::raii will automatically destruct these,
    // but there will be 2 swapchains assigned to a single surface just before this
    _swapchain.clear();
    _swapImageViews.clear();

    createSwapchain();
}

////////////////////////////////////////////////////////////

void VulkanContext::resizeFramebuffer(uint32_t width, uint32_t height)
{
    _frameBufferResized = true;
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
    // GLFW cannot use raii
    VkSurfaceKHR surface;
    VkResult     res = glfwCreateWindowSurface(*_instance, _window, nullptr, &surface);
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
    // enable to avoid validation layer errors
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

    // image count
    uint32_t imageCount = std::max(caps.minImageCount + 1, caps.maxImageCount);
    if (imageCount == 0)
        FATAL("Vulkan surface does not support any images");

    // --- Set swapchain extent --- //
    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);
    _swapChainExtent.width  = width;
    _swapChainExtent.height = height;

    // --- Create swapchain --- //
    vk::SwapchainCreateInfoKHR swapChainInfo{
        .surface               = *_surface,
        .minImageCount         = imageCount,
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
    _swapImageViews.reserve(imageCount);
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

void VulkanContext::createDescriptorSetLayout()
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding{
        .binding         = 0,
        .descriptorType  = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags      = vk::ShaderStageFlagBits::eVertex
    };
    vk::DescriptorSetLayoutCreateInfo layoutInfo{.bindingCount = 1, .pBindings = &uboLayoutBinding};
    _descriptorSetLayout = vk::raii::DescriptorSetLayout(_device, layoutInfo);
}

////////////////////////////////////////////////////////////

void VulkanContext::createPipeline()
{
    const vector<char> code = readShader("shader.spv");

    vk::ShaderModuleCreateInfo shaderModuleInfo{
        .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };
    _shader = vk::raii::ShaderModule(_device, shaderModuleInfo);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex, .module = _shader, .pName = "vertMain"
    };
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment, .module = _shader, .pName = "fragMain"
    };
    vk::PipelineShaderStageCreateInfo shaderStagesInfo[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto                                   bindingDescription    = Vertex::getBindingDescription();
    auto                                   attributeDescriptions = Vertex::getAttributeDescriptions();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions    = attributeDescriptions.data()
    };
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{.topology = vk::PrimitiveTopology::eTriangleList};
    vk::PipelineViewportStateCreateInfo      viewportInfo{.viewportCount = 1, .scissorCount = 1};

    vk::PipelineRasterizationStateCreateInfo rasterizationInfo{
        .depthClampEnable        = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode             = vk::PolygonMode::eFill,
        .cullMode                = vk::CullModeFlagBits::eBack,
        .frontFace               = vk::FrontFace::eCounterClockwise,
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

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = 1, .pSetLayouts = &*_descriptorSetLayout, .pushConstantRangeCount = 0
    };
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
    _commandPool = vk::raii::CommandPool(_device, poolInfo);
}

////////////////////////////////////////////////////////////

void VulkanContext::createVertexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(_vertices[0]) * _vertices.size();

    auto [stagingBuffer, stagingBufferMemory] = createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(dataStaging, _vertices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();

    std::tie(_vertexBuffer, _vertexBufferMemory) = createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    copyBuffer(stagingBuffer, _vertexBuffer, bufferSize);
}

////////////////////////////////////////////////////////////

void VulkanContext::createIndexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(_indices[0]) * _indices.size();

    auto [stagingBuffer, stagingBufferMemory] = createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    void* data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, _indices.data(), (size_t)bufferSize);
    stagingBufferMemory.unmapMemory();

    std::tie(_indexBuffer, _indexBufferMemory) = createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    copyBuffer(stagingBuffer, _indexBuffer, bufferSize);
}

////////////////////////////////////////////////////////////

void VulkanContext::createUniformBuffers()
{

    for (size_t i = 0; i < _framesInFlight; i++)
    {
        vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
        auto [buffer, bufferMem]  = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
        _uniformBuffers.emplace_back(std::move(buffer));
        _uniformBuffersMemory.emplace_back(std::move(bufferMem));
        _uniformBuffersMapped.emplace_back(_uniformBuffersMemory.back().mapMemory(0, bufferSize));
    }
}

////////////////////////////////////////////////////////////

void VulkanContext::createDescriptorPool()
{
    vk::DescriptorPoolSize poolSize{.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = _framesInFlight};
    vk::DescriptorPoolCreateInfo poolInfo{
        .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets       = _framesInFlight,
        .poolSizeCount = 1,
        .pPoolSizes    = &poolSize
    };
    _descriptorPool = vk::raii::DescriptorPool(_device, poolInfo);
}

////////////////////////////////////////////////////////////

void VulkanContext::createDescriptorSets()
{
    std::vector<vk::DescriptorSetLayout> layouts(_framesInFlight, *_descriptorSetLayout);
    vk::DescriptorSetAllocateInfo        allocInfo{
        .descriptorPool     = _descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts        = layouts.data()
    };
    _descriptorSets = _device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < _framesInFlight; i++)
    {
        vk::DescriptorBufferInfo bufferInfo{
            .buffer = _uniformBuffers[i], .offset = 0, .range = sizeof(UniformBufferObject)
        };
        vk::WriteDescriptorSet descriptorWrite{
            .dstSet          = _descriptorSets[i],
            .dstBinding      = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo     = &bufferInfo
        };
        _device.updateDescriptorSets(descriptorWrite, {});
    }
}

////////////////////////////////////////////////////////////

void VulkanContext::createCommandBuffer()
{
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = _commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = _framesInFlight
    };
    _commandBuffers = _device.allocateCommandBuffers(allocInfo);
}

////////////////////////////////////////////////////////////

void VulkanContext::recordCommandBuffer(uint32_t imageIndex)
{
    auto& commandBuffer = _commandBuffers[_frameIndex];
    auto& descriptorSet = _descriptorSets[_frameIndex];

    commandBuffer.begin({});

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

    commandBuffer.beginRendering(renderingInfo);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipeline);
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
    commandBuffer.bindVertexBuffers(0, *_vertexBuffer, {0});
    commandBuffer.bindIndexBuffer(*_indexBuffer, 0, vk::IndexType::eUint16);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, *descriptorSet, nullptr);
    commandBuffer.drawIndexed(static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);
    commandBuffer.endRendering();

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
    commandBuffer.end();
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
    _commandBuffers[_frameIndex].pipelineBarrier2(dependency_info);
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

void VulkanContext::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size)
{
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = _commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1
    };
    vk::raii::CommandBuffer commandCopyBuffer = std::move(_device.allocateCommandBuffers(allocInfo).front());
    commandCopyBuffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy(0, 0, size));
    commandCopyBuffer.end();
    _queue.submit(vk::SubmitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandCopyBuffer}, nullptr);
    _queue.waitIdle();
}

////////////////////////////////////////////////////////////

void VulkanContext::updateUniformBuffer(uint32_t frameIndex)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto  currentTime = std::chrono::high_resolution_clock::now();
    float time        = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view  = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj  = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(_swapChainExtent.width) / static_cast<float>(_swapChainExtent.height),
        0.1f,
        10.0f
    );
    ubo.proj[1][1] *= -1;

    memcpy(_uniformBuffersMapped[frameIndex], &ubo, sizeof(ubo));
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
