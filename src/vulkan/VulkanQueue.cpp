#include "VulkanQueue.hpp"

#include "util/Log.hpp"

////////////////////////////////////////////////////////////

VulkanQueue::VulkanQueue(VulkanCore& vulkanCore, uint32_t framesInFlight)
    : _vulkanCore(vulkanCore),
      _framesInFlight(std::clamp(framesInFlight, 1u, vulkanCore.getSwapImageCount()))
{
    createCommandPool();
    createFrameWorkers();
}

////////////////////////////////////////////////////////////

void VulkanQueue::createCommandPool()
{
    // --- Create Command Pool --- //
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = _vulkanCore.getFamilyIndex()
    };
    _commandPool = vk::raii::CommandPool(_vulkanCore.getDevice(), poolInfo);
}

////////////////////////////////////////////////////////////

void VulkanQueue::createFrameWorkers()
{
    for (uint32_t i = 0; i < _framesInFlight; i++)
        _frameWorkers.emplace_back(*this);
}

////////////////////////////////////////////////////////////

void VulkanQueue::submit(uint32_t swapChainIndex)
{
    auto& frameWorker = _frameWorkers[_frameIndex];

    // wait until the swapchain image is acquired (binary, value ignored)
    // binary: the swapchain image is acquired (value ignored)
    vk::SemaphoreSubmitInfo waitInfo{
        .semaphore = frameWorker.presentCompleteSemaphore,
        .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    };

    vk::SemaphoreSubmitInfo signalInfo{
        .semaphore = _vulkanCore.getRenderFinished(swapChainIndex),
        .stageMask = vk::PipelineStageFlagBits2::eAllCommands
    };

    vk::CommandBufferSubmitInfo commandBufferInfo{.commandBuffer = frameWorker.commandBuffer};

    vk::SubmitInfo2 submitInfo{
        .waitSemaphoreInfoCount   = 1,
        .pWaitSemaphoreInfos      = &waitInfo,
        .commandBufferInfoCount   = 1,
        .pCommandBufferInfos      = &commandBufferInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos    = &signalInfo
    };

    // unsignal fence, before submitting to avoid deadlock (if unsignaled and eErrorOutOfDateKHR)
    _vulkanCore.getDevice().resetFences(*frameWorker.drawFence);
    _vulkanCore.getQueue().submit2(submitInfo, *frameWorker.drawFence);
}

////////////////////////////////////////////////////////////

void VulkanQueue::dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    _frameWorkers[_frameIndex].commandBuffer.dispatch(groupX, groupY, groupZ);
}

////////////////////////////////////////////////////////////

void VulkanQueue::singleCommand(std::function<void(vk::raii::CommandBuffer&)> command)
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

void VulkanQueue::reset()
{
    _frameWorkers[_frameIndex].commandBuffer.reset({});
}

////////////////////////////////////////////////////////////

void VulkanQueue::begin()
{
    _frameWorkers[_frameIndex].commandBuffer.begin({});
}

////////////////////////////////////////////////////////////

void VulkanQueue::endRendering()
{
    _frameWorkers[_frameIndex].commandBuffer.endRendering();
}

////////////////////////////////////////////////////////////

void VulkanQueue::end()
{
    _frameWorkers[_frameIndex].commandBuffer.end();
}

////////////////////////////////////////////////////////////

void VulkanQueue::waitForFences()
{
    auto& frameWorker = _frameWorkers[_frameIndex];

    auto fenceResult = _vulkanCore.getDevice().waitForFences(*frameWorker.drawFence, vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess)
        Log::warn("vk::Device::waitForFences returned {} !", vk::to_string(fenceResult));
}

////////////////////////////////////////////////////////////

void VulkanQueue::nextFrame()
{
    _frameIndex = (_frameIndex + 1) % _framesInFlight;
}

////////////////////////////////////////////////////////////

vk::raii::CommandBuffer VulkanQueue::createCommandBuffer()
{
    // --- Allocate Command Buffers --- //
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = _commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1
    };
    return std::move(_vulkanCore.getDevice().allocateCommandBuffers(allocInfo).front());
}

////////////////////////////////////////////////////////////

VulkanQueue::FrameWorker::FrameWorker(VulkanQueue& graphicsQueue)

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