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
      _commandQueue(_vulkanCore, FRAMES_IN_FLIGHT),
      _vulkanImages(_vulkanCore, _commandQueue),
      _vertexBuffer(_vulkanCore, _commandQueue),
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
        .QueueFamily         = _vulkanCore.getFamilyIndex(),
        .Queue               = *_vulkanCore.getQueue(),
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
    _commandQueue.waitForFences();

    // acquire next (possibly still presenting) image
    auto swapChainIndex = _vulkanCore.acquireNextImage(&_commandQueue);

    if (!swapChainIndex)
    {
        resize(backend);
        return;
    }
    // each image has its own semaphore to indicate it is ready for presentation
    auto& renderFinishedSemaphore = _vulkanCore.getRenderFinishedSemaphore(*swapChainIndex);

    /* ======================= RENDER ======================= */
    _commandQueue.reset();

    record(*swapChainIndex, backend);

    _commandQueue.submit(renderFinishedSemaphore);

    /* ======================= PRESENT ====================== */
    if (!_vulkanCore.present(*swapChainIndex))
    {
        resize(backend);
    }

    _commandQueue.nextFrame();
}

////////////////////////////////////////////////////////////

void Renderer::record(uint32_t swapChainIndex, Backend& backend)
{
    _commandQueue.begin();

    // --- Recompute --- //
    backend.record();

    // --- Prepare and Begin Rendering --- //
    _vulkanCore.prepare(&_commandQueue, swapChainIndex); // Layout is ColorAttachmentOptimal

    // --- Update Uniform Buffer --- //
    const auto& settings    = backend.getSettings();
    const auto& lastCompute = backend.getLastCompute();

    const Fractal& curr = settings.fractal;
    const Fractal& prev = lastCompute.fractal;

    auto                                  extent = _vulkanImages.front().getExtent();
    GraphicsPipeline::UniformBufferObject ubo{
        .offset      = {curr.offsetX, curr.offsetY},
        .prevOffset  = {prev.offsetX, prev.offsetY},
        .zoom        = curr.zoom,
        .prevZoom    = prev.zoom,
        .computeSize = lastCompute.computeSize,
        .aspect      = float(extent.width) / extent.height
    };
    _graphicsPipeline.updateUniformBuffer(_commandQueue, ubo);

    // --- Bind Pipeline --- //
    _graphicsPipeline.bind(_commandQueue);

    // --- Draw --- //
    _vertexBuffer.draw(_commandQueue);

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *_commandQueue.getCommandBuffer());

    // --- End Rendering --- //
    _commandQueue.endRendering();

    // --- Transition to PresentSrcKHR --- //
    util::transitionImageLayout(
        _commandQueue.getCommandBuffer(),
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
    _commandQueue.end();
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