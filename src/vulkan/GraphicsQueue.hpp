#pragma once

#include "util/Basics.hpp"
#include "vulkan/VulkanCore.hpp"

#include <functional>

class GraphicsQueue
{
    /* ==================== NESTED CLASS ==================== */

    struct FrameWorker
    {
        FrameWorker(GraphicsQueue& graphicsQueue);

        vk::raii::CommandBuffer commandBuffer            = nullptr;
        vk::raii::Fence         drawFence                = nullptr;
        vk::raii::Semaphore     presentCompleteSemaphore = nullptr;
    };

    /* ======================== SETUP ======================= */
  public:
    GraphicsQueue(VulkanCore& vulkanCore, uint32_t framesInFlight);

    void createCommandPool();

    void createFrameWorkers();

    void createRenderDoneSemaphore();

    /* ================== PUBLIC FUNCTIONS ================== */

    void submit(vk::raii::Semaphore& renderFinishedSemaphore, std::optional<vk::SemaphoreSubmitInfo>);

    void dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ);

    void singleCommand(std::function<void(vk::raii::CommandBuffer&)> command);

    void reset();

    void begin();

    void endRendering();

    void end();

    void waitForFences();

    void nextFrame();

    /* ================== HELPER FUNCTIONS ================== */
  private:
    vk::raii::CommandBuffer createCommandBuffer();

    /* ================= GETTERS AND SETTERS ================ */
  public:
    vk::raii::CommandBuffer& getCommandBuffer() { return _frameWorkers[_frameIndex].commandBuffer; }
    vk::raii::Semaphore& getPresentCompleteSemaphore() { return _frameWorkers[_frameIndex].presentCompleteSemaphore; }
    uint32_t             getFramesInFlight() const { return _framesInFlight; }
    uint32_t             getFrameIndex() const { return _frameIndex; }
    vk::raii::Semaphore& getRenderDoneSemaphore() { return _renderDone; }
    uint64_t             getFrameNumber() const { return _frameNumber; }

    /* ====================== VARIABLES ===================== */
  private:
    VulkanCore& _vulkanCore;

    vk::raii::CommandPool _commandPool = nullptr;

    uint32_t            _framesInFlight = 0;
    uint32_t            _frameIndex     = 0;
    vector<FrameWorker> _frameWorkers;

    // timeline: value N = frame N's graphics work is done
    vk::raii::Semaphore _renderDone  = nullptr;
    uint64_t            _frameNumber = 0;
};