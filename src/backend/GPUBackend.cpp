#include "GPUBackend.hpp"
#include "util/Vulkan.hpp"
#include "vulkan/VulkanImage.hpp"

////////////////////////////////////////////////////////////

GPUBackend::GPUBackend(VulkanCore& vulkanCore, CommandQueue& commandQueue, VulkanImage& vulkanImage)
    : _vulkanCore(vulkanCore),
      _commandQueue(commandQueue),
      _computePipeline(_vulkanCore, vulkanImage)
{
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

void GPUBackend::resize()
{
    _computePipeline.updateDescriptorSet();
}

////////////////////////////////////////////////////////////