#include "GPUBackend.hpp"

#include "util/Vulkan.hpp"
#include "vulkan/VulkanImage.hpp"

////////////////////////////////////////////////////////////

GPUBackend::GPUBackend(VulkanCore& vulkanCore, VulkanImage& vulkanImage, VulkanQueue& vulkanQueue, Settings& settings)
    : Backend(settings),
      _vulkanCore(vulkanCore),
      _vulkanImage(vulkanImage),
      _vulkanQueue(vulkanQueue),
      _computePipeline(_vulkanCore, vulkanImage, _settings)
{
    uint32_t size = _computePipeline.constantSize<real>();
    _computePipeline.updatePipeline(size);

    // timestamp properties
    _timestampPeriod       = _vulkanCore.getPhysicalDevice().getProperties().limits.timestampPeriod;
    auto     props         = _vulkanCore.getPhysicalDevice().getQueueFamilyProperties();
    uint32_t timestampBits = props[_vulkanCore.getFamilyIndex()].timestampValidBits;
    _timestampMask         = (timestampBits >= 64) ? ~uint64_t{0} : (uint64_t{1} << timestampBits) - 1;

    // query pool
    _queryPool = vk::raii::QueryPool(
        _vulkanCore.getDevice(),
        vk::QueryPoolCreateInfo{
            .queryType  = vk::QueryType::eTimestamp,
            .queryCount = 2 * _vulkanQueue.getFramesInFlight(), // start + end per frame-in-flight
        }
    );
    _timestampReady.resize(_vulkanQueue.getFramesInFlight(), false);
}

////////////////////////////////////////////////////////////

void GPUBackend::onResize()
{
    _computePipeline.updateDescriptorSet();
}

////////////////////////////////////////////////////////////

void GPUBackend::onFractalChange()
{
    _computePipeline.readShader(_settings);
    uint32_t size = _computePipeline.constantSize<real>();
    _computePipeline.updatePipeline(size);
}

////////////////////////////////////////////////////////////

void GPUBackend::submitJob()
{
    uint32_t frameIndex = _vulkanQueue.getFrameIndex();
    uint32_t queryStart = 2 * frameIndex;

    vk::raii::CommandBuffer& commandBuffer = _vulkanQueue.getCommandBuffer();
    auto                     extent        = _vulkanImage.getExtent();
    float                    aspect        = static_cast<float>(extent.width) / extent.height;

    // --- Reset Query --- //
    commandBuffer.resetQueryPool(_queryPool, queryStart, 2);

    // --- Read Query --- //
    if (_timestampReady[frameIndex])
    {
        auto [result, ticks] = _queryPool.getResults<uint64_t>(
            queryStart, 2, 2 * sizeof(uint64_t), sizeof(uint64_t), vk::QueryResultFlagBits::e64
        );
        uint64_t delta = (ticks[1] - ticks[0]) & _timestampMask;
        _stats.ns      = static_cast<double>(delta) * _timestampPeriod;
    }
    else
    {
        _timestampReady[frameIndex] = true;
        _stats.ns                   = 0;
    }

    // --- Transition to General --- //
    util::transitionImageLayout(
        _vulkanQueue.getCommandBuffer(),
        {
            .srcStageMask  = vk::PipelineStageFlagBits2::eFragmentShader,
            .srcAccessMask = vk::AccessFlagBits2::eNone,
            .dstStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
            .dstAccessMask = vk::AccessFlagBits2::eShaderWrite,
            .oldLayout     = vk::ImageLayout::eUndefined,
            .newLayout     = vk::ImageLayout::eGeneral,
            .image         = _vulkanImage.getComputeImage(),
        }
    );

    // --- Bind Pipeline --- //
    _computePipeline.pushConstants<real>(_vulkanQueue, _settings);
    _computePipeline.bind(_vulkanQueue);

    // --- Execute --- //
    uint32_t groupX = (extent.width + 15) / 16;
    uint32_t groupY = (extent.height + 15) / 16;

    commandBuffer.writeTimestamp2(vk::PipelineStageFlagBits2::eComputeShader, _queryPool, queryStart);
    _vulkanQueue.dispatch(groupX, groupY, 1);
    commandBuffer.writeTimestamp2(vk::PipelineStageFlagBits2::eComputeShader, _queryPool, queryStart + 1);

    // --- Transition to ShaderReadOnlyOptimal --- //
    util::transitionImageLayout(
        _vulkanQueue.getCommandBuffer(),
        {
            .srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
            .srcAccessMask = vk::AccessFlagBits2::eShaderWrite,
            .dstStageMask  = vk::PipelineStageFlagBits2::eFragmentShader,
            .dstAccessMask = vk::AccessFlagBits2::eShaderSampledRead,
            .oldLayout     = vk::ImageLayout::eGeneral,
            .newLayout     = vk::ImageLayout::eShaderReadOnlyOptimal,
            .image         = _vulkanImage.getComputeImage(),
        }
    );
}

////////////////////////////////////////////////////////////