#pragma once

#include "backend/Backend.hpp"
#include "vulkan/ComputePipeline.hpp"
#include "vulkan/ComputeQueue.hpp"

#include <glm/glm.hpp>

class GPUBackend : public Backend
{
    /* ==================== NESTED CLASS ==================== */
  private:
    struct Constants
    {
        alignas(8) glm::vec2 offset       = {0.0f, 0.0f};
        alignas(4) float zoom             = 1.0f;
        alignas(4) int maxIter            = 100;
        alignas(8) glm::uvec2 computeSize = {16, 16};
        alignas(4) float aspect           = 1.0f;
    };

    /* ======================== SETUP ======================= */
  public:
    GPUBackend(VulkanCore& vulkanCore, Swappable<VulkanImage>& vulkanImages, GraphicsQueue& graphicsQueue);

    virtual ~GPUBackend() = default;

    /* ================== HELPER FUNCTIONS ================== */
  private:
    virtual void onResize() override;

    virtual void onJobDone() override;

    virtual void submitJob() override;

    virtual std::optional<vk::SemaphoreSubmitInfo> getReady() const override;

    Constants makeConstants(const Fractal& fractal, glm::uvec2 computeSize, float aspect);

    /* ================= GETTERS AND SETTERS ================ */
    bool getJobDone() const override;

    /* ====================== VARIABLES ===================== */

    VulkanCore&             _vulkanCore;
    Swappable<VulkanImage>& _vulkanImages;
    GraphicsQueue&          _graphicsQueue;
    ComputeQueue            _computeQueue;
    ComputePipeline         _computePipeline;

    vk::raii::QueryPool _queryPool = nullptr;
    float               _timestampPeriod; // ns per timestamp period
    uint64_t            _timestampMask;

    uint64_t _frontJob = 0, _submittedJob = 0;
};