#include "Renderer.hpp"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "util/Vulkan.hpp"

#include <cstdint>

////////////////////////////////////////////////////////////

namespace
{

constexpr uint32_t FRAMES_IN_FLIGHT = 2;

} // namespace

////////////////////////////////////////////////////////////

Renderer::Renderer(VulkanCore& vulkanCore, Window* window)
    : _window(window),
      _vulkanCore(vulkanCore),
      _graphicsQueue(_vulkanCore, FRAMES_IN_FLIGHT),
      _vulkanImages(_vulkanCore, _graphicsQueue),
      _vertexBuffer(_vulkanCore, _graphicsQueue),
      _graphicsPipeline(_vulkanCore, _vulkanImages, FRAMES_IN_FLIGHT)
{
    initUI();
}

////////////////////////////////////////////////////////////

Renderer::~Renderer()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

////////////////////////////////////////////////////////////

void Renderer::initUI()
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
    _uiDescriptorPool = _vulkanCore.getDevice().createDescriptorPool(pool_info);

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(_window->getGLFWWindow(), true);

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo = {
        .colorAttachmentCount    = 1,                                     //
        .pColorAttachmentFormats = &_vulkanCore.getSurfaceFormat().format //
    };

    ImGui_ImplVulkan_InitInfo initInfo{
        .Instance            = *_vulkanCore.getInstance(),
        .PhysicalDevice      = *_vulkanCore.getPhysicalDevice(),
        .Device              = *_vulkanCore.getDevice(),
        .QueueFamily         = _vulkanCore.getGraphiscsFamilyIndex(),
        .Queue               = *_vulkanCore.getGraphicsQueue(),
        .DescriptorPool      = *_uiDescriptorPool,
        .MinImageCount       = FRAMES_IN_FLIGHT,
        .ImageCount          = FRAMES_IN_FLIGHT,
        .PipelineInfoMain    = {.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo},
        .UseDynamicRendering = true
    };

    ImGui_ImplVulkan_Init(&initInfo);
}

////////////////////////////////////////////////////////////

void Renderer::beginUIFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

////////////////////////////////////////////////////////////

void Renderer::draw(Backend& backend)
{
    _graphicsQueue.waitForFences();

    // acquire next (possibly still presenting) image
    auto swapChainIndex = _vulkanCore.acquireNextImage(&_graphicsQueue);

    if (!swapChainIndex)
    {
        resize(backend);
        return;
    }
    // each image has its own semaphore to indicate it is ready for presentation
    auto& renderFinishedSemaphore = _vulkanCore.getRenderFinishedSemaphore(*swapChainIndex);

    /* ======================= RENDER ======================= */
    _graphicsQueue.reset();

    record(*swapChainIndex, backend);

    _graphicsQueue.submit(renderFinishedSemaphore, backend.getReady());

    /* ======================= PRESENT ====================== */
    if (!_vulkanCore.presentImage(*swapChainIndex))
    {
        resize(backend);
    }

    _graphicsQueue.nextFrame();
}

////////////////////////////////////////////////////////////

void Renderer::record(uint32_t swapChainIndex, Backend& backend)
{
    _graphicsQueue.begin();

    // --- Recompute --- //
    backend.update();

    // --- Prepare and Begin Rendering --- //
    _vulkanCore.prepareImage(&_graphicsQueue, swapChainIndex); // Layout is ColorAttachmentOptimal

    // --- Update Uniform Buffer --- //
    const auto& settings = backend.getSettings();
    const auto& lastJob  = backend.getLastJob();

    const Fractal& curr = settings.fractal;
    const Fractal& prev = lastJob.fractal;

    auto                                  extent = _vulkanImages.front().getExtent();
    GraphicsPipeline::UniformBufferObject ubo{
        .offset     = {curr.offsetX, curr.offsetY},
        .prevOffset = {prev.offsetX, prev.offsetY},
        .zoom       = curr.zoom,
        .prevZoom   = prev.zoom,
        .aspect     = float(extent.width) / extent.height
    };
    _graphicsPipeline.updateUniformBuffer(_graphicsQueue, ubo);

    // --- Bind Pipeline --- //
    _graphicsPipeline.bind(_graphicsQueue);

    // --- Draw --- //
    _vertexBuffer.draw(_graphicsQueue);

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *_graphicsQueue.getCommandBuffer());

    // --- End Rendering --- //
    _graphicsQueue.endRendering();

    // --- Transition to PresentSrcKHR --- //
    util::transitionImageLayout(
        _graphicsQueue.getCommandBuffer(),
        {
            .srcStageMask  = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
            .dstStageMask  = vk::PipelineStageFlagBits2::eBottomOfPipe,
            .dstAccessMask = {},
            .oldLayout     = vk::ImageLayout::eColorAttachmentOptimal,
            .newLayout     = vk::ImageLayout::ePresentSrcKHR,
            .image         = _vulkanCore.getSwapImage(swapChainIndex),
        }
    );
    _graphicsQueue.end();
}

////////////////////////////////////////////////////////////

void Renderer::resize(Backend& backend)
{
    _vulkanCore.waitIdle();

    // recreate swapchain (resize)
    _vulkanCore.recreateSwapchain();

    // recreate image (reassign)
    for (auto& image : _vulkanImages)
        image.recreateComputeImage();

    // update descriptor sets (reassign)
    _graphicsPipeline.updateDescriptorSet();

    backend.resize();
}

////////////////////////////////////////////////////////////