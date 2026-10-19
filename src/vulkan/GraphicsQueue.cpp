#include "GraphicsQueue.hpp"

#include "util/Log.hpp"

////////////////////////////////////////////////////////////

GraphicsQueue::GraphicsQueue(VulkanCore& vulkanCore, uint32_t framesInFlight)
    : _vulkanCore(vulkanCore),
      _framesInFlight(std::clamp(framesInFlight, 1u, vulkanCore.getSwapImageCount()))
{
    createRenderDoneSemaphore();
    createCommandPool();
    createFrameWorkers();
}

////////////////////////////////////////////////////////////

void GraphicsQueue::createRenderDoneSemaphore()
{
    vk::StructureChain<vk::SemaphoreCreateInfo, vk::SemaphoreTypeCreateInfo> chain{
        {},
        {.semaphoreType = vk::SemaphoreType::eTimeline, .initialValue = 0},
    };
    _renderDone = vk::raii::Semaphore(_vulkanCore.getDevice(), chain.get<vk::SemaphoreCreateInfo>());
}

////////////////////////////////////////////////////////////

void GraphicsQueue::createCommandPool()
{
    // --- Create Command Pool --- //
    vk::CommandPoolCreateInfo poolInfo{
        .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = _vulkanCore.getGraphiscsFamilyIndex()
    };
    _commandPool = vk::raii::CommandPool(_vulkanCore.getDevice(), poolInfo);
}

////////////////////////////////////////////////////////////

void GraphicsQueue::createFrameWorkers()
{
    for (uint32_t i = 0; i < _framesInFlight; i++)
        _frameWorkers.emplace_back(*this);
}

////////////////////////////////////////////////////////////

void GraphicsQueue::submit(
    vk::raii::Semaphore& renderFinishedSemaphore, std::optional<vk::SemaphoreSubmitInfo> frontReady
)
{
    auto& frameWorker = _frameWorkers[_frameIndex];

    // wait until the swapchain image is acquired (binary, value ignored)
    std::array<vk::SemaphoreSubmitInfo, 2> waitInfos{
        // binary: the swapchain image is acquired (value ignored)
        vk::SemaphoreSubmitInfo{
            .semaphore = frameWorker.presentCompleteSemaphore,
            .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        },
    };
    // wait for front ready (if any)
    uint32_t waitCount = 1;
    if (frontReady)
        waitInfos[waitCount++] = *frontReady;

    vk::SemaphoreSubmitInfo signalInfos[] = {
        // binary, for present (present cannot wait on a timeline)
        {.semaphore = renderFinishedSemaphore, .stageMask = vk::PipelineStageFlagBits2::eAllCommands},
        // timeline, reaches _frameNumber when this frame's work is done
        {.semaphore = _renderDone, .value = ++_frameNumber, .stageMask = vk::PipelineStageFlagBits2::eAllCommands},
    };

    vk::CommandBufferSubmitInfo commandBufferInfo{.commandBuffer = frameWorker.commandBuffer};

    vk::SubmitInfo2 submitInfo{
        .waitSemaphoreInfoCount   = waitCount,
        .pWaitSemaphoreInfos      = waitInfos.data(),
        .commandBufferInfoCount   = 1,
        .pCommandBufferInfos      = &commandBufferInfo,
        .signalSemaphoreInfoCount = 2,
        .pSignalSemaphoreInfos    = signalInfos,
    };

    // unsignal fence, before submitting to avoid deadlock (if unsignaled and eErrorOutOfDateKHR)
    _vulkanCore.getDevice().resetFences(*frameWorker.drawFence);
    _vulkanCore.getGraphicsQueue().submit2(submitInfo, *frameWorker.drawFence);
}

////////////////////////////////////////////////////////////

void GraphicsQueue::dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    _frameWorkers[_frameIndex].commandBuffer.dispatch(groupX, groupY, groupZ);
}

////////////////////////////////////////////////////////////

void GraphicsQueue::singleCommand(std::function<void(vk::raii::CommandBuffer&)> command)
{
    auto& queue = _vulkanCore.getGraphicsQueue();

    // --- Create Command Buffer --- //
    vk::raii::CommandBuffer commandBuffer = createCommandBuffer();

    vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

    // --- Execute --- //
    commandBuffer.begin(beginInfo);
    command(commandBuffer);
    commandBuffer.end();

    // --- Submit and wait --- //
    vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
    queue.submit(submitInfo, nullptr);
    queue.waitIdle();
}

////////////////////////////////////////////////////////////

void GraphicsQueue::reset()
{
    _frameWorkers[_frameIndex].commandBuffer.reset({});
}

////////////////////////////////////////////////////////////

void GraphicsQueue::begin()
{
    _frameWorkers[_frameIndex].commandBuffer.begin({});
}

////////////////////////////////////////////////////////////

void GraphicsQueue::endRendering()
{
    _frameWorkers[_frameIndex].commandBuffer.endRendering();
}

////////////////////////////////////////////////////////////

void GraphicsQueue::end()
{
    _frameWorkers[_frameIndex].commandBuffer.end();
}

////////////////////////////////////////////////////////////

void GraphicsQueue::waitForFences()
{
    auto& frameWorker = _frameWorkers[_frameIndex];

    auto fenceResult = _vulkanCore.getDevice().waitForFences(*frameWorker.drawFence, vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess)
        Log::warn("vk::Device::waitForFences returned {} !", vk::to_string(fenceResult));
}

////////////////////////////////////////////////////////////

void GraphicsQueue::nextFrame()
{
    _frameIndex = (_frameIndex + 1) % _framesInFlight;
}

////////////////////////////////////////////////////////////

vk::raii::CommandBuffer GraphicsQueue::createCommandBuffer()
{
    // --- Allocate Command Buffers --- //
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = _commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1
    };
    return std::move(_vulkanCore.getDevice().allocateCommandBuffers(allocInfo).front());
}

////////////////////////////////////////////////////////////

GraphicsQueue::FrameWorker::FrameWorker(GraphicsQueue& graphicsQueue)

{
    auto& device = graphicsQueue._vulkanCore.getDevice();

    // Command Buffer
    commandBuffer = graphicsQueue.createCommandBuffer();

    // Semaphore
    presentCompleteSemaphore = vk::raii::Semaphore(device, vk::SemaphoreCreateInfo());

    // Fence
    drawFence = vk::raii::Fence(device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
}

////////////////////////////////////////////////////////////