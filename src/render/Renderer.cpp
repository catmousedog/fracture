#include "Renderer.hpp"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include "util/Log.hpp"
#include "util/Vulkan.hpp"

#include <cstdint>

////////////////////////////////////////////////////////////

namespace
{

constexpr uint32_t FRAMES_IN_FLIGHT = 2;

} // namespace

////////////////////////////////////////////////////////////

Renderer::Renderer(VulkanCore& vulkanCore, const Fractal& fractal, Window* window)
    : _fractal(fractal),
      _window(window),
      _vulkanCore(vulkanCore),
      _commandQueue(_vulkanCore, FRAMES_IN_FLIGHT),
      _vertexBuffer(_vulkanCore, _commandQueue),
      _vulkanImage(_vulkanCore, _commandQueue),
      _graphicsPipeline(_vulkanCore, _vulkanImage)
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

void Renderer::draw(Backend& backend, bool recompute)
{
    _commandQueue.waitForFences();

    // acquire next (possibly still presenting) image
    auto imageIndex = _vulkanCore.acquireNextImage(&_commandQueue);

    if (!imageIndex)
    {
        resize(backend);
        return;
    }
    // each image has its own semaphore to indicate it is ready for presentation
    auto& renderFinishedSemaphore = _vulkanCore.getRenderFinishedSemaphore(*imageIndex);

    /* ======================= RENDER ======================= */
    _commandQueue.reset();

    record(*imageIndex, backend, recompute);

    _commandQueue.submit(renderFinishedSemaphore);

    /* ======================= PRESENT ====================== */
    if (!_vulkanCore.present(*imageIndex))
    {
        resize(backend);
    }

    _commandQueue.nextFrame();
}

////////////////////////////////////////////////////////////

void Renderer::record(uint32_t imageIndex, Backend& backend, bool recompute)
{
    _commandQueue.begin();

    // // --- Recompute --- //
    if (recompute || _resized)
    {
        _resized = false;

        backend.recompute();
    }

    // --- Prepare and Begin Rendering --- //
    _vulkanCore.prepare(&_commandQueue, imageIndex); // Layout is ColorAttachmentOptimal

    // --- Bind Pipeline --- //
    _graphicsPipeline.bind(_commandQueue);

    // --- Draw --- //
    _vertexBuffer.draw(_commandQueue);

    // _ui.draw(); // UI SHOULD DRAW IN BETWEEN HERE SO SPLIT THIS FUNCTION

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
            .image         = _vulkanCore.getSwapImage(imageIndex),
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
    _vulkanImage.recreateComputeImage();

    // update descriptor sets (reassign)
    _graphicsPipeline.updateDescriptorSet();

    backend.resize();

    _resized = true;
}

////////////////////////////////////////////////////////////