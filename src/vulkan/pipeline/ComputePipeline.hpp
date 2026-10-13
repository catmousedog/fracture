#pragma once

#include "vulkan/VulkanImage.hpp"
#include "vulkan/pipeline/Pipeline.hpp"

class ComputePipeline : public Pipeline
{
    /* ======================== SETUP ======================= */
  public:
    ComputePipeline(VulkanCore& vulkanCore, VulkanImage& image);

  private:
    void createDescriptorPool();

    void createDescriptorLayout();

    void createDescriptorSet();

    void createPipeline();

    /* ==================== CAPABILITIES ==================== */
  public:
    virtual void updateDescriptorSet() override;

    virtual void bind(CommandQueue& commandQueue) override;

    /* ================= GETTERS AND SETTERS ================ */

    VulkanImage& getImage()
    {
        return _image;
    }

    /* ====================== VARIABLES ===================== */
  private:
    VulkanImage& _image;
};