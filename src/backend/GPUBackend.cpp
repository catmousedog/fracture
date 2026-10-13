#include "GPUBackend.hpp"

#include "util/Vulkan.hpp"
#include "vulkan/VulkanImage.hpp"

////////////////////////////////////////////////////////////

GPUBackend::GPUBackend(VulkanCore& vulkanCore, CommandQueue& commandQueue, VulkanImage& vulkanImage)
    : _vulkanCore(vulkanCore),
      _commandQueue(commandQueue),
      _computePipeline(_vulkanCore, vulkanImage)
{
    _computePipeline.updatePipeline(sizeof(Constants)); // fixed for now (same push constants always)
}

////////////////////////////////////////////////////////////

void GPUBackend::recompute()
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
            .image         = _computePipeline.getImage().getComputeImage(),
        }
    );

    // --- Bind Pipeline --- //
    Constants constants = makeConstants(_fractal);
    _computePipeline.pushConstants(_commandQueue, constants);
    _computePipeline.bind(_commandQueue);

    // --- Execute --- //
    auto     extent = _computePipeline.getImage().getExtent();
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
            .image         = _computePipeline.getImage().getComputeImage(),
        }
    );
}

////////////////////////////////////////////////////////////

void GPUBackend::onResize()
{
    _computePipeline.updateDescriptorSet();
}

////////////////////////////////////////////////////////////

GPUBackend::Constants GPUBackend::makeConstants(const Fractal& fractal)
{
    Constants constants;

    constants.offset  = glm::vec2(fractal.offsetX, fractal.offsetY);
    constants.zoom    = fractal.zoom;
    constants.maxIter = (int)fractal.maxIter;

    return constants;
}

////////////////////////////////////////////////////////////