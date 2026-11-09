#pragma once

#include "backend/Backend.hpp"
#include "fractals/Precision.hpp"
#include "vulkan/ComputePipeline.hpp"

#include <concepts>
#include <glm/glm.hpp>

class GPUBackend : public Backend
{
    /* ======================== SETUP ======================= */
  public:
    GPUBackend(VulkanCore& vulkanCore, VulkanImage& vulkanImage, VulkanQueue& vulkanQueue, Settings& settings);

    virtual ~GPUBackend() = default;

    /* ================== HELPER FUNCTIONS ================== */
  private:
    virtual void onResize() override;

    virtual void onFractalChange() override;

    virtual void submitJob() override;

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