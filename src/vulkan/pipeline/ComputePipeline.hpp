#pragma once

#include "vulkan/PushConstants.hpp"
#include "vulkan/VulkanImage.hpp"
#include "vulkan/pipeline/Pipeline.hpp"

class ComputePipeline : public Pipeline
{
    /* ======================== SETUP ======================= */
  public:
    ComputePipeline(VulkanCore& vulkanCore, VulkanImage& image, const PushConstants& pushConstants);

  private:
    void createDescriptorSet();

    void createPipeline();

    /* ==================== CAPABILITIES ==================== */
  public:
    virtual void updateDescriptorSet() override;

    virtual void bind(CommandQueue& commandQueue) override;

    /* ====================== VARIABLES ===================== */
  private:
    VulkanImage&         _image;
    const PushConstants& _pushConstants;
};