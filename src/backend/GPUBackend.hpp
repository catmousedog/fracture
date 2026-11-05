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
        alignas(8) glm::vec2 offset = {0.0f, 0.0f};
        alignas(4) float zoom       = 1.0f;
        alignas(4) int maxIter      = 100;
        alignas(4) float aspect     = 1.0f;
    };

    /* ======================== SETUP ======================= */
  public:
    GPUBackend(VulkanCore& vulkanCore, VulkanImage& vulkanImage, VulkanQueue& vulkanQueue, Settings& settings);

    virtual ~GPUBackend() = default;

    /* ================== HELPER FUNCTIONS ================== */
  private:
    virtual void onResize() override;

    virtual void onFractalChange() override;

    virtual void submitJob() override;

    Constants makeConstants(const Fractal& fractal, float aspect);

    /* ====================== VARIABLES ===================== */

    VulkanCore&     _vulkanCore;
    VulkanImage&    _vulkanImage;
    VulkanQueue&    _vulkanQueue;
    ComputePipeline _computePipeline;

    vk::raii::QueryPool _queryPool = nullptr;
    float               _timestampPeriod; // ns per timestamp period
    uint64_t            _timestampMask;
    vector<bool>        _timestampReady;
};