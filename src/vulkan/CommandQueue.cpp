#include "CommandQueue.hpp"

#include "util/Log.hpp"

////////////////////////////////////////////////////////////

CommandQueue::CommandQueue(VulkanCore& vulkanCore, uint32_t framesInFlight)
    : _vulkanCore(vulkanCore),
      _framesInFlight(std::clamp(framesInFlight, 1u, vulkanCore.getSwapImageCount()))
{
    createCommandPool();
    createFrameWorkers();
}

////////////////////////////////////////////////////////////

void CommandQueue::createCommandPool()
{
    // --- Create Command Pool --- //
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = _vulkanCore.getFamilyIndex()
    };
    _commandPool = vk::raii::CommandPool(_vulkanCore.getDevice(), poolInfo);
}

////////////////////////////////////////////////////////////

void CommandQueue::createFrameWorkers()
{
    for (uint32_t i = 0; i < _framesInFlight; i++)
        _frameWorkers.emplace_back(*this);
}

////////////////////////////////////////////////////////////

void CommandQueue::submit(vk::raii::Semaphore& renderFinishedSemaphore)
{
    auto& frameWorker = _frameWorkers[_frameIndex];

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo   submitInfo{
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &*frameWorker.presentCompleteSemaphore, // wait for this frame to finish presenting
        .pWaitDstStageMask    = &waitDestinationStageMask,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &*frameWorker.commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &*renderFinishedSemaphore // signal this command buffer finished
    };

    // unsignal fence, before submitting to avoid deadlock (if unsignaled and eErrorOutOfDateKHR)
    _vulkanCore.getDevice().resetFences(*frameWorker.drawFence);
    _vulkanCore.getQueue().submit(submitInfo, *frameWorker.drawFence);
}

////////////////////////////////////////////////////////////

void CommandQueue::dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    _frameWorkers[_frameIndex].commandBuffer.dispatch(groupX, groupY, groupZ);
}

////////////////////////////////////////////////////////////

void CommandQueue::singleCommand(std::function<void(vk::raii::CommandBuffer&)> command)
{
    auto& queue = _vulkanCore.getQueue();

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

void CommandQueue::reset()
{
    _frameWorkers[_frameIndex].commandBuffer.reset({});
}

////////////////////////////////////////////////////////////

void CommandQueue::begin()
{
    _frameWorkers[_frameIndex].commandBuffer.begin({});
}

////////////////////////////////////////////////////////////

void CommandQueue::endRendering()
{
    _frameWorkers[_frameIndex].commandBuffer.endRendering();
}

////////////////////////////////////////////////////////////

void CommandQueue::end()
{
    _frameWorkers[_frameIndex].commandBuffer.end();
}

////////////////////////////////////////////////////////////

void CommandQueue::waitForFences()
{
    auto& frameWorker = _frameWorkers[_frameIndex];

    auto fenceResult = _vulkanCore.getDevice().waitForFences(*frameWorker.drawFence, vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess)
        Log::warn("vk::Device::waitForFences returned {} !", vk::to_string(fenceResult));
}

////////////////////////////////////////////////////////////

void CommandQueue::nextFrame()
{
    _frameIndex = (_frameIndex + 1) % _framesInFlight;
}

////////////////////////////////////////////////////////////

vk::raii::CommandBuffer CommandQueue::createCommandBuffer()
{
    // --- Allocate Command Buffers --- //
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = _commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1
    };
    return std::move(_vulkanCore.getDevice().allocateCommandBuffers(allocInfo).front());
}

////////////////////////////////////////////////////////////

CommandQueue::FrameWorker::FrameWorker(CommandQueue& commandQueue)

{
    auto& device = commandQueue._vulkanCore.getDevice();

    // Command Buffer
    commandBuffer = commandQueue.createCommandBuffer();

    // Semaphore
    presentCompleteSemaphore = vk::raii::Semaphore(device, vk::SemaphoreCreateInfo());

    // Fence
    drawFence = vk::raii::Fence(device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
}

////////////////////////////////////////////////////////////