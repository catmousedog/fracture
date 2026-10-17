#pragma once

#include "vulkan/VulkanImage.hpp"

class GraphicsPipeline
{
    /* ======================== SETUP ======================= */
  public:
    GraphicsPipeline(VulkanCore& vulkanCore, VulkanImage& image);

  private:
    void createDescriptorPool();

    void createDescriptorLayout();

    void createDescriptorSet();

    void createPipeline();

  public:
    void updateDescriptorSet();

    void bind(CommandQueue& commandQueue);

    /* ====================== VARIABLES ===================== */
  private:
    VulkanCore&  _vulkanCore;
    VulkanImage& _image;

    vk::raii::PipelineLayout _pipelineLayout = nullptr;
    vk::raii::Pipeline       _pipeline       = nullptr;

    vk::raii::DescriptorPool      _descriptorPool      = nullptr;
    vk::raii::DescriptorSetLayout _descriptorSetLayout = nullptr;
    vk::raii::DescriptorSet       _descriptorSet       = nullptr;
};