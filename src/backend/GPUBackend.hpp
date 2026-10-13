#pragma once

#include "backend/Backend.hpp"
#include "vulkan/pipeline/ComputePipeline.hpp"

class GPUBackend : public Backend
{
    /* ======================== SETUP ======================= */
  public:
    GPUBackend(VulkanCore& vulkanCore, CommandQueue& commandQueue, VulkanImage& vulkanImage);

    virtual ~GPUBackend() = default;

    /* ================== VIRTUAL FUNCTIONS ================= */
  private:
    virtual void recompute() override;

    virtual void onResize() override;

    /* ====================== VARIABLES ===================== */

    VulkanCore&   _vulkanCore;
    CommandQueue& _commandQueue;

    ComputePipeline _computePipeline;
};