#pragma once

#include "backend/Backend.hpp"
#include "vulkan/ComputePipeline.hpp"

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
    GPUBackend(VulkanCore& vulkanCore, CommandQueue& commandQueue, Swappable<VulkanImage>& vulkanImages);

    virtual ~GPUBackend() = default;

    /* ================== VIRTUAL FUNCTIONS ================= */
  private:
    virtual void recompute() override;

    virtual void onResize() override;

    /* ================== HELPER FUNCTIONS ================== */

    Constants makeConstants(const Fractal& fractal, glm::uvec2 computeSize, float aspect);

    /* ====================== VARIABLES ===================== */

    VulkanCore&             _vulkanCore;
    CommandQueue&           _commandQueue;
    Swappable<VulkanImage>& _vulkanImages;

    ComputePipeline _computePipeline;

    vk::raii::QueryPool _queryPool = nullptr;
    float               _timestampPeriod; // ns per timestamp period

    vector<bool> _queryReset;
};