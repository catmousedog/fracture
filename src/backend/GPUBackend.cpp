#include "GPUBackend.hpp"

#include "util/Log.hpp"
#include "util/Vulkan.hpp"
#include "vulkan/VulkanImage.hpp"

////////////////////////////////////////////////////////////

GPUBackend::GPUBackend(VulkanCore& vulkanCore, CommandQueue& commandQueue, Swappable<VulkanImage>& vulkanImages)
    : _vulkanCore(vulkanCore),
      _commandQueue(commandQueue),
      _vulkanImages(vulkanImages),
      _computePipeline(_vulkanCore, vulkanImages)
{
    _computePipeline.updatePipeline(sizeof(Constants)); // fixed for now (same push constants always)

    _queryPool = vk::raii::QueryPool(
        _vulkanCore.getDevice(),
        vk::QueryPoolCreateInfo{
            .queryType  = vk::QueryType::eTimestamp,
            .queryCount = 2 * _commandQueue.getFramesInFlight(), // start + end per frame-in-flight
        }
    );
    _timestampPeriod = _vulkanCore.getPhysicalDevice().getProperties().limits.timestampPeriod;
    _queryReset.resize(_commandQueue.getFramesInFlight(), false);
}

////////////////////////////////////////////////////////////

void GPUBackend::recompute()
{
    vk::raii::CommandBuffer& commandBuffer = _commandQueue.getCommandBuffer();
    uint32_t                 frameIndex    = _commandQueue.getFrameIndex();
    uint32_t                 startQuery    = 2 * frameIndex;
    VulkanImage&             image         = _vulkanImages.back();

    // --- Reset (and read) Query --- //
    commandBuffer.resetQueryPool(_queryPool, startQuery, 2);
    if (_queryReset[frameIndex])
    {
        auto [result, ticks] = _queryPool.getResults<uint64_t>(
            startQuery, 2, 2 * sizeof(uint64_t), sizeof(uint64_t), vk::QueryResultFlagBits::e64
        );
        if (result != vk::Result::eSuccess)
            FATAL("QueryPool::getResults returned {}", vk::to_string(result));

        _stats.ns = (ticks[1] - ticks[0]) * _timestampPeriod;
    }

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
            .image         = image.getComputeImage(),
        }
    );

    // --- Store this compute --- // (Will have to be moved later to after the dispatch has finished)
    auto extent  = image.getExtent();
    _lastCompute = {
        .fractal     = _settings.fractal,                                  //
        .computeSize = _settings.computeSize(extent.width, extent.height), //
    };
    float aspect = static_cast<float>(extent.width) / extent.height;

    // --- Bind Pipeline --- //
    Constants constants = makeConstants(_settings.fractal, _lastCompute.computeSize, aspect);
    _computePipeline.pushConstants(_commandQueue, constants);
    _computePipeline.bind(_commandQueue);

    // --- Execute --- //
    uint32_t groupX = (_lastCompute.computeSize.x + 15) / 16;
    uint32_t groupY = (_lastCompute.computeSize.y + 15) / 16;
    commandBuffer.writeTimestamp2(vk::PipelineStageFlagBits2::eComputeShader, _queryPool, startQuery);
    _commandQueue.dispatch(groupX, groupY, 1);
    commandBuffer.writeTimestamp2(vk::PipelineStageFlagBits2::eComputeShader, _queryPool, startQuery + 1);

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
            .image         = image.getComputeImage(),
        }
    );

    // this query will be recorded and reset in the next frame
    _queryReset[frameIndex] = true;

    // swap images
    _vulkanImages.swap();
}

////////////////////////////////////////////////////////////

void GPUBackend::onResize()
{
    _computePipeline.updateDescriptorSet();
}

////////////////////////////////////////////////////////////

GPUBackend::Constants GPUBackend::makeConstants(const Fractal& fractal, glm::uvec2 computeSize, float aspect)
{
    Constants constants;

    constants.offset      = glm::vec2(fractal.offsetX, fractal.offsetY);
    constants.zoom        = fractal.zoom;
    constants.maxIter     = (int)fractal.maxIter;
    constants.computeSize = computeSize;
    constants.aspect      = aspect;

    return constants;
}

////////////////////////////////////////////////////////////