#pragma once

#include "vulkan/VulkanCore.hpp"

class Pipeline
{
    /* ======================== SETUP ======================= */
  public:
    Pipeline(VulkanCore& vulkanCore)
        : _vulkanCore(vulkanCore)
    {
    }

    virtual ~Pipeline() = default;

    /* ====================== VARIABLES ===================== */
  protected:
    VulkanCore& _vulkanCore;

    vk::raii::PipelineLayout _pipelineLayout = nullptr;
    vk::raii::Pipeline       _pipeline       = nullptr;

    vk::raii::DescriptorPool      _descriptorPool      = nullptr;
    vk::raii::DescriptorSetLayout _descriptorSetLayout = nullptr;
    vk::raii::DescriptorSet       _descriptorSet       = nullptr;
};