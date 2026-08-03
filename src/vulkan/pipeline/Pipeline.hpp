#pragma once

#include "vulkan/VulkanCore.hpp"

class Pipeline
{
    /* ======================== SETUP ======================= */
  public:
    Pipeline(VulkanCore& vulkanCore, vk::DescriptorType type, vk::ShaderStageFlagBits stage);

    virtual ~Pipeline();

  private:
    void createDescriptorPool(vk::DescriptorType type);

    void createDescriptorLayout(vk::DescriptorType type, vk::ShaderStageFlagBits stage);

    /* ==================== CAPABILITIES ==================== */
  public:
    virtual void updateDescriptorSet() = 0;

    virtual void bind(CommandQueue& commandQueue) = 0;

    /* ====================== VARIABLES ===================== */
  protected:
    VulkanCore& _vulkanCore;

    vk::raii::PipelineLayout _pipelineLayout = nullptr;
    vk::raii::Pipeline       _pipeline       = nullptr;

    vk::raii::DescriptorPool      _descriptorPool      = nullptr;
    vk::raii::DescriptorSetLayout _descriptorSetLayout = nullptr;
    vk::raii::DescriptorSet       _descriptorSet       = nullptr;
};