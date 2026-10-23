#include "GPUBackend.hpp"

#include "util/Vulkan.hpp"
#include "vulkan/VulkanImage.hpp"

////////////////////////////////////////////////////////////

GPUBackend::GPUBackend(
    VulkanCore& vulkanCore, Swappable<VulkanImage>& vulkanImages, GraphicsQueue& graphicsQueue, Settings& settings
)
    : Backend(settings),
      _vulkanCore(vulkanCore),
      _vulkanImages(vulkanImages),
      _computeQueue(_vulkanCore),
      _graphicsQueue(graphicsQueue),
      _computePipeline(_vulkanCore, vulkanImages, _settings.fractal.type)
{
    _computePipeline.updatePipeline(sizeof(Constants)); // fixed for now (same push constants always)

    // timestamp properties
    _timestampPeriod       = _vulkanCore.getPhysicalDevice().getProperties().limits.timestampPeriod;
    auto     props         = _vulkanCore.getPhysicalDevice().getQueueFamilyProperties();
    uint32_t timestampBits = props[_vulkanCore.getComputeFamilyIndex()].timestampValidBits;
    _timestampMask         = (timestampBits >= 64) ? ~uint64_t{0} : (uint64_t{1} << timestampBits) - 1;

    // query pool
    _queryPool = vk::raii::QueryPool(
        _vulkanCore.getDevice(),
        vk::QueryPoolCreateInfo{
            .queryType  = vk::QueryType::eTimestamp,
            .queryCount = 2, // start + end per job
        }
    );
}

////////////////////////////////////////////////////////////

void GPUBackend::onResize()
{
    _computePipeline.updateDescriptorSet();
}

////////////////////////////////////////////////////////////

void GPUBackend::onFractalChange()
{
    _computePipeline.readShader(_settings.fractal.type);
    _computePipeline.updatePipeline(sizeof(Constants));
}

////////////////////////////////////////////////////////////

void GPUBackend::onJobDone()
{
    // --- Read Query --- //
    auto [result, ticks] =
        _queryPool.getResults<uint64_t>(0, 2, 2 * sizeof(uint64_t), sizeof(uint64_t), vk::QueryResultFlagBits::e64);
    uint64_t delta = (ticks[1] - ticks[0]) & _timestampMask;
    _stats.ns      = static_cast<double>(delta) * _timestampPeriod;

    // previous = current
    _lastJob  = _job;
    _frontJob = _submittedJob;

    // swap images
    _vulkanImages.swap();
}

////////////////////////////////////////////////////////////

void GPUBackend::submitJob()
{
    vk::raii::CommandBuffer& commandBuffer = _computeQueue.getCommandBuffer();
    VulkanImage&             image         = _vulkanImages.back();
    auto                     extent        = image.getExtent();
    float                    aspect        = static_cast<float>(extent.width) / extent.height;

    // --- Store this compute --- //
    _job = {
        .fractal = _settings.fractal, //
    };

    // --- Reset Compute Queue --- //
    _computeQueue.reset();
    _computeQueue.begin();

    // --- Reset Query --- //
    commandBuffer.resetQueryPool(_queryPool, 0, 2);

    // --- Transition to General --- //
    util::transitionImageLayout(
        _computeQueue.getCommandBuffer(),
        {
            .srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
            .srcAccessMask = vk::AccessFlagBits2::eNone,
            .dstStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
            .dstAccessMask = vk::AccessFlagBits2::eShaderWrite,
            .oldLayout     = vk::ImageLayout::eShaderReadOnlyOptimal,
            .newLayout     = vk::ImageLayout::eGeneral,
            .image         = image.getComputeImage(),
        }
    );

    // --- Bind Pipeline --- //
    Constants constants = makeConstants(_job.fractal, aspect);
    _computePipeline.pushConstants(_computeQueue, constants);
    _computePipeline.bind(_computeQueue);

    // --- Execute --- //
    uint32_t groupX = (extent.width + 15) / 16;
    uint32_t groupY = (extent.height + 15) / 16;
    commandBuffer.writeTimestamp2(vk::PipelineStageFlagBits2::eComputeShader, _queryPool, 0);
    _computeQueue.dispatch(groupX, groupY, 1);
    commandBuffer.writeTimestamp2(vk::PipelineStageFlagBits2::eComputeShader, _queryPool, 1);

    // --- Transition to ShaderReadOnlyOptimal --- //
    util::transitionImageLayout(
        _computeQueue.getCommandBuffer(),
        {
            .srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
            .srcAccessMask = vk::AccessFlagBits2::eShaderWrite,
            .dstStageMask  = vk::PipelineStageFlagBits2::eAllCommands,
            .dstAccessMask = vk::AccessFlagBits2::eNone,
            .oldLayout     = vk::ImageLayout::eGeneral,
            .newLayout     = vk::ImageLayout::eShaderReadOnlyOptimal,
            .image         = image.getComputeImage(),
        }
    );

    // --- Submit Compute Queue --- //
    _computeQueue.end();
    _computeQueue.submit(_graphicsQueue.getRenderDoneSemaphore(), _graphicsQueue.getFrameNumber());
    _submittedJob = _computeQueue.getJobNumber();
}

////////////////////////////////////////////////////////////

bool GPUBackend::getJobDone() const
{
    return _computeQueue.getJobDoneSemaphore().getCounterValue() >= _submittedJob;
}

////////////////////////////////////////////////////////////

std::optional<vk::SemaphoreSubmitInfo> GPUBackend::getReady() const
{
    if (_frontJob == 0)
        return std::nullopt; // nothing computed yet
    return vk::SemaphoreSubmitInfo{
        .semaphore = _computeQueue.getJobDoneSemaphore(),
        .value     = _frontJob,
        .stageMask = vk::PipelineStageFlagBits2::eFragmentShader,
    };
}

GPUBackend::Constants GPUBackend::makeConstants(const Fractal& fractal, float aspect)
{
    Constants constants;

    constants.offset  = glm::vec2(fractal.offsetX, fractal.offsetY);
    constants.zoom    = fractal.zoom;
    constants.maxIter = (int)fractal.maxIter;
    constants.aspect  = aspect;

    return constants;
}

////////////////////////////////////////////////////////////