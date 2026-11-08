#pragma once

#include "backend/Backend.hpp"
#include "fractals/Fractal.hpp"
#include "vulkan/VulkanImage.hpp"

class ComputePipeline
{
    /* ======================== SETUP ======================= */
  public:
    ComputePipeline(VulkanCore& vulkanCore, VulkanImage& vulkanImage, const Backend::Settings& settings);

  private:
    void createDescriptorPool();

    void createDescriptorLayout();

    void createDescriptorSet();

    /* ================== PUBLIC FUNCTIONS ================== */
  public:
    void updatePipeline(uint32_t pushConstantSize);

    template <typename T>
    void pushConstants(VulkanQueue& computeQueue, T& constants)
    {
        auto& commandBuffer = computeQueue.getCommandBuffer();
        commandBuffer.pushConstants(_pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(T), &constants);
    }

    void updateDescriptorSet();

    void readShader(const Backend::Settings& settings);

    void bind(VulkanQueue& computeQueue);

    /* ====================== VARIABLES ===================== */
  private:
    VulkanCore&  _vulkanCore;
    VulkanImage& _vulkanImage;

    vk::raii::PipelineLayout _pipelineLayout = nullptr;
    vk::raii::Pipeline       _pipeline       = nullptr;

    vk::raii::DescriptorPool      _descriptorPool      = nullptr;
    vk::raii::DescriptorSetLayout _descriptorSetLayout = nullptr;
    vk::raii::DescriptorSet       _descriptorSet       = nullptr;

    vk::raii::ShaderModule _shader = nullptr;
};