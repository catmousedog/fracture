#pragma once

#include "util/Basics.hpp"
#include "vulkan/VulkanCore.hpp"

#include <functional>

class ComputeQueue
{
    /* ==================== NESTED CLASS ==================== */

    struct JobWorker
    {
        JobWorker(ComputeQueue& ComputeQueue);

        vk::raii::CommandBuffer commandBuffer = nullptr;
        vk::raii::Fence         drawFence     = nullptr;
        // vk::raii::Semaphore     presentCompleteSemaphore = nullptr;
    };

    /* ======================== SETUP ======================= */
  public:
    ComputeQueue(VulkanCore& vulkanCore);

    void createRenderDoneSemaphore();

    void createCommandPool();

    void createJobWorkers();

    /* ================== PUBLIC FUNCTIONS ================== */

    void submit(vk::Semaphore renderDone, uint64_t waitValue);

    void dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ);

    void singleCommand(std::function<void(vk::raii::CommandBuffer&)> command);

    void reset();

    void begin();

    void endRendering();

    void end();

    void waitForFences();

    void nextJob();

    void wait();

    /* ================== HELPER FUNCTIONS ================== */
  private:
    vk::raii::CommandBuffer createCommandBuffer();

    /* ================= GETTERS AND SETTERS ================ */
  public:
    vk::raii::CommandBuffer&   getCommandBuffer() { return _jobWorkers[_jobIndex].commandBuffer; }
    uint32_t                   getJobsInFlight() const { return _jobsInFlight; }
    uint32_t                   getJobIndex() const { return _jobIndex; }
    const vk::raii::Semaphore& getJobDoneSemaphore() const { return _jobDone; }
    uint64_t                   getJobNumber() const { return _jobNumber; }

    /* ====================== VARIABLES ===================== */
  private:
    VulkanCore& _vulkanCore;

    vk::raii::CommandPool _commandPool = nullptr;

    uint32_t          _jobsInFlight = 1; // front and back
    uint32_t          _jobIndex     = 0;
    vector<JobWorker> _jobWorkers;

    // timeline: value N = job N's compute work is done
    vk::raii::Semaphore _jobDone   = nullptr;
    uint64_t            _jobNumber = 0;
};