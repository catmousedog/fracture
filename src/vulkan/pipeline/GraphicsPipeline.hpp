#pragma once

#include "vulkan/VulkanImage.hpp"
#include "vulkan/pipeline/Pipeline.hpp"

class GraphicsPipeline : public Pipeline
{
    /* ======================== SETUP ======================= */
  public:
    GraphicsPipeline(VulkanCore& vulkanCore, VulkanImage& image);

  private:
    void createDescriptorSet();

    void createPipeline();

  public:
    virtual void updateDescriptorSet() override;

    virtual void bind(CommandQueue& commandQueue) override;

    /* ====================== VARIABLES ===================== */
  private:
    VulkanImage& _image;
};