#pragma once

#include "vulkan/VulkanImage.hpp"

#include <glm/glm.hpp>

class GraphicsPipeline
{
    /* ======================== SETUP ======================= */
  public:
    GraphicsPipeline(VulkanCore& vulkanCore, VulkanImage& vulkanImage);

    /* ================== PUBLIC FUNCTIONS ================== */

    void updateDescriptorSet();

    void bind(VulkanQueue& graphicsQueue);

    /* ================== HELPER FUNCTIONS ================== */
  private:
    void createDescriptorPool();

    void createDescriptorLayout();

    void createDescriptorSet();

    void createPipeline();

    /* ====================== VARIABLES ===================== */

    VulkanCore&  _vulkanCore;
    VulkanImage& _vulkanImage;

    vk::raii::PipelineLayout _pipelineLayout = nullptr;
    vk::raii::Pipeline       _pipeline       = nullptr;

    vk::raii::DescriptorPool      _descriptorPool      = nullptr;
    vk::raii::DescriptorSetLayout _descriptorSetLayout = nullptr;
    vk::raii::DescriptorSet       _descriptorSet       = nullptr;
};