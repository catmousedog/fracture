#pragma once

#include "backend/Backend.hpp"
#include "vulkan/pipeline/ComputePipeline.hpp"

#include <glm/glm.hpp>

class GPUBackend : public Backend
{
    /* ==================== NESTED CLASS ==================== */
  private:
    struct Constants
    {
        alignas(8) glm::vec2 offset = {0.0f, 0.0f};
        alignas(4) float zoom       = 1.0f;
        alignas(4) int maxIter      = 100;
    };

    /* ======================== SETUP ======================= */
  public:
    GPUBackend(VulkanCore& vulkanCore, CommandQueue& commandQueue, VulkanImage& vulkanImage);

    virtual ~GPUBackend() = default;

    /* ================== VIRTUAL FUNCTIONS ================= */
  private:
    virtual void recompute() override;

    virtual void onResize() override;

    /* ================== HELPER FUNCTIONS ================== */

    Constants makeConstants(const Fractal& fractal);

    /* ====================== VARIABLES ===================== */

    VulkanCore&   _vulkanCore;
    CommandQueue& _commandQueue;

    ComputePipeline _computePipeline;

    vk::raii::QueryPool _queryPool = nullptr;
    float               _timestampPeriod; // ns per timestamp period

    vector<bool> _queryReset;
};