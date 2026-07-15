#include "VulkanContext.hpp"

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

namespace
{

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
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
        Log::info("Validation layer VERB  {}: {}", objectType, msg);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        Log::info("Validation layer INFO  {}: {}", objectType, msg);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        Log::warn("Validation layer WARN  {}: {}", objectType, msg);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        Log::error("Validation layer ERROR {}: {}", objectType, msg);
        break;
    default:
        Log::error("Validation layer DEFLT {}: {}", objectType, msg);
        break;
    }

    return VK_FALSE;
}

vector<char> readFile(const string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file!");
    }

    vector<char> buffer(file.tellg());

    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    file.close();

    return buffer;
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
    createCommandpool();
    createCommandBuffers();
}

void VulkanContext::drawFrame() { }

void VulkanContext::waitIdle()
{
    _device.waitIdle();
}

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
    {
        throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));
    }

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
    {
        throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
    }

    // create instance
    vk::InstanceCreateInfo info{
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = layerCount,
        .ppEnabledLayerNames     = layers.data(),
        .enabledExtensionCount   = extensionsCount,
        .ppEnabledExtensionNames = extensions.data(),
    };

    _instance = vk::raii::Instance(_context, info);
}

void VulkanContext::createDebugCallback()
{
    // create debug callback for any severity and type
    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{
        .messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType     = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = &debugCallback,
    };

    _debugMessenger = _instance.createDebugUtilsMessengerEXT(debugMessengerCreateInfo);
}

void VulkanContext::createSurface(GLFWwindow* window)
{
    // create Vulkan surface for GLFW window

    // GLFW cannot use raii
    VkSurfaceKHR surface;
    VkResult     res = glfwCreateWindowSurface(*_instance, window, nullptr, &surface);
    if (res != VK_SUCCESS)
        FATAL("glfwCreateWindowSurface failed");

    // convert to raii
    _surface = vk::raii::SurfaceKHR(_instance, surface);
}

void VulkanContext::pickPhysicalDevice()
{
    // --- Get all physical devices --- //
    vector<vk::raii::PhysicalDevice> devices = _instance.enumeratePhysicalDevices();

    if (devices.empty())
        FATAL("No Vulkan GPU found");

    // --- Pick first device --- //
    _physicalDevice = devices[0];
}

void VulkanContext::createLogicalDevice()
{
    // --- Obtain queue family properties --- //
    vector<vk::QueueFamilyProperties> queueFamilies = _physicalDevice.getQueueFamilyProperties();

    // --- Find queue family with graphics and present support --- //
    _family = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilies.size(); i++)
    {
        VkBool32 presentSupport = _physicalDevice.getSurfaceSupportKHR(i, _surface);
        if ((queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) && presentSupport)
        {
            _family = i;
            break;
        }
    }
    if (_family == UINT32_MAX)
        FATAL("No graphics+present queue family found");

    // --- Create single queue --- //
    float                     priority = 1.0f;
    vk::DeviceQueueCreateInfo queueInfo{.queueFamilyIndex = _family, .queueCount = 1, .pQueuePriorities = &priority};

    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // --- Create logical device --- //
    vk::DeviceCreateInfo info{
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &queueInfo,
        .enabledExtensionCount   = 1,
        .ppEnabledExtensionNames = deviceExtensions
    };

    _device = _physicalDevice.createDevice(info, nullptr);

    // --- Obtain queue --- //
    _queue = _device.getQueue(_family, 0);
}

void VulkanContext::createSwapchain(uint32_t width, uint32_t height)
{
    // --- Obtain surface formats --- //
    vector<vk::SurfaceFormatKHR> formats = _physicalDevice.getSurfaceFormatsKHR();

    if (formats.empty())
        FATAL("No surface formats available");

    // --- Pick format (prefer B8G8R8A8_SRGB / SRGB_NONLINEAR) --- //
    vk::SurfaceFormatKHR format = formats[0]; // default
    for (auto& f : formats)
    {
        if (f.format == vk::Format::eB8G8R8A8Srgb && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            format = f;
            break;
        }
    }

    // --- Obtain surface capabilities --- //
    vk::SurfaceCapabilitiesKHR caps = _physicalDevice.getSurfaceCapabilitiesKHR(_surface);
    _imageCount                     = std::max(caps.minImageCount + 1, caps.maxImageCount);
    if (_imageCount == 0)
        FATAL("Vulkan surface does not support any images");

    vk::Extent2D swapchainExtent = {width, height};

    // --- Create swapchain --- //
    vk::SwapchainCreateInfoKHR info{};
    info.surface               = *_surface;
    info.minImageCount         = _imageCount;
    info.imageFormat           = format.format;
    info.imageColorSpace       = format.colorSpace;
    info.imageExtent           = swapchainExtent;
    info.imageArrayLayers      = 1;
    info.imageUsage            = vk::ImageUsageFlagBits::eColorAttachment;
    info.imageSharingMode      = vk::SharingMode::eExclusive; // single queue family
    info.queueFamilyIndexCount = 1;
    info.pQueueFamilyIndices   = &_family;
    info.preTransform          = caps.currentTransform;
    info.compositeAlpha        = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    info.presentMode           = vk::PresentModeKHR::eFifo; // vsync, always supported
    info.clipped               = VK_TRUE;

    _swapchain = _device.createSwapchainKHR(info, nullptr);

    // --- Obtain images --- //
    _swapImages = _swapchain.getImages();

    // --- Create image views --- //
    _swapImageViews.reserve(_swapImages.size());
    for (const auto& image : _swapImages)
    {
        vk::ImageViewCreateInfo ivInfo{};
        ivInfo.sType                           = vk::StructureType::eImageViewCreateInfo;
        ivInfo.image                           = image;
        ivInfo.viewType                        = vk::ImageViewType::e2D;
        ivInfo.format                          = format.format;
        ivInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
        ivInfo.subresourceRange.baseMipLevel   = 0;
        ivInfo.subresourceRange.levelCount     = 1;
        ivInfo.subresourceRange.baseArrayLayer = 0;
        ivInfo.subresourceRange.layerCount     = 1;

        _swapImageViews.emplace_back(_device, ivInfo);
    }
}

void VulkanContext::createCommandpool()
{
    // --- Create command buffer pool --- //
    vk::CommandPoolCreateInfo info{};
    info.queueFamilyIndex = _family;
    info.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

    _commandPool = _device.createCommandPool(info, nullptr);
}

void VulkanContext::createCommandBuffers()
{
    _commandBuffers.clear();
    vk::CommandBufferAllocateInfo info{
        .commandPool = _commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = _imageCount
    };
    _commandBuffers = vk::raii::CommandBuffers(_device, info);
}

void VulkanContext::recordCommandBuffers()
{
    // TODO
}

void VulkanContext::createPipeline()
{
    const vector<char> code = readFile("shaders/slang.spv");

    // VkShaderModuleCreateInfo createInfo{};
    // createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    vk::ShaderModuleCreateInfo info{
        .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };

    _shader = _device.createShaderModule(info, nullptr);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex, .module = _shader, .pName = "vertMain"
    };

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment, .module = _shader, .pName = "fragMain"
    };
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo   vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
    VkPipelineViewportStateCreateInfo      viewportState{.viewportCount = 1, .scissorCount = 1};

    VkPipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable        = false,
        .rasterizerDiscardEnable = false,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_BACK_BIT,
        .frontFace               = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable         = false,
        .lineWidth               = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT, .sampleShadingEnable = vk::False
    };
}

PFN_vkVoidFunction VulkanContext::getFunctionEXT(const char* funcName)
{
    auto func = _instance.getProcAddr(funcName);
    if (!func)
        FATAL("FunctionEXT {} not found", funcName);
    return func;
}
