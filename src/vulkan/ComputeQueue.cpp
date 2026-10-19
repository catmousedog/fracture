#include "ComputeQueue.hpp"

#include "util/Log.hpp"

////////////////////////////////////////////////////////////

ComputeQueue::ComputeQueue(VulkanCore& vulkanCore)
    : _vulkanCore(vulkanCore)
{
    createRenderDoneSemaphore();
    createCommandPool();
    createJobWorkers();
}

////////////////////////////////////////////////////////////

void ComputeQueue::createRenderDoneSemaphore()
{
    vk::StructureChain<vk::SemaphoreCreateInfo, vk::SemaphoreTypeCreateInfo> chain{
        {},
        {.semaphoreType = vk::SemaphoreType::eTimeline, .initialValue = 0},
    };
    _jobDone = vk::raii::Semaphore(_vulkanCore.getDevice(), chain.get<vk::SemaphoreCreateInfo>());
}

////////////////////////////////////////////////////////////

void ComputeQueue::createCommandPool()
{
    // --- Create Command Pool --- //
    vk::CommandPoolCreateInfo poolInfo{
        .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = _vulkanCore.getComputeFamilyIndex()
    };
    _commandPool = vk::raii::CommandPool(_vulkanCore.getDevice(), poolInfo);
}

////////////////////////////////////////////////////////////

void ComputeQueue::createJobWorkers()
{
    for (uint32_t i = 0; i < _jobsInFlight; i++)
        _jobWorkers.emplace_back(*this);
}

////////////////////////////////////////////////////////////

void ComputeQueue::submit(vk::Semaphore renderDone, uint64_t waitValue)
{
    auto& jobWorker = _jobWorkers[_jobIndex];

    // wait until frame is done being sampled
    vk::SemaphoreSubmitInfo waitInfo{
        .semaphore = renderDone,
        .value     = waitValue,
        .stageMask = vk::PipelineStageFlagBits2::eComputeShader,
    };

    vk::SemaphoreSubmitInfo signalInfo = {
        .semaphore = _jobDone, .value = ++_jobNumber, .stageMask = vk::PipelineStageFlagBits2::eAllCommands
    };

    vk::CommandBufferSubmitInfo commandBufferInfo{.commandBuffer = jobWorker.commandBuffer};

    vk::SubmitInfo2 submitInfo{
        .waitSemaphoreInfoCount   = 1,
        .pWaitSemaphoreInfos      = &waitInfo,
        .commandBufferInfoCount   = 1,
        .pCommandBufferInfos      = &commandBufferInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos    = &signalInfo,
    };

    // unsignal fence, before submitting to avoid deadlock (if unsignaled and eErrorOutOfDateKHR)
    _vulkanCore.getDevice().resetFences(*jobWorker.drawFence);
    _vulkanCore.getComputeQueue().submit2(submitInfo, *jobWorker.drawFence);
}

////////////////////////////////////////////////////////////

void ComputeQueue::dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    _jobWorkers[_jobIndex].commandBuffer.dispatch(groupX, groupY, groupZ);
}

////////////////////////////////////////////////////////////

void ComputeQueue::singleCommand(std::function<void(vk::raii::CommandBuffer&)> command)
{
    auto& queue = _vulkanCore.getComputeQueue();

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

void ComputeQueue::reset()
{
    _jobWorkers[_jobIndex].commandBuffer.reset({});
}

////////////////////////////////////////////////////////////

void ComputeQueue::begin()
{
    _jobWorkers[_jobIndex].commandBuffer.begin({});
}

////////////////////////////////////////////////////////////

void ComputeQueue::endRendering()
{
    _jobWorkers[_jobIndex].commandBuffer.endRendering();
}

////////////////////////////////////////////////////////////

void ComputeQueue::end()
{
    _jobWorkers[_jobIndex].commandBuffer.end();
}

////////////////////////////////////////////////////////////

void ComputeQueue::waitForFences()
{
    auto& jobWorker = _jobWorkers[_jobIndex];

    auto fenceResult = _vulkanCore.getDevice().waitForFences(*jobWorker.drawFence, vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess)
        Log::warn("vk::Device::waitForFences returned {} !", vk::to_string(fenceResult));
}

////////////////////////////////////////////////////////////

void ComputeQueue::nextJob()
{
    _jobIndex = (_jobIndex + 1) % _jobsInFlight;
}

////////////////////////////////////////////////////////////

void ComputeQueue::wait()
{
    vk::SemaphoreWaitInfo waitInfo{
        .semaphoreCount = 1,
        .pSemaphores    = &*_jobDone,
        .pValues        = &_jobNumber, // the value the last submit will signal
    };
    auto result = _vulkanCore.getDevice().waitSemaphores(waitInfo, UINT64_MAX);

    if (result != vk::Result::eSuccess)
        Log::error("vk::Device::waitSemaphores returned {} !", vk::to_string(result));
}

////////////////////////////////////////////////////////////

vk::raii::CommandBuffer ComputeQueue::createCommandBuffer()
{
    // --- Allocate Command Buffers --- //
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = _commandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1
    };
    return std::move(_vulkanCore.getDevice().allocateCommandBuffers(allocInfo).front());
}

////////////////////////////////////////////////////////////

ComputeQueue::JobWorker::JobWorker(ComputeQueue& ComputeQueue)

{
    auto& device = ComputeQueue._vulkanCore.getDevice();

    // Command Buffer
    commandBuffer = ComputeQueue.createCommandBuffer();

    // Fence
    drawFence = vk::raii::Fence(device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
}

////////////////////////////////////////////////////////////