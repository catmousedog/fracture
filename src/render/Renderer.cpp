#include "Renderer.hpp"

#include "util/Vulkan.hpp"

#include <cstdint>

////////////////////////////////////////////////////////////

namespace
{

constexpr uint32_t FRAMES_IN_FLIGHT = 2;

} // namespace

////////////////////////////////////////////////////////////

Renderer::Renderer(Window* window, const Fractal* fractal)
    : _fractal(fractal),
      _window(window),
      _vulkanCore(_window),
      _commandQueue(_vulkanCore, FRAMES_IN_FLIGHT),
      _vertexBuffer(_vulkanCore, _commandQueue),
      _vulkanImage(_vulkanCore, _commandQueue),
      _computePipeline(_vulkanCore, _vulkanImage, _fractal->getSettings()),
      _graphicsPipeline(_vulkanCore, _vulkanImage),
      _ui(&_vulkanCore, FRAMES_IN_FLIGHT)
{
    _vulkanCore.logInfo();
}

////////////////////////////////////////////////////////////

Renderer::~Renderer() = default;

////////////////////////////////////////////////////////////

void Renderer::draw()
{
    _commandQueue.waitForFences();

    // acquire next (possibly still presenting) image
    auto imageIndex = _vulkanCore.acquireNextImage(&_commandQueue);

    if (!imageIndex)
    {
        resize();
        return;
    }
    // each image has its own semaphore to indicate it is ready for presentation
    auto& renderFinishedSemaphore = _vulkanCore.getRenderFinishedSemaphore(*imageIndex);

    /* ======================= RENDER ======================= */
    _commandQueue.reset();

    record(*imageIndex);

    _commandQueue.submit(renderFinishedSemaphore);

    /* ======================= PRESENT ====================== */
    if (!_vulkanCore.present(*imageIndex))
    {
        resize();
    }

    _commandQueue.nextFrame();
}

////////////////////////////////////////////////////////////

void Renderer::record(uint32_t imageIndex)
{
    _commandQueue.begin();

    // --- Recompute --- //
    if (_dirty)
    {
        // --- Transition to General --- //
        util::transitionImageLayout(
            _commandQueue.getCommandBuffer(),
            {
                .srcStageMask  = vk::PipelineStageFlagBits2::eFragmentShader,
                .srcAccessMask = vk::AccessFlagBits2::eShaderRead,
                .dstStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
                .dstAccessMask = vk::AccessFlagBits2::eShaderWrite,
                .oldLayout     = vk::ImageLayout::eShaderReadOnlyOptimal,
                .newLayout     = vk::ImageLayout::eGeneral,
                .image         = _vulkanImage.getComputeImage(),
            }
        );

        // --- Bind Pipeline --- //
        _computePipeline.bind(_commandQueue);

        // --- Execute --- //
        auto     extent = _vulkanCore.getSwapChainExtent();
        uint32_t groupX = (extent.width + 15) / 16;
        uint32_t groupY = (extent.height + 15) / 16;
        _commandQueue.dispatch(groupX, groupY, 1);

        // --- Transition to ShaderReadOnlyOptimal --- //
        util::transitionImageLayout(
            _commandQueue.getCommandBuffer(),
            {
                .srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
                .srcAccessMask = vk::AccessFlagBits2::eShaderWrite,
                .dstStageMask  = vk::PipelineStageFlagBits2::eFragmentShader,
                .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
                .oldLayout     = vk::ImageLayout::eGeneral,
                .newLayout     = vk::ImageLayout::eShaderReadOnlyOptimal,
                .image         = _vulkanImage.getComputeImage(),
            }
        );
        _dirty = false;
    }

    // --- Prepare and Begin Rendering --- //
    _vulkanCore.prepare(&_commandQueue, imageIndex); // Layout is ColorAttachmentOptimal

    // --- Bind Pipeline --- //
    _graphicsPipeline.bind(_commandQueue);

    // --- Draw --- //
    _vertexBuffer.draw(_commandQueue);
    _ui.draw(_commandQueue);

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

void Renderer::resize()
{
    _vulkanCore.waitIdle();

    // recreate swapchain (resize)
    _vulkanCore.recreateSwapchain();

    // recreate image (reassign)
    _vulkanImage.recreateComputeImage();

    // update descriptor sets (reassign)
    _graphicsPipeline.updateDescriptorSet();
    _computePipeline.updateDescriptorSet();

    _dirty = true;
}

////////////////////////////////////////////////////////////