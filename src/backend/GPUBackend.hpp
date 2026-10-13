#pragma once

#include "backend/Backend.hpp"
#include "vulkan/pipeline/ComputePipeline.hpp"

class GPUBackend : public Backend
{

  public:
    GPUBackend(VulkanCore& vulkanCore, CommandQueue& commandQueue, VulkanImage& vulkanImage);

    virtual ~GPUBackend() = default;

    virtual void recompute() override;

    virtual void resize() override;

  private:
    VulkanCore&   _vulkanCore;
    CommandQueue& _commandQueue;

    ComputePipeline _computePipeline;
};